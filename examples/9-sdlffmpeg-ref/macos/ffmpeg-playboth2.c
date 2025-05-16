#include <SDL.h>
#include <SDL_render.h>
#include <SDL_thread.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libavutil/time.h> // For av_gettime_relative
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h> // For isnan

#ifdef __WIIU__
#include <coreinit/thread.h>
#endif

// Define queue sizes
#define MAX_AUDIO_QUEUE_SIZE (10 * 1024 * 1024) // 10MB
#define MAX_VIDEO_QUEUE_SIZE 5 // Max 5 video frames

// Timeout for queue pop in milliseconds
#define QUEUE_POP_TIMEOUT_MS 10

// Simple queue structure for packets
typedef struct PacketQueueNode {
    AVPacket packet;
    int serial; // Serial for handling stream changes/seeks
    struct PacketQueueNode *next;
} PacketQueueNode;

typedef struct PacketQueue {
    PacketQueueNode *first;
    PacketQueueNode *last;
    int nb_packets;
    int size; // Size in bytes
    int abort_request; // Flag to signal threads to stop waiting
    int serial; // Current serial number for the queue
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

// Simple queue structure for frames
typedef struct FrameQueueNode {
    AVFrame *frame;
    double pts; // Presentation timestamp in seconds
    int serial; // Serial for handling stream changes/seeks
    struct FrameQueueNode *next;
} FrameQueueNode;

typedef struct FrameQueue {
    FrameQueueNode *first;
    FrameQueueNode *last;
    int nb_frames;
    int abort_request; // Flag to signal threads to stop waiting
    int serial; // Current serial number for the queue
    SDL_mutex *mutex;
    SDL_cond *cond;
} FrameQueue;


// Structure to hold video and audio player context
typedef struct {
    const char *filepath;
    AVFormatContext *format_context;

    // Video related
    int video_stream_index;
    AVCodecContext *video_codec_context;
    AVFrame *video_frame; // Frame for decoding
    AVFrame *rgb_frame; // Frame for SWS conversion
    struct SwsContext *sws_context;
    int width;
    int height;
    double video_time_base; // Time base for video stream in seconds

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    FrameQueue video_queue; // Queue for decoded video frames

    // Audio related
    int audio_stream_index;
    AVCodecContext *audio_codec_context;
    AVFrame *audio_frame; // Frame for decoding
    SwrContext *swr_ctx;
    SDL_AudioDeviceID audio_device_id;
    double audio_time_base; // Time base for audio stream in seconds
    double audio_clock; // Current audio playback time in seconds (master clock)
    SDL_mutex *audio_clock_mutex; // Mutex for audio_clock
    uint8_t *audio_buffer; // Buffer for resampled audio data
    unsigned int audio_buffer_size; // Size of audio_buffer
    unsigned int audio_buffer_index; // Current read position in audio_buffer

    FrameQueue audio_queue; // Queue for decoded audio frames/data

    int quit; // Global quit flag
    SDL_Thread *decode_thread; // Thread for reading and decoding

} MediaContext;

// Initialize a packet queue
void packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
    q->abort_request = 0;
    q->serial = 0;
}

// Destroy a packet queue
void packet_queue_destroy(PacketQueue *q) {
    PacketQueueNode *node, *tmp;
    SDL_LockMutex(q->mutex);
    q->abort_request = 1; // Signal waiting threads to abort
    SDL_CondSignal(q->cond); // Signal all waiting threads
    SDL_UnlockMutex(q->mutex);

    // Wait a bit for threads to potentially wake up and exit waiting
    SDL_Delay(50);

    SDL_LockMutex(q->mutex);
    node = q->first;
    while (node) {
        tmp = node->next;
        av_packet_unref(&node->packet);
        free(node);
        node = tmp;
    }
    q->first = NULL;
    q->last = NULL;
    q->nb_packets = 0;
    q->size = 0;
    SDL_UnlockMutex(q->mutex);
    SDL_DestroyMutex(q->mutex);
    SDL_DestroyCond(q->cond);
}

// Push a packet onto the queue
int packet_queue_push(PacketQueue *q, AVPacket *pkt) {
    PacketQueueNode *node = (PacketQueueNode *)malloc(sizeof(PacketQueueNode));
    if (!node) {
        av_packet_unref(pkt); // Free packet if node allocation fails
        fprintf(stderr, "Error allocating PacketQueueNode\n");
        return -1;
    }
    node->packet = *pkt; // Copy the packet data
    node->serial = q->serial;
    node->next = NULL;

    SDL_LockMutex(q->mutex);
    if (q->abort_request) {
        SDL_UnlockMutex(q->mutex);
        av_packet_unref(pkt);
        free(node);
        return -1; // Aborting
    }

    if (!q->last) {
        q->first = node;
    } else {
        q->last->next = node;
    }
    q->last = node;
    q->nb_packets++;
    q->size += node->packet.size;
    SDL_CondSignal(q->cond); // Signal that a packet is available
    SDL_UnlockMutex(q->mutex);
    return 0;
}

