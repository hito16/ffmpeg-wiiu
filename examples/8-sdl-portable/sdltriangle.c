/* A generic SDL draw triangle example that compiles for Mac OS,
  and with small changes, compiles on WIIU
*/

//  WIIU code change 1.
//  WUT homebrew pkgconfig includes the SDL sub directories
#ifdef __WIIU__
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include <stdio.h>

#include "sdlportables.h"

/* WIIU code change 2.
   This was originally a stand alone SDL sample.  We will rename main()
   to just a function so we can call it from our generic crossplatorm main()
*/
// int main(int argc, char* argv[]) {
int sdltriangle_main() {
    // SDL window and renderer
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    // Screen dimensions
    const int SCREEN_WIDTH = 640;
    const int SCREEN_HEIGHT = 480;

    // Initialize SDL
    // WIIU code change 3a.  Controller / gamepad support
    // if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("SDL Error: %s\n", SDL_GetError());
        return 1;
    }

    printf("SDL_Init()\n");

    // Create window and renderer
    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN,
                                &window, &renderer);
    if (!window || !renderer) {
        printf("SDL Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    printf("SDL_CreateWindowAndRenderer()\n");

    // Set the background color to black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    printf("SDL_RenderClear()\n");

    // Draw a red triangle
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Red color
    SDL_Point points[4] = {
        {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 4},          // Top point
        {SCREEN_WIDTH / 4, 3 * SCREEN_HEIGHT / 4},      // Bottom-left point
        {3 * SCREEN_WIDTH / 4, 3 * SCREEN_HEIGHT / 4},  // Bottom-right point
        {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 4}           // Return to Top point
    };
    SDL_RenderDrawLines(renderer, points, 4);

    // Update the screen
    SDL_RenderPresent(renderer);
    printf("SDL_RenderPresent()\n");

    // WIIU code change 3b. Controller / gamepad support
    SDL_GameController* pad;
    SDL_bool quit = SDL_FALSE;
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = SDL_TRUE;
    // WIIU code change 3c.
    // Add controller events so we can exit on any button push
            } else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
                quit = SDL_TRUE;
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
    }
    // While this works on the WiiU, the Mac did not render anything
    // SDL_Delay(5000);  // Display for 5 seconds

    // Clean up: Destroy renderer and window, and quit SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("SDL_Quit()\n");

    return 0;
}