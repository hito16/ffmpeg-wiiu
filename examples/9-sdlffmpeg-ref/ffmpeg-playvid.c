/* Based on a similar freezing experinece with ffplay,
 * I tried to simplify to a single thread (video only)
 *
 * on WiiU-
 *   Currently this decodes several frames, renders them and then
 *   appears to hang at around the 7th call to RenderPresent.
 *
 * on MacOS
 *   No issues.
 */

#include <SDL.h>
#include <SDL_render.h>
#include <SDL_thread.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __WIIU__
#include <coreinit/thread.h>
#endif

// Structure to hold video player context
typedef struct {
    const char *filepath;
    AVFormatContext *format_context;
    int video_stream_index;
    AVCodecContext *video_codec_context;
    AVFrame *frame;
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
} VideoPlayerContext;

// Decoding thread function
// static void *decode_thread(void *arg) {
static int decode_thread_func(void *arg) {
    VideoPlayerContext *ctx = (VideoPlayerContext *)arg;
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        fprintf(stderr, "Error allocating AVPacket\n");
        ctx->quit = 1;
        return 1;
        // return NULL;
    }
    int printrate = 2000;
    long frames = 0;
    while (!ctx->quit) {
        if (frames % printrate == 0) printf(" d ? av_read_frame\n");
        if (av_read_frame(ctx->format_context, packet) < 0) {
            printf(" d ? av_read_frame - end of stream or error\n");
            break;  // End of stream or error
        }

        if (packet->stream_index == ctx->video_stream_index) {
            if (frames % printrate == 0) printf(" d < avcodec_send_packet\n");
            avcodec_send_packet(ctx->video_codec_context, packet);
            if (frames % printrate == 0) printf(" d > avcodec_receive_frame\n");
            while (avcodec_receive_frame(ctx->video_codec_context,
                                         ctx->frame) == 0) {
                if (frames % printrate == 0) printf(" d sws_scale\n");
                // Convert frame to RGB
                sws_scale(
                    ctx->sws_context, (const uint8_t *const *)ctx->frame->data,
                    ctx->frame->linesize, 0, ctx->video_codec_context->height,
                    ctx->rgb_frame->data, ctx->rgb_frame->linesize);

                if (frames % printrate == 0) printf(" d SDL_LockMutex\n");
                SDL_LockMutex(ctx->frame_mutex);
                if (frames % printrate == 0)
                    printf(" d memcpy(ctx->frame_buffer...\n");
                memcpy(ctx->frame_buffer, ctx->rgb_frame->data[0],
                       ctx->frame_buffer_size);
                if (frames % printrate == 0) printf(" d SDL_UnlockMutex\n");
                SDL_UnlockMutex(ctx->frame_mutex);

                // Introduce a delay based on the frame rate
                if (ctx->frame_rate > 0) {
                    if (frames % printrate == 0) printf(" d SDL_Delay\n");
                    // usleep((int)(1000000.0 / ctx->frame_rate));
                    SDL_Delay((int)(1000.0 / ctx->frame_rate));
                }
            }
        }

        av_packet_unref(packet);
        // to slow down stdout.
        if (frames % printrate == 0)
            printf("decoding thread frames=%ld\n", frames);
        frames++;
        SDL_Delay(1);
    }

    printf("decoder thread exiting\n");
    av_packet_free(&packet);
    // return NULL;
    return 0;
}

