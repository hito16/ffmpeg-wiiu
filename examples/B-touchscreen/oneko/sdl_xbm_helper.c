#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // for sleep

// Include the neko_xbm.h header file
#ifdef BUILD_AS_MAIN
#include "neko_image_states.h"
#include "neko_xbm.h"
#endif

// Function to create an SDL surface from XBM data
SDL_Surface* create_surface_from_xbm(const unsigned char* xbm_data, int width,
                                     int height) {
    // Calculate bytes per row, accounting for padding
    int bytes_per_row = (width + 7) / 8;

    // Create an SDL surface with 8-bit depth (SDL_PIXELFORMAT_INDEX8)
    SDL_Surface* surface =
        SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);
    if (!surface) {
        fprintf(stderr, "SDL_CreateRGBSurface failed: %s\n", SDL_GetError());
        return NULL;
    }

    // Set the color palette (black and white)
    SDL_Color colors[2];
    colors[0].r = 0;  // Black
    colors[0].g = 0;
    colors[0].b = 0;
    colors[1].r = 255;  // White
    colors[1].g = 255;
    colors[1].b = 255;

    // Create a color palette for the surface.  Important for 8-bit surfaces.
    if (SDL_SetPaletteColors(surface->format->palette, colors, 0, 2) < 0) {
        fprintf(stderr, "SDL_SetPaletteColors failed: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return NULL;
    }

    // Get a pointer to the surface's pixel data
    Uint8* pixels = (Uint8*)surface->pixels;
    if (!pixels) {
        fprintf(stderr, "Could not get surface pixels\n");
        SDL_FreeSurface(surface);
        return NULL;
    }

    // Initialize the surface to black (important for ensuring correct pixels)
    memset(pixels, 0, surface->pitch * height);

    // Copy the XBM data to the SDL surface, handling bit order correctly
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Calculate the byte offset for the current pixel in the XBM data
            int xbm_byte_index = (y * bytes_per_row) + (x / 8);
            // Get the byte containing the pixel from XBM data
            Uint8 xbm_byte = xbm_data[xbm_byte_index];
            // Extract the bit for the current pixel, LSB-first
            Uint8 bit = (xbm_byte >> (x % 8)) & 0x01;

            // Calculate the index into the pixel array.
            size_t pixel_index = y * surface->pitch + x;
            pixels[pixel_index] = bit ? 1 : 0;  // 0 for black, 1 for white
        }
    }
    return surface;
}

#ifdef BUILD_AS_MAIN
int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow(
        "XBM Image Cycle", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        640,  // Fixed window size
        480, SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Surface* image_surface = NULL;
    SDL_Texture* texture = NULL;
    int current_image_index = 0;  // Add this line

    int num_images = sizeof(xbm_images) / sizeof(xbm_images[0]);
    // Main loop to display the images
    int quit = 0;
    SDL_Event event;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
        }

        // Clear the renderer
        SDL_RenderClear(renderer);

        // Create and display the current image
        const unsigned char* xbm_data = xbm_images[current_image_index].bits;
        int width = xbm_images[current_image_index].width;
        int height = xbm_images[current_image_index].height;

        // Create a surface from the XBM data
        if (image_surface) {
            SDL_FreeSurface(image_surface);
        }
        image_surface = create_surface_from_xbm(xbm_data, width, height);
        if (!image_surface) {
            fprintf(stderr, "create_surface_from_xbm failed\n");
            // create_surface_from_xbm already prints the error.
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }

        // Create a texture from the surface
        if (texture) {
            SDL_DestroyTexture(texture);
        }
        texture = SDL_CreateTextureFromSurface(renderer, image_surface);
        if (!texture) {
            fprintf(stderr, "SDL_CreateTextureFromSurface failed: %s\n",
                    SDL_GetError());
            SDL_FreeSurface(image_surface);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
        // Define the destination rectangle to center the image.
        SDL_Rect dest_rect;
        dest_rect.x = (640 - width) / 2;   // Center horizontally
        dest_rect.y = (480 - height) / 2;  // Center vertically
        dest_rect.w = width;
        dest_rect.h = height;

        // Copy the texture to the renderer
        SDL_RenderCopy(renderer, texture, NULL, &dest_rect);

        // Update the screen
        SDL_RenderPresent(renderer);

        // Increment the image index and wrap around
        current_image_index = (current_image_index + 1) % num_images;

        // Wait for 2 seconds
        sleep(2);
    }

    // Clean up
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_FreeSurface(image_surface);
    SDL_Quit();

    return 0;
}
#endif
