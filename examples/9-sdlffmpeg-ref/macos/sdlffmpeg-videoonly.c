#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <SDL.h>
#include <SDL_joystick.h>
#include <SDL_gamecontroller.h>
#include <libavutil/imgutils.h> // Include for av_image_get_buffer_size and av_image_fill_arrays

#ifdef __MINGW32__
#undef main /* Prevents SDL from redefining main */
#endif

// Define screen width and height
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

// Struct to hold FFmpeg objects
typedef struct {
    AVFormatContext *formatContext;
    int videoStreamIndex;
    AVCodecParameters *videoCodecParameters;
    AVCodec *videoCodec;
    AVCodecContext *videoCodecContext;
    AVFrame *videoFrame;
    AVFrame *rgbFrame;
    struct SwsContext *swsContext;
    uint8_t *videoBuffer;
} FFmpegCtx;

// Struct to hold SDL objects
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_GameController *controller; // Add game controller
} SDL_Ctx;

// Function to initialize FFmpeg objects
int initializeFFmpeg(const char *filename, FFmpegCtx *fCtx) {
    // Initialize to NULL state
    fCtx->formatContext = NULL;
    fCtx->videoStreamIndex = -1;
    fCtx->videoCodecParameters = NULL;
    fCtx->videoCodec = NULL;
    fCtx->videoCodecContext = NULL;
    fCtx->videoFrame = NULL;
    fCtx->rgbFrame = NULL;
    fCtx->swsContext = NULL;
    fCtx->videoBuffer = NULL;

    // 1. Open the input file with FFmpeg
    printf("Opening input file: %s\n", filename);
    if (avformat_open_input(&fCtx->formatContext, filename, NULL, NULL) != 0) {
        fprintf(stderr, "avformat_open_input failed\n");
        return -1;
    }

    // 2. Find the stream information
    // Add parameters to avformat_find_stream_info to potentially speed it up
    //fCtx->formatContext->flags |= AVFMT_FLAG_FAST_SEEK;
    AVDictionary *options = NULL;
    //av_dict_set(&options, "analyzeduration", "500000", 0); // Analyze only the first 0.5 seconds
    //av_dict_set(&options, "probesize", "1000000", 0);    // Limit probe size to 1MB
    printf("Finding stream information\n");
    if (avformat_find_stream_info(fCtx->formatContext, &options) < 0) {
        fprintf(stderr, "avformat_find_stream_info failed\n");
        avformat_close_input(&fCtx->formatContext);
        return -1;
    }
    av_dict_free(&options); // Clean up the options.
   // fCtx->formatContext->flags &= ~AVFMT_FLAG_FAST_SEEK;

    // 3. Find the video stream
    printf("Finding video stream\n");
    for (int i = 0; i < fCtx->formatContext->nb_streams; i++) {
        if (fCtx->formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            fCtx->videoStreamIndex = i;
            break;
        }
    }

    if (fCtx->videoStreamIndex == -1) {
        fprintf(stderr, "Could not find video stream\n");
        avformat_close_input(&fCtx->formatContext);
        return -1;
    }

    // 4. Get the video codec parameters
    printf("Getting video codec parameters\n");
    fCtx->videoCodecParameters = fCtx->formatContext->streams[fCtx->videoStreamIndex]->codecpar;

    // 5. Find the video decoder
    printf("Finding video decoder\n");
    const AVCodec *decoder = avcodec_find_decoder(fCtx->videoCodecParameters->codec_id);
    fCtx->videoCodec = (AVCodec*)decoder;
    if (fCtx->videoCodec == NULL) {
        fprintf(stderr, "avcodec_find_decoder failed for video\n");
        avformat_close_input(&fCtx->formatContext);
        return -1;
    }

    // 6. Allocate a video codec context
    printf("Allocating video codec context\n");
    fCtx->videoCodecContext = avcodec_alloc_context3(fCtx->videoCodec);
    if (fCtx->videoCodecContext == NULL) {
        fprintf(stderr, "avcodec_alloc_context3 failed for video\n");
        avformat_close_input(&fCtx->formatContext);
        return -1;
    }

    // 7. Copy video codec parameters to the video codec context
    printf("Copying video codec parameters to codec context\n");
    if (avcodec_parameters_to_context(fCtx->videoCodecContext, fCtx->videoCodecParameters) < 0) {
        fprintf(stderr, "avcodec_parameters_to_context failed for video\n");
        avcodec_free_context(&fCtx->videoCodecContext);
        avformat_close_input(&fCtx->formatContext);
        return -1;
    }

    // 8. Open the video codec
    printf("Opening video codec\n");
    if (avcodec_open2(fCtx->videoCodecContext, fCtx->videoCodec, NULL) < 0) {
        fprintf(stderr, "avcodec_open2 failed for video\n");
        avcodec_free_context(&fCtx->videoCodecContext);
        avformat_close_input(&fCtx->formatContext);
        return -1;
    }

    // 9. Allocate a frame to hold the decoded video data
    printf("Allocating frame for decoded video\n");
    fCtx->videoFrame = av_frame_alloc();
    if (fCtx->videoFrame == NULL) {
        fprintf(stderr, "av_frame_alloc failed for video\n");
        avcodec_free_context(&fCtx->videoCodecContext);
        avformat_close_input(&fCtx->formatContext);
        return -1;
    }

    // 10. Allocate a frame to hold the RGB data
    printf("Allocating frame for RGB data\n");
    fCtx->rgbFrame = av_frame_alloc();
    if (fCtx->rgbFrame == NULL) {
        fprintf(stderr, "av_frame_alloc failed for RGB frame\n");
        av_frame_free(&fCtx->videoFrame);
        avcodec_free_context(&fCtx->videoCodecContext);
        avformat_close_input(&fCtx->formatContext);
        return -1;
    }

    // 11. Determine the required buffer size for the RGB frame
    printf("Determining buffer size for RGB frame\n");
    int videoBufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, fCtx->videoCodecContext->width, fCtx->videoCodecContext->height, 1); // 1 for alignment
    fCtx->videoBuffer = (uint8_t *)av_malloc(videoBufferSize * sizeof(uint8_t));
    if (fCtx->videoBuffer == NULL) {
        fprintf(stderr, "av_malloc failed for video buffer\n");
        av_frame_free(&fCtx->rgbFrame);
        av_frame_free(&fCtx->videoFrame);
        avcodec_free_context(&fCtx->videoCodecContext);
        avformat_close_input(&fCtx->formatContext);
        return -1;
    }

    // 12. Assign the buffer to the RGB frame
    printf("Assigning buffer to RGB frame\n");
    if (av_image_fill_arrays(fCtx->rgbFrame->data, fCtx->rgbFrame->linesize, fCtx->videoBuffer, AV_PIX_FMT_RGB24, fCtx->videoCodecContext->width, fCtx->videoCodecContext->height, 1) < 0) {
        fprintf(stderr, "av_image_fill_arrays failed\n");
        av_free(fCtx->videoBuffer);
        av_frame_free(&fCtx->rgbFrame);
        av_frame_free(&fCtx->videoFrame);
        avcodec_free_context(&fCtx->videoCodecContext);
        avformat_close_input(&fCtx->formatContext);
        return -1;
    }

    // 13. Create a Swscale context for converting from the video frame to RGB
    printf("Creating Swscale context\n");
    fCtx->swsContext = sws_getContext(
        fCtx->videoCodecContext->width, fCtx->videoCodecContext->height, fCtx->videoCodecContext->pix_fmt,
        fCtx->videoCodecContext->width, fCtx->videoCodecContext->height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, NULL, NULL, NULL);
    if (fCtx->swsContext == NULL) {
        fprintf(stderr, "sws_getContext failed\n");
        av_free(fCtx->videoBuffer);
        av_frame_free(&fCtx->rgbFrame);
        av_frame_free(&fCtx->videoFrame);
        avcodec_free_context(&fCtx->videoCodecContext);
        avformat_close_input(&fCtx->formatContext);
        return -1;
    }

    return 0; //success
}

