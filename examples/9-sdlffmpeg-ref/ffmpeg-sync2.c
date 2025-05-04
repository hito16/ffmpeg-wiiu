#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Required FFmpeg headers
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>  // For new AVChannelLayout API
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>  // Explicitly include for AVSampleFormat
#include <libavutil/timestamp.h>  // For AV_TIME_BASE_Q
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

// --- Application Context Structure ---
typedef struct AppContext {
    AVFormatContext *fmt_ctx;
    AVCodecContext *video_dec_ctx;
    AVCodecContext *audio_dec_ctx;
    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;

    int video_stream_idx;
    int audio_stream_idx;

    // Target video frame (RGB24)
    AVFrame *rgb_frame;
    uint8_t *rgb_buffer;

    // Target audio frame (resampled)
    AVFrame *resampled_audio_frame;
    uint8_t **resampled_audio_data;
    int resampled_audio_buf_size;
    int resampled_audio_linesize;
    AVChannelLayout target_audio_ch_layout;  // Store target layout

    // Synchronization State
    double audio_clock;  // Current audio playback time in seconds
    AVFrame
        *held_video_frame;  // Holds the *converted* RGB frame if video is early
    double held_video_pts_sec;  // PTS of the held video frame
    int video_frame_held;       // Flag indicating if a video frame is held

} AppContext;

// --- Forward Declarations ---
int open_media_file(AppContext *ctx, const char *filename);
int prepare_decoders_and_conversion(AppContext *ctx);
int initialize_decoder(AVCodecContext **dec_ctx_ptr, const AVCodec **codec_ptr,
                       AVStream *stream);
int decode_and_process_frame(AppContext *ctx, AVPacket *pkt, int flushing);
void playit(AppContext *ctx, AVFrame *video_frame_rgb,
            AVFrame *audio_frame_resampled, double video_pts_sec,
            double audio_pts_sec);
void cleanup(AppContext *ctx);

// --- Playback Function (User Provided Logic) ---
/**
 * @brief Placeholder function to simulate playing synchronized frames.
 *
 * @param ctx Application context (read-only access if needed).
 * @param video_frame_rgb The video frame to display (converted to RGB24). NULL
 * if no video frame ready now.
 * @param audio_frame_resampled The audio frame to play (resampled). NULL if no
 * audio frame ready now. NOTE: The actual audio data is in
 * ctx->resampled_audio_data.
 * @param video_pts_sec Presentation time of the video frame in seconds.
 * Negative if no video frame.
 * @param audio_pts_sec Presentation time of the audio frame in seconds.
 * Negative if no audio frame.
 */
void playit(AppContext *ctx, AVFrame *video_frame_rgb,
            AVFrame *audio_frame_resampled, double video_pts_sec,
            double audio_pts_sec) {
    // --- THIS IS WHERE YOUR PLAYBACK/RENDERING LOGIC GOES ---
    // Example: Use SDL, OpenGL, PortAudio, etc.

    printf("playit: ");
    if (video_frame_rgb) {
        // Access RGB data via video_frame_rgb->data[0] and
        // video_frame_rgb->linesize[0] Dimensions: ctx->video_dec_ctx->width,
        // ctx->video_dec_ctx->height
        printf("VID PTS: %8.3f sec | ", video_pts_sec);
    } else {
        printf("VID PTS:    N/A    | ");
    }

    if (audio_frame_resampled) {
        // Access resampled audio data via ctx->resampled_audio_data[0],
        // ctx->resampled_audio_data[1]... Format: audio_frame_resampled->format
        // (e.g., AV_SAMPLE_FMT_S16) Sample Rate:
        // audio_frame_resampled->sample_rate Channels:
        // audio_frame_resampled->ch_layout.nb_channels Samples per channel:
        // audio_frame_resampled->nb_samples
        printf("AUD PTS: %8.3f sec (%d samples @ %d Hz)", audio_pts_sec,
               audio_frame_resampled->nb_samples,
               audio_frame_resampled->sample_rate);
    } else {
        printf("AUD PTS:    N/A");
    }
    printf("\n");

    // --- Update Audio Clock (Simplified) ---
    // In a real player, the audio device callback provides a more accurate
    // clock. This simple version sets the clock to the start time of the audio
    // frame being sent.
    if (audio_frame_resampled) {
        ctx->audio_clock = audio_pts_sec;
        //  A slightly better estimation might be end time:
        //  ctx->audio_clock = audio_pts_sec +
        //  (double)audio_frame_resampled->nb_samples /
        //  audio_frame_resampled->sample_rate;
        // printf("       Audio clock updated to: %f\n", ctx->audio_clock);
    }
}

