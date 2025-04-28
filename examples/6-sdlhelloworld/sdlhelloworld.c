/* An SDL WiiU "Hello World" example
   Assumes you built romfs-wiiu and downloaded fonts

   The is the last example of us being explicit and pedantic, ie close
   to SDL tutorials. 
*/
#include <SDL.h>
#include <SDL_ttf.h>
#include <romfs-wiiu.h>
#include <whb/log.h>
#include <whb/log_udp.h>

int main(int argc, char** argv) {
    WHBLogUdpInit();

    romfsInit();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        WHBLogPrintf("Failed to init SDL: %s\n", SDL_GetError());
        romfsExit();
        WHBLogUdpDeinit();
        return 1;
    }

    // Create a window
    SDL_Window* window =
        SDL_CreateWindow("Simple SDL Window", SDL_WINDOWPOS_UNDEFINED,
                         SDL_WINDOWPOS_UNDEFINED, 854, 480, SDL_WINDOW_SHOWN);

    if (window == NULL) {
        WHBLogPrintf("Window creation failed: %s\n", SDL_GetError());
        romfsExit();
        WHBLogUdpDeinit();
        SDL_Quit();
        return 1;
    }

    // Create a renderer
    SDL_Renderer* renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        WHBLogPrintf("Renderer creation failed: %s\n", SDL_GetError());
        romfsExit();
        WHBLogUdpDeinit();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Set the background color to white
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);  // RGBA: White
    SDL_RenderClear(renderer);  // Clear the entire window with the chosen color

    // Make a red background for the text
    SDL_Rect rect = {240, 200, 200, 80};               // x, y, width, height
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Red color
    SDL_RenderFillRect(renderer, &rect);

    WHBLogPrint("Finished Draw Rectangle");

    /* BEGIN writing text "Hello, World!" to screen
       Set up fonts, texture and surface so we can draw text
    */

    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        WHBLogPrintf("SDL_ttf initialization failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    // Downloaded google font and placed in ./romfs/res
    // Built into the rpx along with the romfs image
    // Mounted under romfs:/
    TTF_Font* font = TTF_OpenFont("romfs:/res/Roboto-Regular.ttf", 28);
    if (font == NULL) {
        WHBLogPrintf("Font loading failed: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create a surface containing the text "Hello, World!"
    SDL_Color text_color = {0, 0, 0, 255};  // Black color
    SDL_Surface* text_surface =
        TTF_RenderText_Solid(font, "Hello, World!", text_color);
    if (text_surface == NULL) {
        WHBLogPrintf("Text rendering failed: %s\n", TTF_GetError());
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create a texture from the surface
    SDL_Texture* text_texture =
        SDL_CreateTextureFromSurface(renderer, text_surface);
    if (text_texture == NULL) {
        WHBLogPrintf("Texture creation failed: %s\n", SDL_GetError());
        SDL_FreeSurface(text_surface);
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Get the dimensions of the text texture
    int text_width = text_surface->w;
    int text_height = text_surface->h;

    // Define the position where the text will be rendered
    SDL_Rect text_rect = {(640 - text_width) / 2,   // Center horizontally
                          (480 - text_height) / 2,  // Center vertically
                          text_width, text_height};

    // Copy the texture to the renderer
    SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);

    // Present the changes to the window
    SDL_RenderPresent(renderer);

    /* END writing text "Hello, World!" to screen
     */
    WHBLogPrint("Finished writing Hello, World!");

    int countdown = 10;
    while (countdown > 0) {
        WHBLogPrintf("hello world closing in %d", countdown);
        countdown--;
        SDL_Delay(3000);
    }

    // Clean up
    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(text_surface);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    romfsExit();
    WHBLogUdpDeinit();

    /*  Your exit code doesn't really matter, though that may be changed in
        future. Don't use -3, that's reserved for HBL. */
    return 1;
}