void cleanupFFmpeg(FFmpegCtx *fCtx) {
    if (fCtx->swsContext) {
        sws_freeContext(fCtx->swsContext);
    }
    if (fCtx->videoBuffer) {
        av_free(fCtx->videoBuffer);
    }
    if (fCtx->rgbFrame) {
        av_frame_free(&fCtx->rgbFrame);
    }
    if (fCtx->videoFrame) {
        av_frame_free(&fCtx->videoFrame);
    }
    if (fCtx->videoCodecContext) {
        avcodec_free_context(&fCtx->videoCodecContext);
    }
    if (fCtx->formatContext) {
        avformat_close_input(&fCtx->formatContext); // Corrected line
    }
    //  No need to free codecParameters, it's part of formatContext
    //  avcodec_free_context frees the codec
}

int initializeSDL(SDL_Ctx *sCtx, int width, int height) {
    sCtx->window = NULL;
    sCtx->renderer = NULL;
    sCtx->texture = NULL;
    sCtx->controller = NULL; // Initialize controller to NULL
     // 1. Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    // 2. Create an SDL window
    sCtx->window = SDL_CreateWindow(
        "FFmpeg SDL Player",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        width, height,
        SDL_WINDOW_SHOWN);
    if (sCtx->window == NULL) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // 3. Create an SDL renderer
    sCtx->renderer = SDL_CreateRenderer(sCtx->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (sCtx->renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(sCtx->window);
        SDL_Quit();
        return -1;
    }

    // 4. Create an SDL texture
    sCtx->texture = SDL_CreateTexture(
        sCtx->renderer,
        SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height);
    if (sCtx->texture == NULL) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(sCtx->renderer);
        SDL_DestroyWindow(sCtx->window);
        SDL_Quit();
        return -1;
    }
    return 0;
}