// Pop a packet from the queue
int packet_queue_pop(PacketQueue *q, AVPacket *pkt, int block, int *serial) {
    PacketQueueNode *node;
    int ret = -1;

    SDL_LockMutex(q->mutex);
    while (1) {
        node = q->first;
        if (node) {
            q->first = node->next;
            if (!q->first) {
                q->last = NULL;
            }
            q->nb_packets--;
            q->size -= node->packet.size;
            *pkt = node->packet; // Copy the packet data
            if (serial) *serial = node->serial;
            free(node);
            ret = 1; // Success
            break;
        } else if (block) {
            SDL_CondWait(q->cond, q->mutex);
        } else {
            ret = 0; // No packet available
            break;
        }
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}

// Flush a packet queue
void packet_queue_flush(PacketQueue *q) {
    PacketQueueNode *node, *tmp;
    SDL_LockMutex(q->mutex);
    node = q->first;
    while (node) {
        tmp = node->next;
        av_packet_unref(&node->packet);
        free(node);
        node = tmp;
    }
    q->first = NULL;
    q->last = NULL;
    q->nb_packets = 0;
    q->size = 0;
    q->serial++; // Increment serial on flush
    SDL_UnlockMutex(q->mutex);
}

// Start a packet queue (reset abort request and increment serial)
void packet_queue_start(PacketQueue *q) {
    SDL_LockMutex(q->mutex);
    q->abort_request = 0;
    q->serial++;
    SDL_UnlockMutex(q->mutex);
}


// Initialize a frame queue
void frame_queue_init(FrameQueue *q) {
    memset(q, 0, sizeof(FrameQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
    q->abort_request = 0;
    q->serial = 0;
}

// Destroy a frame queue
void frame_queue_destroy(FrameQueue *q) {
    FrameQueueNode *node, *tmp;
    SDL_LockMutex(q->mutex);
    q->abort_request = 1; // Signal waiting threads to abort
    SDL_CondSignal(q->cond); // Signal all waiting threads
    SDL_UnlockMutex(q->mutex);

    // Wait a bit for threads to potentially wake up and exit waiting
    SDL_Delay(50);

    SDL_LockMutex(q->mutex);
    node = q->first;
    while (node) {
        tmp = node->next;
        av_frame_free(&node->frame);
        free(node);
        node = tmp;
    }
    q->first = NULL;
    q->last = NULL;
    q->nb_frames = 0;
    SDL_UnlockMutex(q->mutex);
    SDL_DestroyMutex(q->mutex);
    SDL_DestroyCond(q->cond);
}

// Push a frame onto the queue
int frame_queue_push(FrameQueue *q, AVFrame *frame, double pts, int serial) {
    FrameQueueNode *node = (FrameQueueNode *)malloc(sizeof(FrameQueueNode));
    if (!node) {
        av_frame_free(&frame); // Free frame if node allocation fails
        fprintf(stderr, "Error allocating FrameQueueNode\n");
        return -1;
    }
    node->frame = frame;
    node->pts = pts;
    node->serial = serial;
    node->next = NULL;

    SDL_LockMutex(q->mutex);
    // Wait if the queue is full and not quitting
    while (q->nb_frames >= MAX_VIDEO_QUEUE_SIZE && !q->abort_request) {
        SDL_CondWait(q->cond, q->mutex);
    }
    if (q->abort_request) {
        SDL_UnlockMutex(q->mutex);
        av_frame_free(&frame);
        free(node);
        return -1; // Aborting
    }

    if (!q->last) {
        q->first = node;
    } else {
        q->last->next = node;
    }
    q->last = node;
    q->nb_frames++;
    SDL_CondSignal(q->cond); // Signal that a frame is available
    SDL_UnlockMutex(q->mutex);
    return 0;
}

// Pop a frame from the queue with a timeout
AVFrame *frame_queue_pop(FrameQueue *q, double *pts, int *serial) {
    FrameQueueNode *node;
    AVFrame *frame = NULL;

    SDL_LockMutex(q->mutex);
    while (1) {
        node = q->first;
        if (node) {
            q->first = node->next;
            if (!q->first) {
                q->last = NULL;
            }
            q->nb_frames--;
            frame = node->frame;
            if (pts) *pts = node->pts;
            if (serial) *serial = node->serial;
            free(node);
            SDL_CondSignal(q->cond); // Signal that space is available (for push)
            break;
        } else if (!q->abort_request) {
            // Wait with a timeout
            int wait_result = SDL_CondWaitTimeout(q->cond, q->mutex, QUEUE_POP_TIMEOUT_MS);
            if (wait_result == SDL_MUTEX_TIMEDOUT) {
                SDL_UnlockMutex(q->mutex);
                return NULL; // Return NULL on timeout
            }
        } else {
            // Abort requested
            SDL_UnlockMutex(q->mutex);
            return NULL;
        }
    }
    SDL_UnlockMutex(q->mutex);
    return frame;
}

// Get the first frame from the queue without removing it
AVFrame *frame_queue_peek(FrameQueue *q, double *pts, int *serial) {
    FrameQueueNode *node;
    AVFrame *frame = NULL;
    SDL_LockMutex(q->mutex);
    if (q->first) {
        node = q->first;
        frame = node->frame;
        if (pts) *pts = node->pts;
        if (serial) *serial = node->serial;
    }
    SDL_UnlockMutex(q->mutex);
    return frame;
}


void audio_callback(void *userdata, Uint8 *stream, int len) {
    MediaContext *ctx = (MediaContext *)userdata;
    int audio_size;

    SDL_memset(stream, 0, len); // Fill the stream with silence initially

    SDL_LockMutex(ctx->audio_clock_mutex);
    double current_audio_time = ctx->audio_clock;
    SDL_UnlockMutex(ctx->audio_clock_mutex);

    // While there's space in the SDL buffer and we have audio data
    while (len > 0) {
        if (ctx->audio_buffer_index >= ctx->audio_buffer_size) {
            // Need to decode and resample more audio
            AVFrame *audio_frame = frame_queue_pop(&ctx->audio_queue, &current_audio_time, NULL);
            if (!audio_frame) {
                // No audio frames in the queue, output silence
                break;
            }

            // Resample the audio frame
            int out_samples = av_rescale_rnd(
                swr_get_delay(ctx->swr_ctx, audio_frame->sample_rate) + audio_frame->nb_samples,
                44100, audio_frame->sample_rate, AV_ROUND_UP);

            int out_size = av_samples_get_buffer_size(NULL, 2, out_samples, AV_SAMPLE_FMT_S16, 0);
            if (out_size < 0) {
                 fprintf(stderr, "audio_callback: av_samples_get_buffer_size() failed\n");
                 av_frame_free(&audio_frame);
                 break;
            }

            // Allocate buffer for resampled data (reallocate if needed)
            if (!ctx->audio_buffer || ctx->audio_buffer_size < out_size) {
                 av_free(ctx->audio_buffer);
                 ctx->audio_buffer_size = out_size; // Use out_size for buffer size
                 ctx->audio_buffer = av_malloc(ctx->audio_buffer_size);
                 if (!ctx->audio_buffer) {
                     fprintf(stderr, "audio_callback: Error allocating audio buffer\n");
                     av_frame_free(&audio_frame);
                     break;
                 }
            }
             // Reset index as buffer is new or reallocated
            ctx->audio_buffer_index = 0;


            int converted = swr_convert(ctx->swr_ctx, &ctx->audio_buffer, out_samples,
                                        (const uint8_t **)audio_frame->extended_data, audio_frame->nb_samples);
            if (converted < 0) {
                fprintf(stderr, "audio_callback: swr_convert() failed\n");
                av_frame_free(&audio_frame);
                break;
            }
            audio_size = converted * 2 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16); // 2 channels, S16 format
            ctx->audio_buffer_size = audio_size; // Set buffer size to actual converted data size

            av_frame_free(&audio_frame);

            // Update audio clock after processing a frame
            SDL_LockMutex(ctx->audio_clock_mutex);
            ctx->audio_clock = current_audio_time + (double)ctx->audio_buffer_size / (2 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * 44100);
            SDL_UnlockMutex(ctx->audio_clock_mutex);

        }

        int to_copy = ctx->audio_buffer_size - ctx->audio_buffer_index;
        if (to_copy > len) {
            to_copy = len;
        }

        if (to_copy > 0) {
            memcpy(stream, ctx->audio_buffer + ctx->audio_buffer_index, to_copy);
            ctx->audio_buffer_index += to_copy;
            stream += to_copy;
            len -= to_copy;
        }
    }
}


static int decode_thread_func(void *arg) {
    MediaContext *ctx = (MediaContext *)arg;
    AVPacket packet;
    int ret;

    // Initialize packet queues
    PacketQueue video_pkt_queue;
    PacketQueue audio_pkt_queue;
    packet_queue_init(&video_pkt_queue);
    packet_queue_init(&audio_pkt_queue);

    // Start packet queues
    packet_queue_start(&video_pkt_queue);
    packet_queue_start(&audio_pkt_queue);

    while (!ctx->quit) {
        // Read a frame from the media file
        ret = av_read_frame(ctx->format_context, &packet);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                 fprintf(stderr, "Decode thread: End of file.\n");
            } else {
                 fprintf(stderr, "Decode thread: Error reading frame: %s\n", av_err2str(ret));
            }
            // End of stream or error, signal quit after processing remaining packets
            ctx->quit = 1; // Signal main loop to quit
            // Push NULL packets to signal end of stream to decoders
            if (ctx->video_stream_index != -1) {
                AVPacket *null_pkt = av_packet_alloc();
                if (null_pkt) packet_queue_push(&video_pkt_queue, null_pkt); // NULL packet
            }
            if (ctx->audio_stream_index != -1) {
                 AVPacket *null_pkt = av_packet_alloc();
                 if (null_pkt) packet_queue_push(&audio_pkt_queue, null_pkt); // NULL packet
            }
            break;
        }

        // Push packet to appropriate queue
        if (packet.stream_index == ctx->video_stream_index) {
            packet_queue_push(&video_pkt_queue, &packet);
        } else if (packet.stream_index == ctx->audio_stream_index) {
            packet_queue_push(&audio_pkt_queue, &packet);
        } else {
            av_packet_unref(&packet); // Discard packet for other streams
        }

        // --- Decode video packets and push frames to video_queue ---
        if (ctx->video_stream_index != -1) {
            AVPacket video_packet;
            // Pop a packet from the video packet queue (non-blocking)
            if (packet_queue_pop(&video_pkt_queue, &video_packet, 0, NULL) > 0) {
                 int send_packet_result = avcodec_send_packet(ctx->video_codec_context, &video_packet);
                 if (send_packet_result == 0) {
                     // Receive all possible frames from the decoder
                     int receive_frame_result;
                     while (!ctx->quit) {
                         receive_frame_result = avcodec_receive_frame(ctx->video_codec_context, ctx->video_frame);
                         if (receive_frame_result == 0) {
                             // Frame received, convert and push to frame queue
                             AVFrame *rgb_frame_copy = av_frame_alloc();
                             if (!rgb_frame_copy) {
                                 fprintf(stderr, "Decode thread: Error allocating RGB frame copy\n");
                                 av_frame_unref(ctx->video_frame);
                                 continue;
                             }
                             rgb_frame_copy->format = AV_PIX_FMT_RGB24;
                             rgb_frame_copy->width = ctx->width;
                             rgb_frame_copy->height = ctx->height;
                             if (av_frame_get_buffer(rgb_frame_copy, 0) < 0) {
                                 fprintf(stderr, "Decode thread: Error allocating buffer for RGB frame copy\n");
                                 av_frame_free(&rgb_frame_copy);
                                 av_frame_unref(ctx->video_frame);
                                 continue;
                             }

                             sws_scale(ctx->sws_context, (const uint8_t *const *)ctx->video_frame->data,
                                       ctx->video_frame->linesize, 0, ctx->video_codec_context->height,
                                       rgb_frame_copy->data, rgb_frame_copy->linesize);

                             double pts = (ctx->video_frame->pts == AV_NOPTS_VALUE) ?
                                          NAN : ctx->video_frame->pts * ctx->video_time_base;

                             frame_queue_push(&ctx->video_queue, rgb_frame_copy, pts, video_pkt_queue.serial);

                             av_frame_unref(ctx->video_frame); // Unreference the decoded frame
                         } else if (receive_frame_result == AVERROR(EAGAIN)) {
                             // Decoder needs more input, break receive loop and send next packet
                             break;
                         } else if (receive_frame_result == AVERROR_EOF) {
                              // End of stream for this decoder, break receive loop
                              break;
                         }
                         else {
                             fprintf(stderr, "Decode thread: Error receiving video frame: %s\n", av_err2str(receive_frame_result));
                             ctx->quit = 1; // Error, signal quit
                             break;
                         }
                     }
                 } else {
                      fprintf(stderr, "Decode thread: Error sending video packet: %s\n", av_err2str(send_packet_result));
                      ctx->quit = 1; // Error, signal quit
                 }
                 av_packet_unref(&video_packet);
            }
        }

        // --- Decode audio packets and push frames to audio_queue ---
        if (ctx->audio_stream_index != -1) {
            AVPacket audio_packet;
            // Pop a packet from the audio packet queue (non-blocking)
            if (packet_queue_pop(&audio_pkt_queue, &audio_packet, 0, NULL) > 0) {
                 int send_packet_result = avcodec_send_packet(ctx->audio_codec_context, &audio_packet);
                 if (send_packet_result == 0) {
                     // Receive all possible frames from the decoder
                     int receive_frame_result;
                     while (!ctx->quit) {
                         receive_frame_result = avcodec_receive_frame(ctx->audio_codec_context, ctx->audio_frame);
                         if (receive_frame_result == 0) {
                             // Frame received, push to audio frame queue
                             AVFrame *audio_frame_copy = av_frame_alloc();
                             if (!audio_frame_copy) {
                                 fprintf(stderr, "Decode thread: Error allocating audio frame copy\n");
                                 av_frame_unref(ctx->audio_frame);
                                 continue;
                             }
                             av_frame_move_ref(audio_frame_copy, ctx->audio_frame); // Move reference

                             double pts = (ctx->audio_frame->pts == AV_NOPTS_VALUE) ?
                                          NAN : ctx->audio_frame->pts * ctx->audio_time_base;

                             frame_queue_push(&ctx->audio_queue, audio_frame_copy, pts, audio_pkt_queue.serial);

                             av_frame_unref(ctx->audio_frame); // Unreference the decoded frame
                         } else if (receive_frame_result == AVERROR(EAGAIN)) {
                             // Decoder needs more input, break receive loop and send next packet
                             break;
                         } else if (receive_frame_result == AVERROR_EOF) {
                              // End of stream for this decoder, break receive loop
                              break;
                         }
                         else {
                             fprintf(stderr, "Decode thread: Error receiving audio frame: %s\n", av_err2str(receive_frame_result));
                             ctx->quit = 1; // Error, signal quit
                             break;
                         }
                     }
                 } else {
                      fprintf(stderr, "Decode thread: Error sending audio packet: %s\n", av_err2str(send_packet_result));
                      ctx->quit = 1; // Error, signal quit
                 }
                 av_packet_unref(&audio_packet);
            }
        }

        // Small delay to avoid busy-waiting
        SDL_Delay(1);
    }

    // Cleanup packet queues
    packet_queue_destroy(&video_pkt_queue);
    packet_queue_destroy(&audio_pkt_queue);

    fprintf(stderr, "Decode thread exiting\n");
    return 0;
}