// --- FFmpeg Initialization and Processing Functions ---

/**
 * @brief Opens the media file and finds the best audio/video streams.
 */
int open_media_file(AppContext *ctx, const char *filename) {
    int ret;
    ctx->fmt_ctx = NULL;  // Ensure it's NULL for avformat_open_input

    // Open input file and read header
    if ((ret = avformat_open_input(&ctx->fmt_ctx, filename, NULL, NULL)) < 0) {
        fprintf(stderr, "ERROR: Cannot open input file '%s': %d\n", filename,
                ret);
        return ret;
    }

    // Retrieve stream information
    if ((ret = avformat_find_stream_info(ctx->fmt_ctx, NULL)) < 0) {
        fprintf(stderr, "ERROR: Cannot find stream information: %d\n", ret);
        // No need to close input here, cleanup() will handle it if fmt_ctx is
        // non-NULL
        return ret;
    }

    // Find the best video and audio streams
    ctx->video_stream_idx =
        av_find_best_stream(ctx->fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    ctx->audio_stream_idx =
        av_find_best_stream(ctx->fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);

    if (ctx->video_stream_idx < 0) {
        fprintf(stderr, "ERROR: Cannot find video stream in file '%s'\n",
                filename);
        // Allow proceeding without video, maybe? For this example, treat as
        // error.
        return AVERROR_STREAM_NOT_FOUND;
    }
    if (ctx->audio_stream_idx < 0) {
        fprintf(stderr, "ERROR: Cannot find audio stream in file '%s'\n",
                filename);
        return AVERROR_STREAM_NOT_FOUND;
    }

    printf("Found video stream index: %d\n", ctx->video_stream_idx);
    printf("Found audio stream index: %d\n", ctx->audio_stream_idx);

    // Print detailed information about the input file format
    av_dump_format(ctx->fmt_ctx, 0, filename, 0);

    return 0;  // Success
}

/**
 * @brief Initializes a decoder context for a given stream.
 */
int initialize_decoder(AVCodecContext **dec_ctx_ptr, const AVCodec **codec_ptr,
                       AVStream *stream) {
    int ret;
    const AVCodec *codec;
    AVCodecContext *codec_ctx;

    // Find decoder for the stream
    codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        fprintf(stderr, "ERROR: Failed to find codec for stream #%u (ID %d)\n",
                stream->index, stream->codecpar->codec_id);
        return AVERROR_DECODER_NOT_FOUND;
    }
    *codec_ptr = codec;  // Return found codec if needed elsewhere

    // Allocate a codec context for the decoder
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        fprintf(stderr, "ERROR: Failed to allocate the %s codec context\n",
                av_get_media_type_string(codec->type));
        return AVERROR(ENOMEM);
    }

    // Copy codec parameters from input stream to output codec context
    if ((ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar)) <
        0) {
        fprintf(stderr,
                "ERROR: Failed to copy %s codec parameters to decoder context: "
                "%d\n",
                av_get_media_type_string(codec->type), ret);
        avcodec_free_context(&codec_ctx);  // Free allocated context on error
        return ret;
    }

    // Use multiple threads for decoding if available
    // codec_ctx->thread_count = 0; // 0 = auto
    // codec_ctx->thread_type = FF_THREAD_FRAME; // Or FF_THREAD_SLICE

    // Init the decoders, with or without reference counting
    // AVDictionary *opts = NULL;
    // av_dict_set(&opts, "refcounted_frames", "1", 0); // Default usually okay
    if ((ret = avcodec_open2(codec_ctx, codec, NULL)) < 0) {
        fprintf(stderr, "ERROR: Failed to open %s codec: %d\n",
                av_get_media_type_string(codec->type), ret);
        avcodec_free_context(&codec_ctx);  // Free allocated context on error
        return ret;
    }

    *dec_ctx_ptr = codec_ctx;
    printf("Initialized %s decoder: %s\n",
           av_get_media_type_string(codec->type), codec->name);
    return 0;  // Success
}