void cleanupSDL(SDL_Ctx *sCtx) {
    if (sCtx->texture) {
        SDL_DestroyTexture(sCtx->texture);
    }
    if (sCtx->renderer) {
        SDL_DestroyRenderer(sCtx->renderer);
    }
    if (sCtx->window) {
        SDL_DestroyWindow(sCtx->window);
    }
    if (sCtx->controller) {
        SDL_GameControllerClose(sCtx->controller);
    }
    SDL_Quit();
}

// Function to handle decoding and display of video frames
int decodeAndDisplay(FFmpegCtx *fCtx, SDL_Ctx *sCtx, AVPacket *packet) {
    int ret = 0; // Initialize ret
    while (ret >= 0) {
        ret = avcodec_receive_frame(fCtx->videoCodecContext, fCtx->videoFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return 0; // Need more input or decoder is finished
        } else if (ret < 0) {
            fprintf(stderr, "avcodec_receive_frame failed for video: %s\n", av_err2str(ret));
            return -1;
        }

        // 6.5 Convert the video frame to RGB
        sws_scale(fCtx->swsContext, (const uint8_t *const *)fCtx->videoFrame->data, fCtx->videoFrame->linesize, 0, fCtx->videoFrame->height,
                  fCtx->rgbFrame->data, fCtx->rgbFrame->linesize);

        // 6.6 Update the SDL texture with the RGB data
        SDL_UpdateTexture(
            sCtx->texture,
            NULL,
            fCtx->rgbFrame->data[0],
            fCtx->rgbFrame->linesize[0]);

        // 6.7 Clear the renderer
        SDL_RenderClear(sCtx->renderer);

        // 6.8 Copy the texture to the renderer
        SDL_RenderCopy(sCtx->renderer, sCtx->texture, NULL, NULL);

        // 6.9 Present the rendered image
        SDL_RenderPresent(sCtx->renderer);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    // Check if a file was provided.
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return -1;
    }

    const char *filename = argv[1];

    // 1. Initialize FFmpeg
    FFmpegCtx fCtx;
    if (initializeFFmpeg(filename, &fCtx) != 0) {
        return -1; // Error already printed by initializeFFmpeg
    }

    // 2. Initialize SDL
    SDL_Ctx sCtx;
    if (initializeSDL(&sCtx, SCREEN_WIDTH, SCREEN_HEIGHT) != 0) {
        cleanupFFmpeg(&fCtx);
        return -1;
    }
    // 6. Main loop: read, decode, convert, display
    SDL_Event event;
    int quit = 0;
    AVPacket *packet = av_packet_alloc(); // Allocate packet.  av_init_packet is deprecated.
    if (!packet) {
        fprintf(stderr, "av_packet_alloc failed\n");
        cleanupSDL(&sCtx);
        cleanupFFmpeg(&fCtx);
        return -1;
    }
    while (!quit) {
        // 6.1 Read a packet from the input file
        if (av_read_frame(fCtx.formatContext, packet) < 0) {
            // Handle end of file or error.  For simplicity, we'll just break.
            fprintf(stderr, "av_read_frame end of file\n");
            break;
        }

        // 6.2 Check if the packet belongs to the video stream
        if (packet->stream_index == fCtx.videoStreamIndex) {
            // 6.3 Decode the video packet
            int ret = avcodec_send_packet(fCtx.videoCodecContext, packet);
            if (ret < 0) {
                fprintf(stderr, "avcodec_send_packet failed for video: %s\n", av_err2str(ret));
                av_packet_unref(packet);
                continue; // IMPORTANT: Continue to the next packet.  Do NOT break here.
            }

            // 6.4 Receive frames from the decoder and display
            if (decodeAndDisplay(&fCtx, &sCtx, packet) != 0){
                quit = 1;
            }
        }
        av_packet_unref(packet);  // VERY IMPORTANT
        // 6.10 Handle SDL events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
            // Handle controller events
            else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                printf("Controller button %d down\n", event.cbutton.button);
            }
            else if (event.type == SDL_CONTROLLERDEVICEADDED) {
                printf("Controller added: index %d\n", event.cdevice.which);
                if (SDL_IsGameController(event.cdevice.which)) {
                    sCtx.controller = SDL_GameControllerOpen(event.cdevice.which);
                    if (sCtx.controller) {
                         printf("Successfully opened controller\n");
                    }
                    else{
                        fprintf(stderr,"SDL_GameControllerOpen failed: %s\n",SDL_GetError());
                    }
                }
            }
            else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
                printf("Controller removed: index %d\n", event.cdevice.which);
                if (sCtx.controller) {
                    SDL_GameControllerClose(sCtx.controller);
                    sCtx.controller = NULL; // important
                    printf("Controller closed\n");
                }
            }
        }
    }
    av_packet_free(&packet);
    // 7. Clean up and exit
    cleanupSDL(&sCtx);
    cleanupFFmpeg(&fCtx);
    return 0;
}

