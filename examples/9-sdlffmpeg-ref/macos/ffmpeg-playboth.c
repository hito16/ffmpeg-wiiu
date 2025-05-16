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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __WIIU__
#include <coreinit/thread.h>
#endif

//#define AUDIO_BUFFER_SIZE 192000
#define AUDIO_BUFFER_SIZE 1920000

// Structure to hold video and audio player context
typedef struct {
    const char *filepath;
    AVFormatContext *format_context;
    int video_stream_index;
    AVCodecContext *video_codec_context;
    AVFrame *video_frame;
    AVFrame *rgb_frame;
    struct SwsContext *sws_context;
    int width;
    int height;
    double frame_rate;

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_mutex *frame_mutex;
    uint8_t *frame_buffer;
    int frame_buffer_size;
    int quit;
    SDL_Thread *decode_thread;

    // Audio related
    int audio_stream_index;
    AVCodecContext *audio_codec_context;
    AVFrame *audio_frame;
    SwrContext *swr_ctx;
    SDL_AudioDeviceID audio_device_id;
    SDL_mutex *audio_mutex;
    uint8_t audio_buffer[AUDIO_BUFFER_SIZE];
    int audio_buffer_size;
} MediaContext;

void audio_callback(void *userdata, Uint8 *stream, int len) {
    MediaContext *ctx = (MediaContext *)userdata;
    SDL_LockMutex(ctx->audio_mutex);
    int to_copy = (len > ctx->audio_buffer_size) ? ctx->audio_buffer_size : len;

    if (to_copy > 0) {
        memcpy(stream, ctx->audio_buffer, to_copy);
        memmove(ctx->audio_buffer, ctx->audio_buffer + to_copy,
                ctx->audio_buffer_size - to_copy);
        ctx->audio_buffer_size -= to_copy;
    }

    if (to_copy < len) {
        memset(stream + to_copy, 0, len - to_copy);  // fill with silence
    }
    SDL_UnlockMutex(ctx->audio_mutex);
}