// Function to initialize the video player
int init_video_player(VideoPlayerContext *ctx, const char *filepath) {
    ctx->filepath = filepath;
    ctx->format_context = NULL;
    ctx->video_stream_index = -1;
    ctx->video_codec_context = NULL;
    ctx->frame = NULL;
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

    // Initialize FFmpeg
    // av_register_all();
    avformat_network_init();

    // Open video file
    if (avformat_open_input(&ctx->format_context, filepath, NULL, NULL) != 0) {
        fprintf(stderr, "Error opening video file: %s\n", filepath);
        return -1;
    }

    printf("avformat_find_stream_info\n");
    // Find video stream information
    if (avformat_find_stream_info(ctx->format_context, NULL) < 0) {
        fprintf(stderr, "Error finding stream information\n");
        avformat_close_input(&ctx->format_context);
        return -1;
    }

    // Find the first video stream
    for (int i = 0; i < ctx->format_context->nb_streams; i++) {
        if (ctx->format_context->streams[i]->codecpar->codec_type ==
            AVMEDIA_TYPE_VIDEO) {
            ctx->video_stream_index = i;
            break;
        }
    }

    if (ctx->video_stream_index == -1) {
        fprintf(stderr, "Error: No video stream found\n");
        avformat_close_input(&ctx->format_context);
        return -1;
    }

    // Get the codec parameters for the video stream
    AVCodecParameters *codec_params =
        ctx->format_context->streams[ctx->video_stream_index]->codecpar;

    // Find the video decoder
    const AVCodec *video_codec = avcodec_find_decoder(codec_params->codec_id);
    if (!video_codec) {
        fprintf(stderr, "Error: Video codec not found\n");
        avformat_close_input(&ctx->format_context);
        return -1;
    }

    // Allocate codec context
    ctx->video_codec_context = avcodec_alloc_context3(video_codec);
    if (!ctx->video_codec_context) {
        fprintf(stderr, "Error allocating video codec context\n");
        avformat_close_input(&ctx->format_context);
        return -1;
    }

    // Copy codec parameters to the context
    if (avcodec_parameters_to_context(ctx->video_codec_context, codec_params) <
        0) {
        fprintf(stderr, "Error copying codec parameters to context\n");
        avcodec_free_context(&ctx->video_codec_context);
        avformat_close_input(&ctx->format_context);
        return -1;
    }

    // Open codec
    if (avcodec_open2(ctx->video_codec_context, video_codec, NULL) < 0) {
        fprintf(stderr, "Error opening video codec\n");
        avcodec_free_context(&ctx->video_codec_context);
        avformat_close_input(&ctx->format_context);
        return -1;
    }

    printf("av_frame_alloc()\n");
    // Allocate video frames
    ctx->frame = av_frame_alloc();
    if (!ctx->frame) {
        fprintf(stderr, "Error allocating frame\n");
        // avcodec_close(ctx->video_codec_context);
        avcodec_free_context(&ctx->video_codec_context);
        avformat_close_input(&ctx->format_context);
        return -1;
    }

    ctx->width = ctx->video_codec_context->width;
    ctx->height = ctx->video_codec_context->height;

    printf("av_frame_alloc() rgb_frame\n");
    // Allocate RGB frame
    ctx->rgb_frame = av_frame_alloc();
    if (!ctx->rgb_frame) {
        fprintf(stderr, "Error allocating RGB frame\n");
        av_frame_free(&ctx->frame);
        // avcodec_close(ctx->video_codec_context);
        avcodec_free_context(&ctx->video_codec_context);
        avformat_close_input(&ctx->format_context);
        return -1;
    }

    printf("av_image_get_buffer_size() \n");
    ctx->frame_buffer_size =
        av_image_get_buffer_size(AV_PIX_FMT_RGB24, ctx->width, ctx->height, 1);
    ctx->frame_buffer = (uint8_t *)av_malloc(ctx->frame_buffer_size);
    if (!ctx->frame_buffer) {
        fprintf(stderr, "Error allocating frame buffer\n");
        av_frame_free(&ctx->rgb_frame);
        av_frame_free(&ctx->frame);
        // avcodec_close(ctx->video_codec_context);
        avcodec_free_context(&ctx->video_codec_context);
        avformat_close_input(&ctx->format_context);
        return -1;
    }
    printf("av_image_fill_arrays() \n");
    av_image_fill_arrays(ctx->rgb_frame->data, ctx->rgb_frame->linesize,
                         ctx->frame_buffer, AV_PIX_FMT_RGB24, ctx->width,
                         ctx->height, 1);

    printf("sws_getContext() \n");
    // Initialize SWS context for conversion
    ctx->sws_context = sws_getContext(
        ctx->video_codec_context->width, ctx->video_codec_context->height,
        ctx->video_codec_context->pix_fmt, ctx->width, ctx->height,
        AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
    if (!ctx->sws_context) {
        fprintf(stderr, "Error initializing SWS context\n");
        av_free(ctx->frame_buffer);
        av_frame_free(&ctx->rgb_frame);
        av_frame_free(&ctx->frame);
        // avcodec_close(ctx->video_codec_context);
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
            av_q2d(ctx->format_context->streams[ctx->video_stream_index]
                       ->r_frame_rate);
    } else if (ctx->format_context->streams[ctx->video_stream_index]
                   ->avg_frame_rate.num &&
               ctx->format_context->streams[ctx->video_stream_index]
                   ->avg_frame_rate.den) {
        ctx->frame_rate =
            av_q2d(ctx->format_context->streams[ctx->video_stream_index]
                       ->avg_frame_rate);
    } else {
        ctx->frame_rate = 30.0;  // Default frame rate
    }

    // Initialize SDL
    // if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n",
                SDL_GetError());
        sws_freeContext(ctx->sws_context);
        av_free(ctx->frame_buffer);
        av_frame_free(&ctx->rgb_frame);
        av_frame_free(&ctx->frame);
        // avcodec_close(ctx->video_codec_context);
        avcodec_free_context(&ctx->video_codec_context);
        avformat_close_input(&ctx->format_context);
        return -1;
    }

    // Create window
    ctx->window = SDL_CreateWindow("Video Player", SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED, ctx->width,
                                   ctx->height, SDL_WINDOW_SHOWN);
    if (ctx->window == NULL) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n",
                SDL_GetError());
        sws_freeContext(ctx->sws_context);
        av_free(ctx->frame_buffer);
        av_frame_free(&ctx->rgb_frame);
        av_frame_free(&ctx->frame);
        // avcodec_close(ctx->video_codec_context);
        avcodec_free_context(&ctx->video_codec_context);
        avformat_close_input(&ctx->format_context);
        SDL_Quit();
        return -1;
    }

    // Create renderer
    ctx->renderer = SDL_CreateRenderer(
        ctx->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ctx->renderer == NULL) {
        fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n",
                SDL_GetError());
        SDL_DestroyWindow(ctx->window);
        sws_freeContext(ctx->sws_context);
        av_free(ctx->frame_buffer);
        av_frame_free(&ctx->rgb_frame);
        av_frame_free(&ctx->frame);
        // avcodec_close(ctx->video_codec_context);
        avcodec_free_context(&ctx->video_codec_context);
        avformat_close_input(&ctx->format_context);
        SDL_Quit();
        return -1;
    }

    printf("SDL_CreateTexture\n");
    // Create texture
    ctx->texture =
        SDL_CreateTexture(ctx->renderer, SDL_PIXELFORMAT_RGB24,
                          SDL_TEXTUREACCESS_STREAMING, ctx->width, ctx->height);
    if (ctx->texture == NULL) {
        fprintf(stderr, "Texture could not be created! SDL_Error: %s\n",
                SDL_GetError());
        SDL_DestroyRenderer(ctx->renderer);
        SDL_DestroyWindow(ctx->window);
        sws_freeContext(ctx->sws_context);
        av_free(ctx->frame_buffer);
        av_frame_free(&ctx->rgb_frame);
        av_frame_free(&ctx->frame);
        // avcodec_close(ctx->video_codec_context);
        avcodec_free_context(&ctx->video_codec_context);
        avformat_close_input(&ctx->format_context);
        SDL_Quit();
        return -1;
    }
    SDL_SetRenderDrawColor(ctx->renderer, 0, 128, 64, 255);
    SDL_RenderClear(ctx->renderer);
    SDL_RenderPresent(ctx->renderer);

    printf("SDL_CreateMutex\n");
    // Create mutex for frame buffer access
    ctx->frame_mutex = SDL_CreateMutex();
    if (!ctx->frame_mutex) {
        fprintf(stderr, "Error creating mutex! SDL_Error: %s\n",
                SDL_GetError());
        SDL_DestroyTexture(ctx->texture);
        SDL_DestroyRenderer(ctx->renderer);
        SDL_DestroyWindow(ctx->window);
        sws_freeContext(ctx->sws_context);
        av_free(ctx->frame_buffer);
        av_frame_free(&ctx->rgb_frame);
        av_frame_free(&ctx->frame);
        // avcodec_close(ctx->video_codec_context);
        avcodec_free_context(&ctx->video_codec_context);
        avformat_close_input(&ctx->format_context);
        SDL_Quit();
        return -1;
    }

    return 0;
}