// Function to initialize the media player
int init_media_player(MediaContext *ctx, const char *filepath) {
    memset(ctx, 0, sizeof(MediaContext)); // Initialize context struct

    ctx->filepath = filepath;
    ctx->video_stream_index = -1;
    ctx->audio_stream_index = -1;
    ctx->quit = 0;
    ctx->audio_clock = 0.0;
    ctx->audio_buffer = NULL;
    ctx->audio_buffer_size = 0;
    ctx->audio_buffer_index = 0;

    // Initialize FFmpeg
    avformat_network_init();

    // Open media file
    if (avformat_open_input(&ctx->format_context, filepath, NULL, NULL) != 0) {
        fprintf(stderr, "Error opening media file: %s\n", filepath);
        return -1;
    }

    // Find stream information
    if (avformat_find_stream_info(ctx->format_context, NULL) < 0) {
        fprintf(stderr, "Error finding stream information\n");
        return -1; // Cleanup will be handled by stop_media_player
    }

    // Find the first video and audio stream
    for (int i = 0; i < ctx->format_context->nb_streams; i++) {
        if (ctx->format_context->streams[i]->codecpar->codec_type ==
            AVMEDIA_TYPE_VIDEO) {
            if (ctx->video_stream_index == -1)
                ctx->video_stream_index = i;
        } else if (ctx->format_context->streams[i]->codecpar->codec_type ==
                   AVMEDIA_TYPE_AUDIO) {
            if (ctx->audio_stream_index == -1)
                ctx->audio_stream_index = i;
        }
    }

    if (ctx->video_stream_index == -1 && ctx->audio_stream_index == -1) {
        fprintf(stderr, "Error: No video or audio stream found\n");
        return -1; // Cleanup will be handled by stop_media_player
    }

    // Initialize video decoding components if video stream exists
    if (ctx->video_stream_index != -1) {
        AVCodecParameters *video_codecpar =
            ctx->format_context->streams[ctx->video_stream_index]->codecpar;
        const AVCodec *video_codec =
            avcodec_find_decoder(video_codecpar->codec_id);
        if (!video_codec) {
            fprintf(stderr, "Error: Video codec not found\n");
            return -1; // Cleanup will be handled by stop_media_player
        }

        ctx->video_codec_context = avcodec_alloc_context3(video_codec);
        if (!ctx->video_codec_context) {
            fprintf(stderr, "Error allocating video codec context\n");
            return -1; // Cleanup will be handled by stop_media_player
        }

        if (avcodec_parameters_to_context(ctx->video_codec_context,
                                            video_codecpar) < 0) {
            fprintf(stderr,
                    "Error copying video codec parameters to context\n");
            return -1; // Cleanup will be handled by stop_media_player
        }

        if (avcodec_open2(ctx->video_codec_context, video_codec, NULL) < 0) {
            fprintf(stderr, "Error opening video codec\n");
            return -1; // Cleanup will be handled by stop_media_player
        }

        ctx->video_frame = av_frame_alloc();
        if (!ctx->video_frame) {
            fprintf(stderr, "Error allocating video frame\n");
            return -1; // Cleanup will be handled by stop_media_player
        }

        ctx->width = ctx->video_codec_context->width;
        ctx->height = ctx->video_codec_context->height;

        ctx->rgb_frame = av_frame_alloc();
        if (!ctx->rgb_frame) {
            fprintf(stderr, "Error allocating RGB frame\n");
            return -1; // Cleanup will be handled by stop_media_player
        }

        ctx->sws_context = sws_getContext(
            ctx->video_codec_context->width, ctx->video_codec_context->height,
            ctx->video_codec_context->pix_fmt, ctx->width, ctx->height,
            AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
        if (!ctx->sws_context) {
            fprintf(stderr, "Error initializing SWS context\n");
            return -1; // Cleanup will be handled by stop_media_player
        }

        // Get video time base
        ctx->video_time_base = av_q2d(ctx->format_context->streams[ctx->video_stream_index]->time_base);

        // Initialize SDL for video
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
            fprintf(stderr, "SDL could not initialize video! SDL_Error: %s\n",
                    SDL_GetError());
            return -1; // Cleanup will be handled by stop_media_player
        }

        // Create window
        ctx->window = SDL_CreateWindow(
            "Media Player", SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED, ctx->width, ctx->height,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE); // Make window resizable
        if (ctx->window == NULL) {
            fprintf(stderr, "Window could not be created! SDL_Error: %s\n",
                    SDL_GetError());
            return -1; // Cleanup will be handled by stop_media_player
        }

        // Create renderer
        ctx->renderer = SDL_CreateRenderer(
            ctx->window, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (ctx->renderer == NULL) {
            fprintf(stderr,
                    "Renderer could not be created! SDL_Error: %s\n",
                    SDL_GetError());
            return -1; // Cleanup will be handled by stop_media_player
        }

        // Create texture
        ctx->texture = SDL_CreateTexture(
            ctx->renderer, SDL_PIXELFORMAT_RGB24,
            SDL_TEXTUREACCESS_STREAMING, ctx->width, ctx->height);
        if (ctx->texture == NULL) {
            fprintf(stderr,
                    "Texture could not be created! SDL_Error: %s\n",
                    SDL_GetError());
            return -1; // Cleanup will be handled by stop_media_player
        }
        SDL_SetRenderDrawColor(ctx->renderer, 0, 128, 64, 255);
        SDL_RenderClear(ctx->renderer);
        SDL_RenderPresent(ctx->renderer);

        // Initialize video frame queue
        frame_queue_init(&ctx->video_queue);
    }


    // Initialize audio decoding components if audio stream exists.
    if (ctx->audio_stream_index != -1) {
        AVCodecParameters *audio_codecpar =
            ctx->format_context->streams[ctx->audio_stream_index]->codecpar;
        const AVCodec *audio_codec =
            avcodec_find_decoder(audio_codecpar->codec_id);
        if (!audio_codec) {
            fprintf(stderr, "Could not find audio codec\n");
            return -1; // Cleanup will be handled by stop_media_player
        }

        ctx->audio_codec_context = avcodec_alloc_context3(audio_codec);
        if (!ctx->audio_codec_context) {
            fprintf(stderr, "Could not allocate audio codec context\n");
            return -1; // Cleanup will be handled by stop_media_player
        }
        if (avcodec_parameters_to_context(ctx->audio_codec_context,
                                            audio_codecpar) < 0) {
            fprintf(stderr,
                    "Error copying audio codec parameters to context\n");
            return -1; // Cleanup will be handled by stop_media_player
        }

        if (avcodec_open2(ctx->audio_codec_context, audio_codec, NULL) < 0) {
            fprintf(stderr, "Could not open audio codec\n");
            return -1; // Cleanup will be handled by stop_media_player
        }

        ctx->audio_frame = av_frame_alloc();
        if (!ctx->audio_frame) {
            fprintf(stderr, "Could not allocate audio frame\n");
            return -1; // Cleanup will be handled by stop_media_player
        }

        AVChannelLayout in_ch_layout;
        av_channel_layout_copy(&in_ch_layout, &audio_codecpar->ch_layout);

        AVChannelLayout out_ch_layout;
        av_channel_layout_default(&out_ch_layout, 2); // Stereo output

        if (!av_channel_layout_check(&out_ch_layout)) {
            fprintf(stderr, "Invalid default channel layout\n");
            return -1; // Cleanup will be handled by stop_media_player
        }

        ctx->swr_ctx = swr_alloc();
        av_opt_set_chlayout(ctx->swr_ctx, "in_chlayout", &in_ch_layout, 0);
        av_opt_set_chlayout(ctx->swr_ctx, "out_chlayout", &out_ch_layout, 0);
        av_opt_set_int(ctx->swr_ctx, "in_sample_rate",
                       audio_codecpar->sample_rate, 0);
        av_opt_set_int(ctx->swr_ctx, "out_sample_rate", 44100, 0); // Target sample rate
        av_opt_set_sample_fmt(ctx->swr_ctx, "in_sample_fmt",
                               audio_codecpar->format, 0);
        av_opt_set_sample_fmt(ctx->swr_ctx, "out_sample_fmt",
                               AV_SAMPLE_FMT_S16, 0); // Target sample format

        if (swr_init(ctx->swr_ctx) < 0) {
            fprintf(stderr, "Failed to initialize resampler\n");
            return -1; // Cleanup will be handled by stop_media_player
        }

        // Get audio time base
        ctx->audio_time_base = av_q2d(ctx->format_context->streams[ctx->audio_stream_index]->time_base);


        // Initialize SDL for audio
        // Note: SDL_Init(SDL_INIT_AUDIO) might have already been called if video exists.
        // SDL_Init is safe to call multiple times with the same flags.
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
            return -1; // Cleanup will be handled by stop_media_player
        }

        SDL_AudioSpec wanted_spec = {
            .freq = 44100,               // Target frequency
            .format = AUDIO_S16SYS,      // Target format
            .channels = 2,               // Target channels
            .samples = 1024,             // Buffer size (samples)
            .callback = audio_callback,  // Your callback function
            .userdata = ctx,             // Pass context to callback
        };

        ctx->audio_device_id =
            SDL_OpenAudioDevice(NULL, 0, &wanted_spec, NULL, 0);
        if (!ctx->audio_device_id) {
            fprintf(stderr, "Failed to open audio device: %s\n",
                    SDL_GetError());
            return -1; // Cleanup will be handled by stop_media_player
        }

        // Initialize audio clock mutex
        ctx->audio_clock_mutex = SDL_CreateMutex();
        if (!ctx->audio_clock_mutex) {
             fprintf(stderr, "Error creating audio clock mutex! SDL_Error: %s\n", SDL_GetError());
             return -1; // Cleanup will be handled by stop_media_player
        }

        // Initialize audio frame queue
        frame_queue_init(&ctx->audio_queue);

        SDL_PauseAudioDevice(ctx->audio_device_id, 0); // Start audio playback
    }

    // If both video and audio streams are present, initialize SDL with both subsystems
    // This block might be redundant if SDL_Init is called separately for video and audio,
    // but it ensures both are initialized if both streams are found.
    if (ctx->video_stream_index != -1 && ctx->audio_stream_index != -1) {
         if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
            fprintf(stderr, "SDL could not initialize video and audio! SDL_Error: %s\n",
                    SDL_GetError());
            return -1; // Cleanup will be handled by stop_media_player
        }
    }

    return 0; // Initialization successful
}

