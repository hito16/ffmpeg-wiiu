/* An SDL WiiU "day one" example to ensure all the build dependencies are
   configured. Assumes you built romfs-wiiu and downloaded fonts

   Nothing will appear on the screen, but you can confirm the setup works
   To see logs, open up a terminal, type nc -ul 4405
*/
#include <SDL.h>
#include <SDL_ttf.h>
#include <romfs-wiiu.h>
#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/proc.h>

int main(int argc, char** argv) {
    // WHBProcInit();
    WHBLogUdpInit();
    WHBLogPrint("Simple SDL Window. Logging initialised.");

    romfsInit();
    SDL_Init(SDL_INIT_VIDEO);

    // Create a window
    SDL_Window* window =
        SDL_CreateWindow("Simple SDL Window", SDL_WINDOWPOS_UNDEFINED,
                         SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);

    if (window == NULL) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    TTF_Init();
    // downloaded google font and placed in ./romfs/res
    TTF_Font* font = TTF_OpenFont("romfs:/res/Roboto-Regular.ttf", 28);
    if (font == NULL) {
        WHBLogPrintf(">> Failed to open file %s",
                     "romfs:/res/Roboto-Regular.ttf");
        WHBLogPrintf("");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    WHBLogPrint("Font is initialized");
    WHBLogPrintf("");
    int countdown = 10;
    while (countdown > 0) {
        /*You should get past the white WIIU loading screen
          and see a black screen.
          Additionally, you should be seeing logs in your udp reader
        */
        WHBLogPrintf("hello world ... closing in %d", countdown);
        WHBLogPrintf("");
        SDL_Delay(3000);
        countdown--;
    }
    // WHBProcShutdown();

    WHBLogPrint("SDL destroy and close");
    WHBLogPrintf("");
    SDL_DestroyWindow(window);
    SDL_Quit();
    romfsExit();

    WHBLogUdpDeinit();

    /*  Your exit code doesn't really matter, though that may be changed in
        future. Don't use -3, that's reserved for HBL. */
    return 1;
}