// Function to play the video
int play_video(VideoPlayerContext *ctx) {
    // pthread_t decode_thread_id;
    // if (pthread_create(&decode_thread_id, NULL, decode_thread, ctx) != 0) {
    //     fprintf(stderr, "Error creating decode thread\n");
    printf("SDL_CreateThread\n");
    ctx->decode_thread =
        SDL_CreateThread(decode_thread_func, "DecodeThread", ctx);
    if (ctx->decode_thread == NULL) {
        fprintf(stderr, "Error creating decode thread: %s\n", SDL_GetError());
        return -1;
    }
#ifdef __WIIU__
    printf("OSSetThreadRunQuantum\n");
    OSThread *native_handle = (OSThread *)SDL_GetThreadID(ctx->decode_thread);
    OSSetThreadRunQuantum(native_handle, 1000);
    printf("OSSetThreadRunQuantum success\n");
#endif

    printf("main loop SDL_PollEvent\n");
    long frames = 0;
    int printrate = 2000;
    SDL_GameController *pad;
    SDL_Event e;
    while (!ctx->quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                ctx->quit = SDL_TRUE;
            } else if (e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                printf("SDL_KEYDOWN or SDL_MOUSEBUTTONDOWN\n");
                ctx->quit = SDL_TRUE;
                // WIIU code change 3c.
                // Add controller events so we can exit on any button push
            } else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
                ctx->quit = SDL_TRUE;
                printf("SDL_PollEvent revd SDL_CONTROLLERBUTTONDOWN\n");
            } else if (e.type == SDL_CONTROLLERDEVICEADDED) {
                pad = SDL_GameControllerOpen(e.cdevice.which);
                printf("Added controller %d: %s\n", e.cdevice.which,
                       SDL_GameControllerName(pad));
            } else if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
                pad = SDL_GameControllerFromInstanceID(e.cdevice.which);
                printf("Removed controller: %s\n", SDL_GameControllerName(pad));
                SDL_GameControllerClose(pad);
            }
        }

        SDL_LockMutex(ctx->frame_mutex);

        if (frames % printrate == 0)
            printf("UpdateTexture frame %ld\n", frames);
        SDL_UpdateTexture(ctx->texture, NULL, ctx->frame_buffer,
                          ctx->width * 3);
        SDL_UnlockMutex(ctx->frame_mutex);

        SDL_RenderClear(ctx->renderer);

        if (frames % printrate == 0) printf("RenderCopy %ld\n", frames);
        SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, NULL);

        if (frames % printrate == 0) printf("RenderPresent %ld\n", frames);
        SDL_RenderPresent(ctx->renderer);
        if (frames % printrate == 0) printf("RenderPresent done %ld\n", frames);
        // SDL_Delay(7);
        SDL_Delay(1);  // Small delay for the main loop (SDL_Delay)

        // usleep(1000); // Small delay for the main loop

        frames++;
    }

    SDL_WaitThread(ctx->decode_thread, NULL);  // Wait for the thread to finish
    ctx->decode_thread = NULL;                 // Clean up the thread pointer
    // pthread_join(decode_thread_id, NULL);
    return 0;
}

// Function to stop and cleanup the video player
void stop_video_player(VideoPlayerContext *ctx) {
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
    if (ctx->frame) {
        av_frame_free(&ctx->frame);
    }
    if (ctx->video_codec_context) {
        // avcodec_close(ctx->video_codec_context);
        avcodec_free_context(&ctx->video_codec_context);
    }
    if (ctx->format_context) {
        avformat_close_input(&ctx->format_context);
    }
    avformat_network_deinit();
    SDL_Quit();
}

#ifdef BUILD_AS_FUNCTION
int ffmpeg_playvid_main(int argc, char *argv[]) {
#else
int main(int argc, char *argv[]) {
#endif
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <video_file>\n", argv[0]);
        return 1;
    }

    VideoPlayerContext player_ctx;
    if (init_video_player(&player_ctx, argv[1]) != 0) {
        fprintf(stderr, "Failed to initialize video player\n");
        return 1;
    }

    if (play_video(&player_ctx) != 0) {
        fprintf(stderr, "Failed to play video\n");
        stop_video_player(&player_ctx);  // Clean up even on play error.
        return 1;
    }

    stop_video_player(&player_ctx);  //  Always clean up.
    printf("Video playback complete.\n");
    return 0;
}