// Function to play the media
int play_media(MediaContext *ctx) {
    ctx->decode_thread =
        SDL_CreateThread(decode_thread_func, "DecodeThread", ctx);
    if (ctx->decode_thread == NULL) {
        fprintf(stderr, "Error creating decode thread: %s\n",
                SDL_GetError());
        return -1;
    }
#ifdef __WIIU__
    OSThread *native_handle = (OSThread *)SDL_GetThreadID(ctx->decode_thread);
    OSSetThreadRunQuantum(native_handle, 1000);
#endif

    SDL_GameController *pad = NULL;
    SDL_Event e;
    AVFrame *current_video_frame = NULL;
    double current_video_pts = NAN;
    int current_video_serial = -1;
    double frame_timer = 0.0; // Timer for video frame display
    double frame_delay = 0.04; // Initial guess for frame delay (25 fps)

    // Get the first video frame to initialize the frame_timer
    if (ctx->video_stream_index != -1) {
        current_video_frame = frame_queue_pop(&ctx->video_queue, &current_video_pts, &current_video_serial);
        if (current_video_frame) {
             frame_timer = av_gettime_relative() / 1000000.0;
        }
    }


    while (!ctx->quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                ctx->quit = SDL_TRUE;
            } else if (e.type == SDL_KEYDOWN ||
                       e.type == SDL_MOUSEBUTTONDOWN) {
                ctx->quit = SDL_TRUE;
            } else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
                ctx->quit = SDL_TRUE;
            } else if (e.type == SDL_CONTROLLERDEVICEADDED) {
                pad = SDL_GameControllerOpen(e.cdevice.which);
                printf("Added controller %d: %s\n", e.cdevice.which,
                       SDL_GameControllerName(pad));
            } else if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
                pad = SDL_GameControllerFromInstanceID(e.cdevice.which);
                printf("Removed controller: %s\n",
                       SDL_GameControllerName(pad));
                SDL_GameControllerClose(pad);
            } else if (e.type == SDL_WINDOWEVENT) {
                 if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                     // Handle window resize if needed (e.g., update renderer viewport)
                     // SDL_RenderSetViewport(ctx->renderer, NULL); // Reset viewport to full window
                 }
            }
        }

        // Video playback and synchronization
        if (ctx->video_stream_index != -1) {
            if (current_video_frame) {
                double current_time = av_gettime_relative() / 1000000.0;
                double time_to_display = frame_timer + frame_delay;
                double time_diff = time_to_display - current_time;

                // Get current audio clock (master clock)
                double audio_clock;
                SDL_LockMutex(ctx->audio_clock_mutex);
                audio_clock = ctx->audio_clock;
                SDL_UnlockMutex(ctx->audio_clock_mutex);

                // Calculate A-V difference
                double av_diff = current_video_pts - audio_clock;

                // Simple synchronization: drop frame if video is too far behind audio
                if (!isnan(av_diff) && av_diff < -0.1) { // If video is more than 100ms behind audio
                     fprintf(stderr, "Main loop: Video behind audio (diff: %.3f), dropping frame.\n", av_diff);
                     av_frame_free(&current_video_frame);
                     current_video_frame = frame_queue_pop(&ctx->video_queue, &current_video_pts, &current_video_serial);
                     if (!current_video_frame) {
                         // No more frames or timeout, check quit flag
                         if (ctx->quit) break;
                         SDL_Delay(1); // Small delay if queue is empty
                         continue;
                     }
                     // Update frame_timer for the dropped frame
                     frame_timer = current_time;
                     // Recalculate frame_delay based on next frame (basic estimation)
                     AVFrame *next_frame = frame_queue_peek(&ctx->video_queue, NULL, NULL);
                     if (next_frame && !isnan(current_video_pts)) {
                         double next_pts = NAN;
                         frame_queue_peek(&ctx->video_queue, &next_pts, NULL);
                         if (!isnan(next_pts)) {
                             frame_delay = next_pts - current_video_pts;
                         } else {
                             frame_delay = 0.04; // Default if next PTS is unknown
                         }
                     } else {
                          frame_delay = 0.04; // Default if no next frame
                     }
                     continue; // Skip rendering this dropped frame
                }


                if (time_diff <= 0) {
                    // Time to display the frame
                    SDL_UpdateTexture(ctx->texture, NULL, current_video_frame->data[0],
                                      current_video_frame->linesize[0]);

                    SDL_RenderClear(ctx->renderer);
                    SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, NULL);
                    SDL_RenderPresent(ctx->renderer);

                    // Free the displayed frame and get the next one
                    av_frame_free(&current_video_frame);
                    current_video_frame = frame_queue_pop(&ctx->video_queue, &current_video_pts, &current_video_serial);
                     if (!current_video_frame) {
                         // No more frames or timeout, check quit flag
                         if (ctx->quit) break;
                         SDL_Delay(1); // Small delay if queue is empty
                         continue;
                     }

                    // Update frame_timer for the next frame
                    frame_timer = current_time;

                    // Recalculate frame_delay based on the new current frame and the next frame (basic estimation)
                    AVFrame *next_frame = frame_queue_peek(&ctx->video_queue, NULL, NULL);
                    if (next_frame && !isnan(current_video_pts)) {
                        double next_pts = NAN;
                        frame_queue_peek(&ctx->video_queue, &next_pts, NULL);
                        if (!isnan(next_pts)) {
                            frame_delay = next_pts - current_video_pts;
                        } else {
                            frame_delay = 0.04; // Default if next PTS is unknown
                        }
                    } else {
                         frame_delay = 0.04; // Default if no next frame
                    }

                } else {
                    // Video is ahead, wait
                    SDL_Delay((int)(time_diff * 1000));
                }
            } else {
                 // No current video frame, try to pop one
                 current_video_frame = frame_queue_pop(&ctx->video_queue, &current_video_pts, &current_video_serial);
                 if (!current_video_frame) {
                     // Still no frame or timeout, check quit flag and wait briefly
                     if (ctx->quit) break;
                     SDL_Delay(1);
                 } else {
                     // Got a frame, initialize frame_timer
                     frame_timer = av_gettime_relative() / 1000000.0;
                     // Recalculate frame_delay based on the new current frame and the next frame (basic estimation)
                     AVFrame *next_frame = frame_queue_peek(&ctx->video_queue, NULL, NULL);
                     if (next_frame && !isnan(current_video_pts)) {
                         double next_pts = NAN;
                         frame_queue_peek(&ctx->video_queue, &next_pts, NULL);
                         if (!isnan(next_pts)) {
                             frame_delay = next_pts - current_video_pts;
                         } else {
                             frame_delay = 0.04; // Default if next PTS is unknown
                         }
                     } else {
                          frame_delay = 0.04; // Default if no next frame
                     }
                 }
            }
        } else {
             // No video stream, just process events and wait
             SDL_Delay(10);
        }
    }

    // Wait for the decode thread to finish
    if (ctx->decode_thread) {
        SDL_WaitThread(ctx->decode_thread, NULL);
        ctx->decode_thread = NULL;
    }

    // Free the last displayed video frame if it wasn't freed in the loop
    if (current_video_frame) {
        av_frame_free(&current_video_frame);
        current_video_frame = NULL;
    }

    return 0;
}

