#include <SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

//  FFmpeg context struct
typedef struct {
    AVFormatContext *format_context;
    int video_stream_index;
    AVCodecContext *codec_context;
    AVFrame *frame;
    AVPacket *packet;
} FFmpegContext;

// SDL context struct
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
} SDL_Context;

// Define screen width and height - Assumed to be fixed
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

int render_frame_ffmpeg_sdl(SDL_Context *sdl_context, FFmpegContext *ffctx) {
    int ret = 0;
    AVFrame *rgb_frame = NULL;
    struct SwsContext *sws_context = NULL;
    uint8_t *buffer = NULL;

    // 1. Allocate an AVFrame for RGB data
    rgb_frame = av_frame_alloc();
    if (!rgb_frame) {
        fprintf(stderr, "Could not allocate RGB frame\n");
        return -1;
    }

    // 2. Determine the required buffer size for the RGB frame
    // Use the fixed screen dimensions here
    int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, SCREEN_WIDTH, SCREEN_HEIGHT, 1);
    buffer = (uint8_t *)av_malloc(buffer_size * sizeof(uint8_t));
    if (!buffer) {
        fprintf(stderr, "Could not allocate buffer for RGB frame\n");
        ret = -1;
        goto cleanup;
    }

     // 3. Assign the buffer to the RGB frame
     // Use the fixed screen dimensions here
    if (av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, buffer, AV_PIX_FMT_RGB24, SCREEN_WIDTH, SCREEN_HEIGHT, 1) < 0) {
        fprintf(stderr, "av_image_fill_arrays failed\n");
        ret = -1;
        goto cleanup;
    }

    // 4. Create a Swscale context for converting from the video frame to RGB
    sws_context = sws_getContext(
        ffctx->codec_context->width,  // Source width
        ffctx->codec_context->height, // Source height
        ffctx->codec_context->pix_fmt, // Source pixel format
        SCREEN_WIDTH,                         // Destination width
        SCREEN_HEIGHT,                        // Destination height
        AV_PIX_FMT_RGB24,                     // Destination pixel format
        SWS_BILINEAR,                         // Scaling algorithm
        NULL,                                  // Source filter (optional)
        NULL,                                  // Destination filter (optional)
        NULL                                   // Options (optional)
        );
    if (!sws_context) {
        fprintf(stderr, "sws_getContext failed\n");
        ret = -1;
        goto cleanup;
    }

    // 5. Convert the video frame to RGB format
    ret = sws_scale(sws_context,
              (const uint8_t *const *)ffctx->frame->data, // Source data
              ffctx->frame->linesize,                  // Source line size
              0,                                               // Source starting Y offset
              ffctx->frame->height,                   // Source height
              rgb_frame->data,                                  // Destination data
              rgb_frame->linesize);                               // Destination line size
    if (ret < 0)
    {
        fprintf(stderr, "sws_scale failed\n");
        goto cleanup;
    }

    // 6. Update the SDL texture with the RGB data
    SDL_UpdateTexture(
        sdl_context->texture,      // The SDL texture to update
        NULL,                     // A pointer to the update rectangle (NULL for the entire texture)
        rgb_frame->data[0],       // Pointer to the pixel data
        rgb_frame->linesize[0]);    // The pitch of the pixel data in bytes
    if (ret < 0)
    {
        fprintf(stderr, "SDL_UpdateTexture failed\n");
        goto cleanup;
    }

    // 7. Clear the renderer
    SDL_RenderClear(sdl_context->renderer);

    // 8. Copy the texture to the renderer
    SDL_RenderCopy(sdl_context->renderer, sdl_context->texture, NULL, NULL);

    // 9. Present the rendered image
    SDL_RenderPresent(sdl_context->renderer);

cleanup:
    if (sws_context) {
        sws_freeContext(sws_context);
    }
    if (buffer) {
        av_free(buffer);
    }
    if (rgb_frame) {
        av_frame_free(&rgb_frame);
    }
    return ret;
}

