/* An SDL WiiU "day one" example to ensure all the build dependencies are
   configured. Assumes you built romfs-wiiu and downloaded fonts
*/
#include <SDL.h>
#include <SDL_ttf.h>
#include <romfs-wiiu.h>
#include <whb/proc.h>
#ifdef DEBUG
#include <whb/log_udp.h>
#endif

int main(int argc, char** argv) {
    TTF_Font* font = NULL;

#ifdef DEBUG
    WHBLogUdpInit();
#endif

    romfsInit();

    SDL_Init(SDL_INIT_EVERYTHING);

    TTF_Init();
    // downloaded google font and placed in ./romfs/res
    font = TTF_OpenFont("romfs:/res/Roboto-Regular.ttf", 28);
    WHBProcInit();
    while (WHBProcIsRunning()) {
    }
    WHBProcShutdown();

    SDL_Quit();
    romfsExit();

#ifdef DEBUG
    WHBLogUdpDeinit();
#endif

    /*  Your exit code doesn't really matter, though that may be changed in
        future. Don't use -3, that's reserved for HBL. */
    return 1;
}