// Function to stop and cleanup the media player
void stop_media_player(MediaContext *ctx) {
    ctx->quit = 1; // Signal the decode thread to quit

    // Abort queues and wait for decode thread to finish
    if (ctx->video_stream_index != -1) {
        frame_queue_destroy(&ctx->video_queue);
    }
    if (ctx->audio_stream_index != -1) {
        frame_queue_destroy(&ctx->audio_queue);
    }

    // Wait for the decode thread to finish if it's running
    if (ctx->decode_thread) {
        SDL_WaitThread(ctx->decode_thread, NULL);
        ctx->decode_thread = NULL;
    }

    // Cleanup SDL resources
    if (ctx->texture) {
        SDL_DestroyTexture(ctx->texture);
        ctx->texture = NULL;
    }
    if (ctx->renderer) {
        SDL_DestroyRenderer(ctx->renderer);
        ctx->renderer = NULL;
    }
    if (ctx->window) {
        SDL_DestroyWindow(ctx->window);
        ctx->window = NULL;
    }

    // Cleanup FFmpeg resources
    if (ctx->sws_context) {
        sws_freeContext(ctx->sws_context);
        ctx->sws_context = NULL;
    }
    if (ctx->rgb_frame) {
        av_frame_free(&ctx->rgb_frame);
        ctx->rgb_frame = NULL;
    }
    if (ctx->video_frame) {
        av_frame_free(&ctx->video_frame);
        ctx->video_frame = NULL;
    }
    if (ctx->video_codec_context) {
        avcodec_free_context(&ctx->video_codec_context);
        ctx->video_codec_context = NULL;
    }

    if (ctx->audio_device_id) {
        SDL_CloseAudioDevice(ctx->audio_device_id);
        ctx->audio_device_id = 0;
    }
    if (ctx->swr_ctx) {
        swr_free(&ctx->swr_ctx);
        ctx->swr_ctx = NULL;
    }
    if (ctx->audio_codec_context) {
        avcodec_free_context(&ctx->audio_codec_context);
        ctx->audio_codec_context = NULL;
    }
    if (ctx->audio_buffer) {
        av_free(ctx->audio_buffer);
        ctx->audio_buffer = NULL;
        ctx->audio_buffer_size = 0;
        ctx->audio_buffer_index = 0;
    }
    if (ctx->audio_clock_mutex) {
        SDL_DestroyMutex(ctx->audio_clock_mutex);
        ctx->audio_clock_mutex = NULL;
    }


    if (ctx->format_context) {
        avformat_close_input(&ctx->format_context);
        ctx->format_context = NULL;
    }

    avformat_network_deinit();
    SDL_Quit();
}

#ifdef BUILD_AS_FUNCTION
int ffmpeg_play_media(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <media_file>\n", argv[0]);
        return 1;
    }

    MediaContext player_ctx;

    if (init_media_player(&player_ctx, argv[1]) != 0) {
        fprintf(stderr, "Failed to initialize media player\n");
        stop_media_player(&player_ctx); // Ensure cleanup on init failure
        return 1;
    }

    if (play_media(&player_ctx) != 0) {
        fprintf(stderr, "Failed to play media\n");
        stop_media_player(&player_ctx); // Ensure cleanup on play failure
        return 1;
    }

    stop_media_player(&player_ctx); // Ensure cleanup on normal exit
    printf("Media playback complete.\n");
    return 0;
}

