#include <SDL.h>
#include <SDL_audio.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Context struct to hold SDL and application-specific data
typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* videoTexture; // Texture for displaying video frames
    SDL_AudioSpec audioSpec;   // Audio specification
    SDL_AudioDeviceID audioDevice; // Audio device ID
    // Add any other application-specific data here
} AppContext;

// Function to handle SDL errors
void handleSDLError(const char* msg) {
    fprintf(stderr, "%s: %s\n", msg, SDL_GetError());
    exit(1);
}

// Global variable for the audio queue (simplified for this example)
static Uint8* audioQueue = NULL;
static Uint32 audioQueueSize = 0;
static SDL_mutex* audioQueueMutex;

// Audio callback function (called by SDL)
void audioCallback(void* userdata, Uint8* stream, int len) {
    // Clear the stream (important!)
    SDL_memset(stream, 0, len);

    if (audioQueueSize > 0) {
        SDL_LockMutex(audioQueueMutex);
        Uint32 copyLen = (len > audioQueueSize) ? audioQueueSize : len;
        SDL_memcpy(stream, audioQueue, copyLen);
        // Remove the copied data from the audio queue.
        if (copyLen < audioQueueSize) {
            memmove(audioQueue, audioQueue + copyLen, audioQueueSize - copyLen);
        }
        audioQueueSize -= copyLen;
        Uint8* temp = audioQueue;
        audioQueue = (Uint8*)malloc(audioQueueSize);
        if (audioQueue) {
           memcpy(audioQueue, temp + copyLen, audioQueueSize);
        }
        free(temp);
        SDL_UnlockMutex(audioQueueMutex);
    }
}

// Function to queue audio data (thread-safe)
void queueAudioData(AppContext* ctx, const Uint8* data, Uint32 size) {
    SDL_LockMutex(audioQueueMutex);
    Uint8* newQueue = (Uint8*)malloc(audioQueueSize + size);
    if (!newQueue) {
        fprintf(stderr, "Memory allocation error in queueAudioData\n");
        SDL_UnlockMutex(audioQueueMutex);
        return; // Handle allocation failure
    }
    if (audioQueue) {
        memcpy(newQueue, audioQueue, audioQueueSize);
        memcpy(newQueue + audioQueueSize, data, size);
        free(audioQueue);
    }
    else {
        memcpy(newQueue + audioQueueSize, data, size);
    }

    audioQueue = newQueue;
    audioQueueSize += size;
    SDL_UnlockMutex(audioQueueMutex);
}

// Implementation of the playit function
void playit(AppContext* ctx, AVFrame* video_frame_rgb, AVFrame* audio_frame_resampled, double video_pts_sec, double audio_pts_sec) {
    if (video_frame_rgb) {
        // Handle video frame
        if (!ctx->videoTexture) {
            // Create texture on the first video frame
            ctx->videoTexture = SDL_CreateTexture(ctx->renderer,
                                                 SDL_PIXELFORMAT_RGB24, // Or SDL_PIXELFORMAT_ARGB32, depending on your FFmpeg output
                                                 SDL_TEXTUREACCESS_STREAMING,
                                                 video_frame_rgb->width,
                                                 video_frame_rgb->height);
            if (!ctx->videoTexture) {
                handleSDLError("SDL_CreateTexture failed");
                return; // Important: Return on error
            }
        }

        // Update texture with new video frame data
        void* textureData;
        
        int texturePitch;
        if (SDL_LockTexture(ctx->videoTexture, NULL, &textureData, &texturePitch) < 0) {
             handleSDLError("SDL_LockTexture failed");
             return;
        }

        // Copy data to the texture, handle different formats and line sizes
        for (int y = 0; y < video_frame_rgb->height; ++y) {
            Uint8* src = video_frame_rgb->data[0] + y * video_frame_rgb->linesize[0];
            Uint8* dst = (Uint8*)textureData + y * texturePitch;
            memcpy(dst, src, video_frame_rgb->width * 3); // Assumes RGB24, adjust as needed.
        }
        SDL_UnlockTexture(ctx->videoTexture);

        // Clear the renderer
        SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 255); // Black background
        SDL_RenderClear(ctx->renderer);

        // Copy the texture to the renderer
        SDL_Rect destRect = { 0, 0, video_frame_rgb->width, video_frame_rgb->height };
        SDL_RenderCopy(ctx->renderer, ctx->videoTexture, NULL, &destRect);

        // Present the frame
        SDL_RenderPresent(ctx->renderer);

        //av_frame_free(&video_frame_rgb); // You manage frame freeing outside this function
    } else if (audio_frame_resampled) {
        // Handle audio frame
        // Convert audio data to SDL's expected format and queue it.
        // Assuming audio_frame_resampled->format is a supported format by SDL
        // and you have already setup the resampler.

        // Get the buffer size.
        int buffer_size = av_samples_get_buffer_size(NULL,
                                                   audio_frame_resampled->channels,
                                                   audio_frame_resampled->nb_samples,
                                                   (enum AVSampleFormat)audio_frame_resampled->format,
                                                   1); //1 for packed
        queueAudioData(ctx, audio_frame_resampled->data[0], buffer_size);

        //av_frame_free(&audio_frame_resampled);  // You manage frame freeing outside.
    }
}

int main(int argc, char* argv[]) {
    AppContext ctx;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        handleSDLError("SDL_Init failed");
    }

    // Create window and renderer
    ctx.window = SDL_CreateWindow("FFmpeg SDL Player",
                                    SDL_WINDOWPOS_UNDEFINED,
                                    SDL_WINDOWPOS_UNDEFINED,
                                    800, 600, // Or your desired size
                                    SDL_WINDOW_SHOWN);
    if (!ctx.window) {
        handleSDLError("SDL_CreateWindow failed");
    }

    ctx.renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); //Use vsync!
    if (!ctx.renderer) {
        renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_SOFTWARE);
        if (!renderer) {
            handleSDLError("SDL_CreateRenderer Failed");
        }
    }

    // Setup audio
    SDL_memset(&ctx.audioSpec, 0, sizeof(ctx.audioSpec));
    ctx.audioSpec.freq = 44100; // Or your desired frequency
    ctx.audioSpec.format = AUDIO_F32SYS; // Use Float for simplicity and best compatibility.
    ctx.audioSpec.channels = 2; // Or your desired number of channels
    ctx.audioSpec.samples = 1024; // Or your desired buffer size
    ctx.audioSpec.callback = audioCallback;
    ctx.audioSpec.userdata = &ctx;

    audioQueueMutex = SDL_CreateMutex();
    if (!audioQueueMutex){
        handleSDLError("SDL_CreateMutex Failed");
    }
    ctx.audioDevice = SDL_OpenAudioDevice(NULL, 0, &ctx.audioSpec, &ctx.audioSpec, 0);
    if (ctx.audioDevice == 0) {
        handleSDLError("SDL_OpenAudioDevice failed");
    }
     SDL_PauseAudioDevice(ctx.audioDevice, 0); // Start playing

    // Main loop (in a real application, you'd be reading from FFmpeg here)
    SDL_Event event;
    int quit = 0;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
        }
        SDL_Delay(16); // Add a small delay, or use proper timing.
    }

    // Cleanup
    SDL_PauseAudioDevice(ctx.audioDevice, 1);
    SDL_CloseAudioDevice(ctx.audioDevice);
    SDL_DestroyMutex(audioQueueMutex);
    if (ctx.videoTexture)
        SDL_DestroyTexture(ctx.videoTexture);
    SDL_DestroyRenderer(ctx.renderer);
    SDL_DestroyWindow(ctx->window);
    SDL_Quit();

    return 0;
}
