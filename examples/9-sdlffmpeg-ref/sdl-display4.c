#include <SDL.h>
#include <stdio.h>

// Struct to hold SDL resources
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
} SDL_Context;

// Function to initialize SDL, create a window, and a renderer
int initialize_sdl(SDL_Context *sdl_context, int width, int height) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    // Create a window
    sdl_context->window = SDL_CreateWindow(
        "SDL Window",                  // Window title
        SDL_WINDOWPOS_UNDEFINED,           // Initial x position
        SDL_WINDOWPOS_UNDEFINED,           // Initial y position
        width,                           // Width, in pixels
        height,                          // Height, in pixels
        SDL_WINDOW_SHOWN                 // Flags
    );
    if (sdl_context->window == NULL) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // Create a renderer.  This is needed for drawing.
    sdl_context->renderer = SDL_CreateRenderer(sdl_context->window, -1, SDL_RENDERER_ACCELERATED);
    if (sdl_context->renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(sdl_context->window);
        SDL_Quit();
        return -1;
    }

    // Create a texture.  This is where the image data will go.
    sdl_context->texture = SDL_CreateTexture(
        sdl_context->renderer,
        SDL_PIXELFORMAT_RGB24,  // Or another format, e.g., SDL_PIXELFORMAT_ARGB8888
        SDL_TEXTUREACCESS_STREAMING, // Or SDL_TEXTUREACCESS_STATIC
        width,
        height
    );
    if (sdl_context->texture == NULL) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(sdl_context->renderer);
        SDL_DestroyWindow(sdl_context->window);
        SDL_Quit();
        return -1;
    }
    return 0; // Return 0 on success
}

// Function to clean up SDL resources
void cleanup_sdl(SDL_Context *sdl_context) {
    if (sdl_context->texture) {
        SDL_DestroyTexture(sdl_context->texture);
    }
    if (sdl_context->renderer) {
        SDL_DestroyRenderer(sdl_context->renderer);
    }
    if (sdl_context->window) {
        SDL_DestroyWindow(sdl_context->window);
    }
    SDL_Quit();
}

// Function to render a frame
void render_frame(SDL_Context *sdl_context) {
    // In a real program, you would:
    // 1.  Update the texture with your image data.
    // 2.  Copy the texture to the renderer.
    // 3.  Present the renderer.

    // For this example, we just clear the window to black.
    SDL_SetRenderDrawColor(sdl_context->renderer, 0, 0, 0, 255); // Black
    SDL_RenderClear(sdl_context->renderer);

    // Copy the texture to the renderer
    SDL_RenderCopy(sdl_context->renderer, sdl_context->texture, NULL, NULL);

    // Present the back buffer.  This makes the drawing visible.
    SDL_RenderPresent(sdl_context->renderer);
}

// Function to handle the SDL event loop
int handle_events(SDL_Context *sdl_context) {
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
                case SDL_CONTROLLERBUTTONDOWN:
                    printf("Controller button %d pressed\n", event.cbutton.button);
                    break;
                case SDL_CONTROLLERDEVICEADDED:
                    printf("Controller %d connected\n", event.cdevice.which);
                    break;
                case SDL_CONTROLLERDEVICEREMOVED:
                    printf("Controller %d disconnected\n", event.cdevice.which);
                    break;
                    // Add other event handling here as needed.
            }
        }

        render_frame(sdl_context);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    SDL_Context sdl_context = {0}; // Initialize all members to NULL
    int width = 640;   // Initial width
    int height = 480;  // Initial height

    // Initialize SDL, window, renderer, and texture
    if (initialize_sdl(&sdl_context, width, height) != 0) {
        // If initialization fails, the function prints the error message and returns -1
        cleanup_sdl(&sdl_context);
        return 1;
    }
    // Main loop:  This is where the program spends most of its time.
    handle_events(&sdl_context);

    // Clean up:  Free the resources we've allocated.
    cleanup_sdl(&sdl_context);

    return 0;
}