/**
 * @brief Sets up decoders, video scaler (SWS), and audio resampler (SWR).
 */
int prepare_decoders_and_conversion(AppContext *ctx) {
    int ret;
    const AVCodec *video_codec = NULL;
    const AVCodec *audio_codec = NULL;
    AVStream *video_stream = ctx->fmt_ctx->streams[ctx->video_stream_idx];
    AVStream *audio_stream = ctx->fmt_ctx->streams[ctx->audio_stream_idx];

    // --- Initialize Decoders ---
    if ((ret = initialize_decoder(&ctx->video_dec_ctx, &video_codec,
                                  video_stream)) < 0) {
        return ret;
    }
    if ((ret = initialize_decoder(&ctx->audio_dec_ctx, &audio_codec,
                                  audio_stream)) < 0) {
        return ret;
    }

    // --- Prepare Video Conversion (to RGB24) ---
    ctx->rgb_frame = av_frame_alloc();
    if (!ctx->rgb_frame) return AVERROR(ENOMEM);

    int width = ctx->video_dec_ctx->width;
    int height = ctx->video_dec_ctx->height;
    enum AVPixelFormat pix_fmt = ctx->video_dec_ctx->pix_fmt;
    enum AVPixelFormat target_pix_fmt =
        AV_PIX_FMT_RGB24;  // Common display format

    // Allocate buffer for the RGB frame
    // Use av_image_alloc for proper alignment and buffer management
    ret = av_image_alloc(ctx->rgb_frame->data, ctx->rgb_frame->linesize, width,
                         height, target_pix_fmt, 1);  // Alignment 1
    if (ret < 0) {
        fprintf(stderr, "ERROR: Could not allocate RGB video buffer: %d\n",
                ret);
        av_frame_free(&ctx->rgb_frame);  // Free the frame struct itself
        return ret;
    }
    // Store the buffer pointer separately for freeing with
    // av_freep(&ctx->rgb_buffer)
    ctx->rgb_buffer = ctx->rgb_frame->data[0];

    // Get SWS context for scaling/conversion
    ctx->sws_ctx = sws_getContext(width, height, pix_fmt,           // Input
                                  width, height, target_pix_fmt,    // Output
                                  SWS_BILINEAR, NULL, NULL, NULL);  // Flags
    if (!ctx->sws_ctx) {
        fprintf(stderr,
                "ERROR: Failed to get SwsContext for video conversion\n");
        // No need to free rgb_buffer explicitly here, cleanup() handles it
        return AVERROR(EINVAL);
    }
    ctx->rgb_frame->format = target_pix_fmt;
    ctx->rgb_frame->width = width;
    ctx->rgb_frame->height = height;
    printf("Prepared video scaling context (%s -> %s)\n",
           av_get_pix_fmt_name(pix_fmt), av_get_pix_fmt_name(target_pix_fmt));

    // --- Prepare Audio Resampling (Example: to Signed 16-bit Stereo) ---
    ctx->resampled_audio_frame = av_frame_alloc();
    if (!ctx->resampled_audio_frame) return AVERROR(ENOMEM);

    // Define target audio parameters (adjust as needed for your output device)
    enum AVSampleFormat target_sample_fmt =
        AV_SAMPLE_FMT_S16;  // Signed 16-bit integer
    int target_sample_rate =
        ctx->audio_dec_ctx->sample_rate;  // Or choose a fixed rate like 44100
    // Use the new AVChannelLayout API
    // Get default layout for 2 channels (Stereo)
    av_channel_layout_default(&ctx->target_audio_ch_layout, 2);
    if (ret < 0) {  // Note: ret from av_channel_layout_default is void, this
                    // check is for previous error. Removed as per fix 1.
        // fprintf(stderr, "ERROR: Failed to get default stereo channel layout:
        // %s\n", av_err(ret)); return ret; // cleanup() handles freeing other
        // things
    }

    ctx->swr_ctx = swr_alloc();
    if (!ctx->swr_ctx) {
        fprintf(stderr, "ERROR: Could not allocate SwrContext\n");
        return AVERROR(ENOMEM);
    }

    // Set SwrContext options using AVOptions and the new channel layout API
    av_opt_set_chlayout(ctx->swr_ctx, "in_chlayout",
                        &ctx->audio_dec_ctx->ch_layout, 0);
    av_opt_set_int(ctx->swr_ctx, "in_sample_rate",
                   ctx->audio_dec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(ctx->swr_ctx, "in_sample_fmt",
                          ctx->audio_dec_ctx->sample_fmt, 0);

    av_opt_set_chlayout(ctx->swr_ctx, "out_chlayout",
                        &ctx->target_audio_ch_layout, 0);
    av_opt_set_int(ctx->swr_ctx, "out_sample_rate", target_sample_rate, 0);
    av_opt_set_sample_fmt(ctx->swr_ctx, "out_sample_fmt", target_sample_fmt, 0);

    // Initialize the resampling context
    if ((ret = swr_init(ctx->swr_ctx)) < 0) {
        fprintf(stderr,
                "ERROR: Failed to initialize the resampling context: %d\n",
                ret);
        // cleanup() handles swr_free, channel_layout_uninit etc.
        return ret;
    }

    // Setup the target audio frame parameters
    ctx->resampled_audio_frame->sample_rate = target_sample_rate;
    ctx->resampled_audio_frame->format = target_sample_fmt;
    // Copy the channel layout struct (important!)
    av_channel_layout_copy(&ctx->resampled_audio_frame->ch_layout,
                           &ctx->target_audio_ch_layout);

    printf("Prepared audio resampling context (%s/%dHz/%s -> %s/%dHz/%s)\n",
           av_get_sample_fmt_name(ctx->audio_dec_ctx->sample_fmt),
           ctx->audio_dec_ctx->sample_rate,
           "layout",  // TODO: Print input layout nicely
           av_get_sample_fmt_name(target_sample_fmt), target_sample_rate,
           "stereo");  // Assuming stereo target

    return 0;  // Success
}

/**
 * @brief Decodes a packet, processes frames (convert/resample), synchronizes,
 * and calls playit.
 * @param ctx App context
 * @param pkt Packet to decode (NULL if flushing)
 * @param flushing 1 if flushing, 0 otherwise
 * @return 0 on success, < 0 on error, 1 on EOF reached during flushing
 */
int decode_and_process_frame(AppContext *ctx, AVPacket *pkt, int flushing) {
    int ret = 0;
    AVCodecContext *current_dec_ctx = NULL;
    AVFrame *decoded_frame = NULL;
    int stream_index = -1;
    int got_output = 0;

    if (!flushing) {
        stream_index = pkt->stream_index;
        if (stream_index == ctx->video_stream_idx) {
            current_dec_ctx = ctx->video_dec_ctx;
        } else if (stream_index == ctx->audio_stream_idx) {
            current_dec_ctx = ctx->audio_dec_ctx;
        } else {
            return 0;  // Ignore packets from unknown streams
        }
    } else {
        // When flushing, we need to try both decoders
        // This logic iterates through decoders when pkt is NULL
        if (ctx->video_dec_ctx)
            current_dec_ctx = ctx->video_dec_ctx;
        else if (ctx->audio_dec_ctx)
            current_dec_ctx = ctx->audio_dec_ctx;
        else
            return 1;  // No decoders to flush
        stream_index = (current_dec_ctx == ctx->video_dec_ctx)
                           ? ctx->video_stream_idx
                           : ctx->audio_stream_idx;
        printf("Flushing %s decoder...\n",
               (stream_index == ctx->video_stream_idx) ? "video" : "audio");
    }

    // Send packet (or NULL for flushing) to the decoder
    ret = avcodec_send_packet(current_dec_ctx, pkt);
    if (ret < 0 &&
        ret !=
            AVERROR_EOF) {  // Ignore EOF when sending NULL packet for flushing
        if (ret == AVERROR(EAGAIN)) {
            // Decoder full, needs receive_frame first. This shouldn't happen
            // often if receive loop below is correct, but handle defensively.
            fprintf(stderr,
                    "WARN: Decoder full (EAGAIN), trying receive first.\n");
            // Fall through to receive loop
        } else {
            fprintf(stderr, "ERROR sending packet for decoding stream %d: %d\n",
                    stream_index, ret);
            return ret;
        }
    }

    // Receive frames from the decoder
    while (1) {
        decoded_frame = av_frame_alloc();
        if (!decoded_frame) return AVERROR(ENOMEM);

        ret = avcodec_receive_frame(current_dec_ctx, decoded_frame);

        if (ret == AVERROR(EAGAIN)) {
            // Decoder needs more input
            av_frame_free(&decoded_frame);
            break;  // Exit receive loop, need more packets (or flushing is done
                    // for this pkt)
        } else if (ret == AVERROR_EOF) {
            // End of stream for this decoder (flushing complete or end of file)
            av_frame_free(&decoded_frame);
            // If flushing, mark this decoder as done and try the other one next
            // time
            if (flushing) {
                if (current_dec_ctx == ctx->video_dec_ctx)
                    ctx->video_dec_ctx = NULL;
                else if (current_dec_ctx == ctx->audio_dec_ctx)
                    ctx->audio_dec_ctx = NULL;
            }
            // If we got EOF after sending packets, break normally.
            // If we got EOF during flushing, signal completion for this
            // decoder.
            got_output = flushing ? 1 : 0;  // Signal EOF if flushing
            break;                          // Exit receive loop
        } else if (ret < 0) {
            fprintf(stderr, "ERROR during decoding stream %d: %d\n",
                    stream_index, ret);
            av_frame_free(&decoded_frame);
            return ret;  // Serious decoding error
        }

        // --- Frame successfully decoded ---
        got_output = 1;
        double pts_sec = 0;
        AVStream *stream = ctx->fmt_ctx->streams[stream_index];

        if (decoded_frame->pts != AV_NOPTS_VALUE) {
            pts_sec = (double)decoded_frame->pts * av_q2d(stream->time_base);
        } else {
            // Try best effort timestamp if PTS is missing
            int64_t best_effort_ts = decoded_frame->best_effort_timestamp;
            if (best_effort_ts != AV_NOPTS_VALUE) {
                pts_sec = (double)best_effort_ts * av_q2d(stream->time_base);
            } else {
                // Fallback: Use audio clock - very inaccurate!
                pts_sec = ctx->audio_clock;
                fprintf(stderr,
                        "WARN: Frame (stream %d) lacks PTS, using audio clock "
                        "%f as fallback.\n",
                        stream_index, pts_sec);
            }
        }

        if (stream_index == ctx->video_stream_idx) {
            // --- Process Video Frame ---
            // Convert video frame to RGB format using sws_scale
            sws_scale(ctx->sws_ctx, (const uint8_t *const *)decoded_frame->data,
                      decoded_frame->linesize, 0, ctx->video_dec_ctx->height,
                      ctx->rgb_frame->data, ctx->rgb_frame->linesize);

            // Synchronization: Check if this video frame should be displayed
            if (ctx->video_frame_held) {
                // If we are already holding a frame, this new one is probably
                // too early too. A real player would queue frames.
                // Simplification: discard older held frame.
                printf(
                    "WARN: Discarding held video frame (PTS %f) to process new "
                    "one (PTS %f)\n",
                    ctx->held_video_pts_sec, pts_sec);
                // No need to unref ctx->held_video_frame, it's just a pointer
                // to ctx->rgb_frame
            }

            // Compare video PTS to audio clock
            if (pts_sec <= ctx->audio_clock || ctx->audio_clock == 0.0) {
                // Video is on time or audio hasn't started, play it
                playit(ctx, ctx->rgb_frame, NULL, pts_sec, -1.0);
                ctx->video_frame_held = 0;  // Ensure not marked as held
            } else {
                // Video is too early, hold this frame (its data is already in
                // rgb_frame)
                ctx->held_video_pts_sec = pts_sec;
                ctx->video_frame_held = 1;
                // printf("       Holding video frame (PTS %f > clock %f)\n",
                // pts_sec, ctx->audio_clock);
            }

        } else if (stream_index == ctx->audio_stream_idx) {
            // --- Process Audio Frame ---
            // Resample audio frame using swr_convert
            int max_out_samples = (int)av_rescale_rnd(
                swr_get_delay(ctx->swr_ctx, decoded_frame->sample_rate) +
                    decoded_frame->nb_samples,
                ctx->resampled_audio_frame->sample_rate,
                decoded_frame->sample_rate, AV_ROUND_UP);

            // Reallocate buffer for resampled data if necessary
            ret = av_samples_alloc_array_and_samples(
                &ctx->resampled_audio_data,      // Ptr to data pointers
                &ctx->resampled_audio_linesize,  // Ptr to linesize
                ctx->resampled_audio_frame->ch_layout
                    .nb_channels,  // Number of channels from target frame
                                   // layout
                max_out_samples,   // Max samples per channel
                ctx->resampled_audio_frame
                    ->format,  // Target format from frame
                0);            // Alignment (0 = default)

            if (ret < 0) {
                fprintf(stderr,
                        "ERROR: Could not allocate resampled audio buffer\n");
                // No need to free decoded_frame here, done below
            } else {
                ctx->resampled_audio_buf_size =
                    ret;  // Store actual allocated size

                // Perform resampling
                int actual_out_samples = swr_convert(
                    ctx->swr_ctx, ctx->resampled_audio_data, max_out_samples,
                    (const uint8_t **)decoded_frame->data,
                    decoded_frame->nb_samples);

                if (actual_out_samples < 0) {
                    fprintf(stderr, "ERROR while converting audio: %d\n",
                            actual_out_samples);
                } else if (actual_out_samples > 0) {
                    // Set number of samples in the output frame struct
                    ctx->resampled_audio_frame->nb_samples = actual_out_samples;

                    // Play the resampled audio (updates audio clock inside
                    // playit)
                    playit(ctx, NULL, ctx->resampled_audio_frame, -1.0,
                           pts_sec);

                    // Free the resampled data buffer immediately after playit
                    // (playit should copy the data if it needs it
                    // asynchronously)
                    av_freep(
                        &ctx->resampled_audio_data[0]);  // Free the buffer
                                                         // allocated by
                                                         // av_samples_alloc...
                    av_freep(&ctx->resampled_audio_data);  // Free the array of
                                                           // pointers
                    ctx->resampled_audio_data = NULL;
                    ctx->resampled_audio_buf_size = 0;

                    // --- Check if held video frame can now be played ---
                    if (ctx->video_frame_held &&
                        ctx->held_video_pts_sec <= ctx->audio_clock) {
                        // printf("       Playing held video frame (PTS %f <=
                        // clock %f)\n", ctx->held_video_pts_sec,
                        // ctx->audio_clock);
                        playit(ctx, ctx->rgb_frame, NULL,
                               ctx->held_video_pts_sec, -1.0);
                        ctx->video_frame_held = 0;  // Mark as played
                    }

                } else {
                    // 0 samples output, free buffer anyway
                    av_freep(&ctx->resampled_audio_data[0]);
                    av_freep(&ctx->resampled_audio_data);
                    ctx->resampled_audio_data = NULL;
                    ctx->resampled_audio_buf_size = 0;
                }
            }  // End buffer allocation check
        }  // End audio stream check

        // Free the original decoded frame, we are done with it
        av_frame_free(&decoded_frame);

    }  // End while receive frame loop

    // If flushing, determine if both decoders are done
    if (flushing && !ctx->video_dec_ctx && !ctx->audio_dec_ctx) {
        return 1;  // Signal that flushing is fully complete
    }

    return got_output ? 0
                      : (flushing ? 1 : 0);  // 0 if got output, 1 if flushing
                                             // finished, 0 otherwise
}

/**
 * @brief Frees all allocated FFmpeg resources and context data.
 */
void cleanup(AppContext *ctx) {
    printf("Cleaning up resources...\n");

    // Free synchronization state resources
    // (held_video_frame just points to rgb_frame's data, no separate free
    // needed)

    // Free conversion/resampling resources
    if (ctx->resampled_audio_data) {  // Check if buffer was allocated and maybe
                                      // not freed in loop
        av_freep(&ctx->resampled_audio_data[0]);
        av_freep(&ctx->resampled_audio_data);
    }
    av_frame_free(&ctx->resampled_audio_frame);
    swr_free(&ctx->swr_ctx);
    av_channel_layout_uninit(
        &ctx->target_audio_ch_layout);  // Uninit layout from
                                        // av_channel_layout_default

    if (ctx->rgb_buffer) {  // Free the buffer allocated by av_image_alloc
        av_freep(&ctx->rgb_buffer);  // Use av_freep for buffers from
                                     // av_malloc/av_image_alloc
        // Note: rgb_frame->data[0] pointed to this buffer, don't double free.
    }
    av_frame_free(&ctx->rgb_frame);
    sws_freeContext(ctx->sws_ctx);

    // Free decoder contexts
    avcodec_free_context(&ctx->video_dec_ctx);
    avcodec_free_context(&ctx->audio_dec_ctx);

    // Close input file
    avformat_close_input(&ctx->fmt_ctx);  // Handles NULL check internally

    // Zero out the context struct (optional, good practice)
    memset(ctx, 0, sizeof(AppContext));
}

// --- Main Function ---
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    const char *filename = argv[1];
    AppContext app_ctx = {0};  // Initialize context struct to zero
    AVPacket *pkt = NULL;
    int ret;

    // --- Initialization Phase ---
    ret = open_media_file(&app_ctx, filename);
    if (ret < 0) {
        cleanup(&app_ctx);
        return 1;
    }

    ret = prepare_decoders_and_conversion(&app_ctx);
    if (ret < 0) {
        cleanup(&app_ctx);
        return 1;
    }

    pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "ERROR: Failed to allocate packet\n");
        cleanup(&app_ctx);
        return 1;
    }

    // --- Main Decoding Loop ---
    printf("\nStarting decoding loop...\n");
    while (av_read_frame(app_ctx.fmt_ctx, pkt) >= 0) {
        ret = decode_and_process_frame(&app_ctx, pkt, 0);
        av_packet_unref(pkt);  // Important: Release packet reference
        if (ret < 0) {
            fprintf(stderr, "ERROR during decoding/processing. Aborting.\n");
            break;  // Exit loop on critical error
        }
    }
    printf("\nEnd of file reached or read error.\n");

    // --- Flushing Phase ---
    printf("Flushing remaining frames...\n");
    // Send NULL packet to each decoder to flush
    // decode_and_process_frame handles the NULL pkt logic internally when
    // flushing=1
    while (app_ctx.video_dec_ctx ||
           app_ctx.audio_dec_ctx) {  // Loop until both decoders are flushed
        ret = decode_and_process_frame(&app_ctx, NULL, 1);
        if (ret < 0) {
            fprintf(stderr, "ERROR during flushing.\n");
            break;
        } else if (ret == 1) {
            // This signals flushing completed for *one* decoder in this call,
            // loop continues if the other decoder still exists.
            // If both are NULL, the loop condition ends it.
        }
    }

    printf("Flushing complete.\n");

    // --- Cleanup Phase ---
    av_packet_free(&pkt);
    cleanup(&app_ctx);

    printf("Playback finished.\n");
    return 0;
}