static int decode_thread_func(void *arg) {
    MediaContext *ctx = (MediaContext *)arg;
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        fprintf(stderr, "Error allocating AVPacket\n");
        ctx->quit = 1;
        return -1;
    }

    int printrate = 2000;
    long frames = 0;

    while (!ctx->quit) {
        if (frames % printrate == 0)
            printf(" d ? av_read_frame\n"); // Removed unnecessary newline
        if (av_read_frame(ctx->format_context, packet) < 0) {
            printf(" d ? av_read_frame - end of stream or error\n");
            break; // End of stream or error
        }

        if (packet->stream_index == ctx->video_stream_index) {
            if (frames % printrate == 0)
                printf(" d < avcodec_send_packet (video)\n");
            avcodec_send_packet(ctx->video_codec_context, packet);
            if (frames % printrate == 0)
                printf(" d > avcodec_receive_frame (video)\n");
            while (avcodec_receive_frame(ctx->video_codec_context,
                                         ctx->video_frame) == 0) {
                if (frames % printrate == 0)
                    printf(" d sws_scale\n");
                // Convert frame to RGB
                sws_scale(
                    ctx->sws_context,
                    (const uint8_t *const *)ctx->video_frame->data,
                    ctx->video_frame->linesize, 0,
                    ctx->video_codec_context->height, ctx->rgb_frame->data,
                    ctx->rgb_frame->linesize);

                if (frames % printrate == 0)
                    printf(" d SDL_LockMutex\n");
                SDL_LockMutex(ctx->frame_mutex);
                if (frames % printrate == 0)
                    printf(" d memcpy(ctx->frame_buffer...\n");
                memcpy(ctx->frame_buffer, ctx->rgb_frame->data[0],
                       ctx->frame_buffer_size);
                if (frames % printrate == 0)
                    printf(" d SDL_UnlockMutex\n");
                SDL_UnlockMutex(ctx->frame_mutex);

                // Introduce a delay based on the frame rate
                if (ctx->frame_rate > 0) {
                    if (frames % printrate == 0)
                        printf(" d SDL_Delay\n");
                    SDL_Delay((int)(1000.0 / ctx->frame_rate));
                }
            }
        } else if (packet->stream_index == ctx->audio_stream_index) {
            if (avcodec_send_packet(ctx->audio_codec_context, packet) == 0) {
                while (avcodec_receive_frame(ctx->audio_codec_context,
                                             ctx->audio_frame) == 0) {
                    uint8_t *out_buf;
                    int out_linesize;
                    int out_samples = av_rescale_rnd(
                        swr_get_delay(ctx->swr_ctx,
                                     ctx->audio_codec_context->sample_rate) +
                            ctx->audio_frame->nb_samples,
                        44100, ctx->audio_codec_context->sample_rate,
                        AV_ROUND_UP);

                    av_samples_alloc(&out_buf, &out_linesize, 2, out_samples,
                                     AV_SAMPLE_FMT_S16, 0);
                    int converted =
                        swr_convert(ctx->swr_ctx, &out_buf, out_samples,
                                    (const uint8_t **)ctx->audio_frame->extended_data,
                                    ctx->audio_frame->nb_samples);
                    int size = av_samples_get_buffer_size(
                        NULL, 2, converted, AV_SAMPLE_FMT_S16, 1);

                    int written = 0;
                    while (written < size) {
                        SDL_LockMutex(ctx->audio_mutex);
                        int space_left =
                            AUDIO_BUFFER_SIZE - ctx->audio_buffer_size;
                        int to_write =
                            (size - written < space_left)
                                ? (size - written)
                                : space_left;

                        if (to_write > 0) {
                            memcpy(ctx->audio_buffer + ctx->audio_buffer_size,
                                   out_buf + written, to_write);
                            ctx->audio_buffer_size += to_write;
                            written += to_write;
                        }
                        SDL_UnlockMutex(ctx->audio_mutex);

                        if (written < size) {
                            SDL_Delay(1); // Wait for buffer to drain
                        }
                    }
                    av_freep(&out_buf);
                }
            }
        }

        av_packet_unref(packet);
        if (frames % printrate == 0)
            printf("decoding thread frames=%ld\n", frames);
        frames++;
        SDL_Delay(1);
    }

    printf("decoder thread exiting\n");
    av_packet_free(&packet);
    return 0;
}

