/* A simple example of animation with SDL */

//  WIIU code change 1.
//  WUT homebrew pkgconfig includes the SDL sub directories
#ifdef __WIIU__
#include <SDL.h>
#include <SDL_image.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#endif

#include <stdio.h>
#include <string.h>

#include "sdlportables.h"

static const char* IMG_DIR = "images/oneko";

// Function to load a PNG image using SDL_image
SDL_Surface* loadPNG(const char* filename) {
    SDL_Surface* surface = IMG_Load(filename);
    if (!surface) {
        printf("SDL_image Error: %s\n", IMG_GetError());
        return NULL;
    }
    return surface;
}

/* WIIU code change 2.
   This was originally a stand alone SDL sample.  We will rename main()
   to just a function so we can call it from our generic crossplatorm main()
*/
// int main(int argc, char* argv[]) {
int sdlanimate_main() {
    // SDL window and renderer
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    // Array to store loaded PNG surfaces
    SDL_Surface* pngSurfaces[32];
    SDL_Texture* textures[32];  // Array to store textures

    // Screen dimensions
    const int SCREEN_WIDTH =
        640;  // Set these to appropriate values for your animation
    const int SCREEN_HEIGHT = 480;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL Error: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize SDL_image for PNG support
    if (!(IMG_Init(IMG_INIT_PNG))) {
        printf("SDL_image Error: %s\n", IMG_GetError());
        return 1;
    }

    // Create window and renderer
    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN,
                                &window, &renderer);
    if (!window || !renderer) {
        printf("SDL Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // set background to black.

    // Load PNG images and create textures
    printf("loading images\n");
    for (int i = 0; i < 32; i++) {
        char filename[128];  // Sufficient buffer for "i.png\0"
        // WiiU Change 2.  add parameter to control where images are loaded from
        snprintf(filename, 128, "%s/%s/%d.png", RES_ROOT, IMG_DIR, i + 1);
        pngSurfaces[i] = loadPNG(filename);
        if (!pngSurfaces[i]) {
            // Handle error, but continue loading other images
            // We will handle the NULL surfaces later during rendering
            continue;
        }

        // Create texture from surface
        textures[i] = SDL_CreateTextureFromSurface(renderer, pngSurfaces[i]);
        if (!textures[i]) {
            printf("SDL Error: %s\n", SDL_GetError());
            // Handle error, but continue loading other textures
            // We will handle the NULL textures later during rendering
        }
        SDL_FreeSurface(pngSurfaces[i]);  // Free surface after creating texture
        pngSurfaces[i] = NULL;  // set surface to NULL, so we don't double free.
    }

    // Main animation loop
    printf("Main animation loop\n");
    int quit = 0;
    SDL_Event event;
    int frame = 0;
    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
        }

        // Clear the renderer
        SDL_RenderClear(renderer);

        // Render the current frame, only if texture is valid
        if (textures[frame]) {
            SDL_Rect destRect;
            destRect.x = SCREEN_WIDTH / 2;
            destRect.y = SCREEN_HEIGHT / 2;
            SDL_QueryTexture(textures[frame], NULL, NULL, &destRect.w,
                             &destRect.h);  // get width/height
            SDL_RenderCopy(renderer, textures[frame], NULL, &destRect);
        }

        // Update the screen
        SDL_RenderPresent(renderer);

        // Increment frame and wrap around (32 frames, 0-31)
        frame = (frame + 1) % 32;

        SDL_Delay(100);  // Control animation speed (milliseconds)
    }
    printf("cleanup()\n");

    // Clean up: Destroy textures and renderer, and quit SDL
    for (int i = 0; i < 32; i++) {
        if (textures[i]) {  // check if texture is valid before destroying.
            SDL_DestroyTexture(textures[i]);
        }
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
