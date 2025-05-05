#include <SDL.h>
#include <stdio.h>

// Function to initialize SDL, create a window, and a renderer
int initialize_sdl(SDL_Window **window, SDL_Renderer **renderer) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    // Create a window
    *window = SDL_CreateWindow(
        "SDL Window",                  // Window title
        SDL_WINDOWPOS_UNDEFINED,           // Initial x position
        SDL_WINDOWPOS_UNDEFINED,           // Initial y position
        640,                           // Width, in pixels
        480,                           // Height, in pixels
        SDL_WINDOW_SHOWN                 // Flags
    );
    if (*window == NULL) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // Create a renderer.  This is needed for drawing.
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (*renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return -1;
    }
    return 0; // Return 0 on success
}

// Function to clean up SDL resources
void cleanup_sdl(SDL_Window *window, SDL_Renderer *renderer) {
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

int main(int argc, char *argv[]) {
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    // Initialize SDL, window, and renderer
    if (initialize_sdl(&window, &renderer) != 0) {
        // If initialization fails, the function prints the error message and returns -1
        cleanup_sdl(window, renderer);
        return 1;
    }
    // Main loop:  This is where the program spends most of its time.
    int quit = 0;
    SDL_Event event;
    while (!quit) {
        // Wait for an event to occur.  This could be a key press,
        // a mouse movement, a window event (resize, close), etc.
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    // The user has clicked the close button on the window.
                    quit = 1;
                    break;
                case SDL_KEYDOWN:
                    // A key was pressed.
                    printf("Key pressed: %s\n", SDL_GetKeyName(event.key.keysym.sym));
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                         quit = 1;
                    }
                    break;
                // Add other event handling here as needed.
            }
        }

        // In a real program, you would draw things here.  For this
        // example, we just clear the window to black.
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black
        SDL_RenderClear(renderer);

        // Present the back buffer.  This makes the drawing visible.
        SDL_RenderPresent(renderer);
    }

    // Clean up:  Free the resources we've allocated.
    cleanup_sdl(window, renderer);

    return 0;
}