// Function to initialize the media player
int init_media_player(MediaContext *ctx, const char *filepath) {
    ctx->filepath = filepath;
    ctx->format_context = NULL;
    ctx->video_stream_index = -1;
    ctx->video_codec_context = NULL;
    ctx->video_frame = NULL;
    ctx->rgb_frame = NULL;
    ctx->sws_context = NULL;
    ctx->window = NULL;
    ctx->renderer = NULL;
    ctx->texture = NULL;
    ctx->frame_mutex = NULL;
    ctx->frame_buffer = NULL;
    ctx->frame_buffer_size = 0;
    ctx->quit = 0;
    ctx->width = 0;
    ctx->height = 0;
    ctx->frame_rate = 0.0;
    ctx->decode_thread = NULL;

    ctx->audio_stream_index = -1;
    ctx->audio_codec_context = NULL;
    ctx->audio_frame = NULL;
    ctx->swr_ctx = NULL;
    ctx->audio_device_id = 0;
    ctx->audio_mutex = NULL;
    ctx->audio_buffer_size = 0;

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
        avformat_close_input(&ctx->format_context);
        return -1;
    }

    // Find the first video and audio stream
    for (int i = 0; i < ctx->format_context->nb_streams; i++) {
        if (ctx->format_context->streams[i]->codecpar->codec_type ==
            AVMEDIA_TYPE_VIDEO) {
            if (ctx->video_stream_index == -1)
                ctx->video_stream_index = i; //find first video stream
        } else if (ctx->format_context->streams[i]->codecpar->codec_type ==
                   AVMEDIA_TYPE_AUDIO) {
            if (ctx->audio_stream_index == -1)
                ctx->audio_stream_index = i; // find first audio stream
        }
    }

    if (ctx->video_stream_index == -1 && ctx->audio_stream_index == -1) {
        fprintf(stderr, "Error: No video or audio stream found\n");
        avformat_close_input(&ctx->format_context);
        return -1;
    }

    // Initialize video decoding components if video stream exists
    if (ctx->video_stream_index != -1) {
        AVCodecParameters *video_codecpar =
            ctx->format_context
                ->streams[ctx->video_stream_index]
                ->codecpar; // corrected variable name
        const AVCodec *video_codec =
            avcodec_find_decoder(video_codecpar->codec_id);
        if (!video_codec) {
            fprintf(stderr, "Error: Video codec not found\n");
            avformat_close_input(&ctx->format_context);
            return -1;
        }

        ctx->video_codec_context = avcodec_alloc_context3(video_codec);
        if (!ctx->video_codec_context) {
            fprintf(stderr, "Error allocating video codec context\n");
            avformat_close_input(&ctx->format_context);
            return -1;
        }

        if (avcodec_parameters_to_context(ctx->video_codec_context,
                                            video_codecpar) < 0) {
            fprintf(stderr,
                    "Error copying video codec parameters to context\n");
            avcodec_free_context(&ctx->video_codec_context);
            avformat_close_input(&ctx->format_context);
            return -1;
        }

        if (avcodec_open2(ctx->video_codec_context, video_codec, NULL) < 0) {
            fprintf(stderr, "Error opening video codec\n");
            avcodec_free_context(&ctx->video_codec_context);
            avformat_close_input(&ctx->format_context);
            return -1;
        }

        ctx->video_frame = av_frame_alloc();
        if (!ctx->video_frame) {
            fprintf(stderr, "Error allocating video frame\n");
            avcodec_free_context(&ctx->video_codec_context);
            avformat_close_input(&ctx->format_context);
            return -1;
        }

        ctx->width = ctx->video_codec_context->width;
        ctx->height = ctx->video_codec_context->height;

        ctx->rgb_frame = av_frame_alloc();
        if (!ctx->rgb_frame) {
            fprintf(stderr, "Error allocating RGB frame\n");
            av_frame_free(&ctx->video_frame);
            avcodec_free_context(&ctx->video_codec_context);
            avformat_close_input(&ctx->format_context);
            return -1;
        }

        ctx->frame_buffer_size = av_image_get_buffer_size(
            AV_PIX_FMT_RGB24, ctx->width, ctx->height, 1);
        ctx->frame_buffer = (uint8_t *)av_malloc(ctx->frame_buffer_size);
        if (!ctx->frame_buffer) {
            fprintf(stderr, "Error allocating frame buffer\n");
            av_frame_free(&ctx->rgb_frame);
            av_frame_free(&ctx->video_frame);
            avcodec_free_context(&ctx->video_codec_context);
            avformat_close_input(&ctx->format_context);
            return -1;
        }

        av_image_fill_arrays(ctx->rgb_frame->data, ctx->rgb_frame->linesize,
                             ctx->frame_buffer, AV_PIX_FMT_RGB24, ctx->width,
                             ctx->height, 1);

        ctx->sws_context = sws_getContext(
            ctx->video_codec_context->width, ctx->video_codec_context->height,
            ctx->video_codec_context->pix_fmt, ctx->width, ctx->height,
            AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
        if (!ctx->sws_context) {
            fprintf(stderr, "Error initializing SWS context\n");
            av_free(ctx->frame_buffer);
            av_frame_free(&ctx->rgb_frame);
            av_frame_free(&ctx->video_frame);
            avcodec_free_context(&ctx->video_codec_context);
            avformat_close_input(&ctx->format_context);
            return -1;
        }

        // Get frame rate if available
        if (ctx->format_context->streams[ctx->video_stream_index]
                ->r_frame_rate.num &&
            ctx->format_context->streams[ctx->video_stream_index]
                ->r_frame_rate.den) {
            ctx->frame_rate =
                av_q2d(ctx->format_context
                           ->streams[ctx->video_stream_index]
                           ->r_frame_rate);
        } else if (ctx->format_context->streams[ctx->video_stream_index]
                       ->avg_frame_rate.num &&
                   ctx->format_context->streams[ctx->video_stream_index]
                       ->avg_frame_rate.den) {
            ctx->frame_rate =
                av_q2d(ctx->format_context
                           ->streams[ctx->video_stream_index]
                           ->avg_frame_rate);
        } else {
            ctx->frame_rate = 30.0; // Default frame rate
        }
    }

    // Initialize audio decoding components if audio stream exists.
    if (ctx->audio_stream_index != -1) {
        AVCodecParameters *audio_codecpar =
            ctx->format_context
                ->streams[ctx->audio_stream_index]
                ->codecpar; // Corrected variable name
        const AVCodec *audio_codec =
            avcodec_find_decoder(audio_codecpar->codec_id);
        if (!audio_codec) {
            fprintf(stderr, "Could not find audio codec\n");
            if (ctx->sws_context)
                sws_freeContext(ctx->sws_context);
            if (ctx->frame_buffer)
                av_free(ctx->frame_buffer);
            if (ctx->rgb_frame)
                av_frame_free(&ctx->rgb_frame);
            if (ctx->video_frame)
                av_frame_free(&ctx->video_frame);
            if (ctx->video_codec_context)
                avcodec_free_context(&ctx->video_codec_context);
            avformat_close_input(&ctx->format_context);
            return -1;
        }

        ctx->audio_codec_context = avcodec_alloc_context3(audio_codec);
        if (!ctx->audio_codec_context) {
            fprintf(stderr, "Could not allocate audio codec context\n");
            if (ctx->sws_context)
                sws_freeContext(ctx->sws_context);
            if (ctx->frame_buffer)
                av_free(ctx->frame_buffer);
            if (ctx->rgb_frame)
                av_frame_free(&ctx->rgb_frame);
            if (ctx->video_frame)
                av_frame_free(&ctx->video_frame);
            if (ctx->video_codec_context)
                avcodec_free_context(&ctx->video_codec_context);
            avformat_close_input(&ctx->format_context);
            return -1;
        }
        if (avcodec_parameters_to_context(ctx->audio_codec_context,
                                            audio_codecpar) < 0) {
            fprintf(stderr,
                    "Error copying audio codec parameters to context\n");
            if (ctx->sws_context)
                sws_freeContext(ctx->sws_context);
            if (ctx->frame_buffer)
                av_free(ctx->frame_buffer);
            if (ctx->rgb_frame)
                av_frame_free(&ctx->rgb_frame);
            if (ctx->video_frame)
                av_frame_free(&ctx->video_frame);
            avcodec_free_context(&ctx->video_codec_context);
            avformat_close_input(&ctx->format_context);
            return -1;
        }

        if (avcodec_open2(ctx->audio_codec_context, audio_codec, NULL) < 0) {
            fprintf(stderr, "Could not open audio codec\n");
            if (ctx->sws_context)
                sws_freeContext(ctx->sws_context);
            if (ctx->frame_buffer)
                av_free(ctx->frame_buffer);
            if (ctx->rgb_frame)
                av_frame_free(&ctx->rgb_frame);
            if (ctx->video_frame)
                av_frame_free(&ctx->video_frame);
            avcodec_free_context(&ctx->video_codec_context);
            avformat_close_input(&ctx->format_context);
            return -1;
        }

        ctx->audio_frame = av_frame_alloc();
        if (!ctx->audio_frame) {
            fprintf(stderr, "Could not allocate audio frame\n");
            if (ctx->sws_context)
                sws_freeContext(ctx->sws_context);
            if (ctx->frame_buffer)
                av_free(ctx->frame_buffer);
            if (ctx->rgb_frame)
                av_frame_free(&ctx->rgb_frame);
            if (ctx->video_frame)
                av_frame_free(&ctx->video_frame);
            avcodec_free_context(&ctx->video_codec_context);
            avformat_close_input(&ctx->format_context);
            return -1;
        }

        AVChannelLayout in_ch_layout;
        av_channel_layout_copy(&in_ch_layout, &audio_codecpar->ch_layout);

        AVChannelLayout out_ch_layout;
        av_channel_layout_default(&out_ch_layout, 2);

        if (!av_channel_layout_check(&out_ch_layout)) {
            fprintf(stderr, "Invalid default channel layout\n");
            if (ctx->sws_context)
                sws_freeContext(ctx->sws_context);
            if (ctx->frame_buffer)
                av_free(ctx->frame_buffer);
            if (ctx->rgb_frame)
                av_frame_free(&ctx->rgb_frame);
            if (ctx->video_frame)
                av_frame_free(&ctx->video_frame);
            avcodec_free_context(&ctx->video_codec_context);
            avformat_close_input(&ctx->format_context);
            return -1;
        }

        ctx->swr_ctx = swr_alloc();
        av_opt_set_chlayout(ctx->swr_ctx, "in_chlayout", &in_ch_layout, 0);
        av_opt_set_chlayout(ctx->swr_ctx, "out_chlayout", &out_ch_layout, 0);
        av_opt_set_int(ctx->swr_ctx, "in_sample_rate",
                       audio_codecpar->sample_rate, 0);
        av_opt_set_int(ctx->swr_ctx, "out_sample_rate", 44100, 0);
        av_opt_set_sample_fmt(ctx->swr_ctx, "in_sample_fmt",
                               audio_codecpar->format, 0);
        av_opt_set_sample_fmt(ctx->swr_ctx, "out_sample_fmt",
                               AV_SAMPLE_FMT_S16, 0);

        if (swr_init(ctx->swr_ctx) < 0) {
            fprintf(stderr, "Failed to initialize resampler\n");
            if (ctx->sws_context)
                sws_freeContext(ctx->sws_context);
            if (ctx->frame_buffer)
                av_free(ctx->frame_buffer);
            if (ctx->rgb_frame)
                av_frame_free(&ctx->rgb_frame);
            if (ctx->video_frame)
                av_frame_free(&ctx->video_frame);
            avcodec_free_context(&ctx->video_codec_context);
            avcodec_free_context(&ctx->audio_codec_context);
            avformat_close_input(&ctx->format_context);
            return -1;
        }

        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
            if (ctx->sws_context)
                sws_freeContext(ctx->sws_context);
            if (ctx->frame_buffer)
                av_free(ctx->frame_buffer);
            if (ctx->rgb_frame)
                av_frame_free(&ctx->rgb_frame);
            if (ctx->video_frame)
                av_frame_free(&ctx->video_frame);
            avcodec_free_context(&ctx->video_codec_context);
            avcodec_free_context(&ctx->audio_codec_context);
            avformat_close_input(&ctx->format_context);
            SDL_Quit();
            return -1;
        }

        SDL_AudioSpec wanted_spec = {
            .freq = 44100,               //
            .format = AUDIO_S16SYS,      //
            .channels = 2,               //
            .samples = 1024,             // .samples = 1024,   
            .callback = audio_callback,  //
            .userdata = ctx,
        };

        ctx->audio_device_id =
            SDL_OpenAudioDevice(NULL, 0, &wanted_spec, NULL, 0);
        if (!ctx->audio_device_id) {
            fprintf(stderr, "Failed to open audio device: %s\n",
                    SDL_GetError());
            if (ctx->sws_context)
                sws_freeContext(ctx->sws_context);
            if (ctx->frame_buffer)
                av_free(ctx->frame_buffer);
            if (ctx->rgb_frame)
                av_frame_free(&ctx->rgb_frame);
            if (ctx->video_frame)
                av_frame_free(&ctx->video_frame);
            avcodec_free_context(&ctx->video_codec_context);
            avcodec_free_context(&ctx->audio_codec_context);
            avformat_close_input(&ctx->format_context);
            SDL_Quit();
            return -1;
        }
        ctx->audio_mutex = SDL_CreateMutex();
        if (!ctx->audio_mutex) {
            fprintf(stderr, "Error creating audio mutex! SDL_Error: %s\n",
                    SDL_GetError());
            SDL_CloseAudioDevice(ctx->audio_device_id);
            if (ctx->sws_context)
                sws_freeContext(ctx->sws_context);
            if (ctx->frame_buffer)
                av_free(ctx->frame_buffer);
            if (ctx->rgb_frame)
                av_frame_free(&ctx->rgb_frame);
            if (ctx->video_frame)
                av_frame_free(&ctx->video_frame);
            avcodec_free_context(&ctx->video_codec_context);
            avcodec_free_context(&ctx->audio_codec_context);
            avformat_close_input(&ctx->format_context);
            SDL_Quit();
            return -1;
        }
        SDL_PauseAudioDevice(ctx->audio_device_id, 0);
    }

    //Initialize video
    if (ctx->video_stream_index != -1) {
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
            fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n",
                    SDL_GetError());
            if (ctx->sws_context)
                sws_freeContext(ctx->sws_context);
            if (ctx->frame_buffer)
                av_free(ctx->frame_buffer);
            if (ctx->rgb_frame)
                av_frame_free(&ctx->rgb_frame);
            if (ctx->video_frame)
                av_frame_free(&ctx->video_frame);
            avcodec_free_context(&ctx->video_codec_context);
            avformat_close_input(&ctx->format_context);
            SDL_Quit();
            return -1;
        }

        // Create window
        ctx->window = SDL_CreateWindow(
            "Media Player", SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED, ctx->width, ctx->height,
            SDL_WINDOW_SHOWN);
        if (ctx->window == NULL) {
            fprintf(stderr, "Window could not be created! SDL_Error: %s\n",
                    SDL_GetError());
            if (ctx->sws_context)
                sws_freeContext(ctx->sws_context);
            if (ctx->frame_buffer)
                av_free(ctx->frame_buffer);
            if (ctx->rgb_frame)
                av_frame_free(&ctx->rgb_frame);
            if (ctx->video_frame)
                av_frame_free(&ctx->video_frame);
            avcodec_free_context(&ctx->video_codec_context);
            avformat_close_input(&ctx->format_context);
            SDL_Quit();
            return -1;
        }

        // Create renderer
        ctx->renderer = SDL_CreateRenderer(
            ctx->window, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (ctx->renderer == NULL) {
            fprintf(stderr,
                    "Renderer could not be created! SDL_Error: %s\n",
                    SDL_GetError());
            SDL_DestroyWindow(ctx->window);
            if (ctx->sws_context)
                sws_freeContext(ctx->sws_context);
            if (ctx->frame_buffer)
                av_free(ctx->frame_buffer);
            if (ctx->rgb_frame)
                av_frame_free(&ctx->rgb_frame);
            if (ctx->video_frame)
                av_frame_free(&ctx->video_frame);
            avcodec_free_context(&ctx->video_codec_context);
            avformat_close_input(&ctx->format_context);
            SDL_Quit();
            return -1;
        }

        // Create texture
        ctx->texture = SDL_CreateTexture(
            ctx->renderer, SDL_PIXELFORMAT_RGB24,
            SDL_TEXTUREACCESS_STREAMING, ctx->width, ctx->height);
        if (ctx->texture == NULL) {
            fprintf(stderr,
                    "Texture could not be created! SDL_Error: %s\n",
                    SDL_GetError());
            SDL_DestroyRenderer(ctx->renderer);
            SDL_DestroyWindow(ctx->window);
            if (ctx->sws_context)
                sws_freeContext(ctx->sws_context);
            if (ctx->frame_buffer)
                av_free(ctx->frame_buffer);
            if (ctx->rgb_frame)
                av_frame_free(&ctx->rgb_frame);
            if (ctx->video_frame)
                av_frame_free(&ctx->video_frame);
            avcodec_free_context(&ctx->video_codec_context);
            avformat_close_input(&ctx->format_context);
            SDL_Quit();
            return -1;
        }
        SDL_SetRenderDrawColor(ctx->renderer, 0, 128, 64, 255);
        SDL_RenderClear(ctx->renderer);
        SDL_RenderPresent(ctx->renderer);

        // Create mutex for frame buffer access
        ctx->frame_mutex = SDL_CreateMutex();
        if (!ctx->frame_mutex) {
            fprintf(stderr, "Error creating mutex! SDL_Error: %s\n",
                    SDL_GetError());
            SDL_DestroyTexture(ctx->texture);
            SDL_DestroyRenderer(ctx->renderer);
            SDL_DestroyWindow(ctx->window);
            if (ctx->sws_context)
                sws_freeContext(ctx->sws_context);
            if (ctx->frame_buffer)
                av_free(ctx->frame_buffer);
            if (ctx->rgb_frame)
                av_frame_free(&ctx->rgb_frame);
            if (ctx->video_frame)
                av_frame_free(&ctx->video_frame);
            avcodec_free_context(&ctx->video_codec_context);
            avformat_close_input(&ctx->format_context);
            SDL_Quit();
            return -1;
        }
    }
    return 0;
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

    long frames = 0;
    int printrate = 2000;
    SDL_GameController *pad;
    SDL_Event e;
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
            }
        }
        if (ctx->video_stream_index != -1) {
            SDL_LockMutex(ctx->frame_mutex);
            if (frames % printrate == 0)
                printf("UpdateTexture frame %ld\n", frames);
            SDL_UpdateTexture(ctx->texture, NULL, ctx->frame_buffer,
                              ctx->width * 3);
            SDL_UnlockMutex(ctx->frame_mutex);

            SDL_RenderClear(ctx->renderer);
            if (frames % printrate == 0)
                printf("RenderCopy %ld\n", frames);
            SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, NULL);
            if (frames % printrate == 0)
                printf("RenderPresent %ld\n", frames);
            SDL_RenderPresent(ctx->renderer);
        }
        SDL_Delay(1);
        frames++;
    }

    SDL_WaitThread(ctx->decode_thread, NULL);
    ctx->decode_thread = NULL;
    return 0;
}

// Function to stop and cleanup the media player
void stop_media_player(MediaContext *ctx) {
    ctx->quit = 1;

    // Wait for the decode thread to finish if it's running
    if (ctx->decode_thread) {
        SDL_WaitThread(ctx->decode_thread, NULL);
        ctx->decode_thread = NULL;
    }
    if (ctx->frame_mutex) {
        SDL_DestroyMutex(ctx->frame_mutex);
    }
    if (ctx->texture) {
        SDL_DestroyTexture(ctx->texture);
    }
    if (ctx->renderer) {
        SDL_DestroyRenderer(ctx->renderer);
    }
    if (ctx->window) {
        SDL_DestroyWindow(ctx->window);
    }
    if (ctx->sws_context) {
        sws_freeContext(ctx->sws_context);
    }
    if (ctx->frame_buffer) {
        av_free(ctx->frame_buffer);
    }
    if (ctx->rgb_frame) {
        av_frame_free(&ctx->rgb_frame);
    }
    if (ctx->video_frame) {
        av_frame_free(&ctx->video_frame);
    }
    if (ctx->video_codec_context) {
        avcodec_free_context(&ctx->video_codec_context);
    }

    if (ctx->audio_mutex) {
        SDL_DestroyMutex(ctx->audio_mutex);
    }
    if (ctx->audio_device_id) {
        SDL_CloseAudioDevice(ctx->audio_device_id);
    }
    if (ctx->swr_ctx) {
        swr_free(&ctx->swr_ctx);
    }
    if (ctx->audio_codec_context) {
        avcodec_free_context(&ctx->audio_codec_context);
    }

    if (ctx->format_context) {
        avformat_close_input(&ctx->format_context);
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
        return 1;
    }

    if (play_media(&player_ctx) != 0) {
        fprintf(stderr, "Failed to play media\n");
        stop_media_player(&player_ctx);
        return 1;
    }

    stop_media_player(&player_ctx);
    printf("Media playback complete.\n");
    return 0;
}
