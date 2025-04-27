/* An SDL WiiU "day one" example to ensure all the build dependencies are
   configured. Assumes you built romfs-wiiu and downloaded fonts
*/
#include <SDL.h>
#include <SDL_ttf.h>
#include <romfs-wiiu.h>
#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/proc.h>

int main(int argc, char** argv) {
    TTF_Font* font = NULL;

    romfsInit();
    SDL_Init(SDL_INIT_EVERYTHING);

    // WHBProcInit();
    WHBLogUdpInit();

    TTF_Init();
    // downloaded google font and placed in ./romfs/res
    font = TTF_OpenFont("romfs:/res/Roboto-Regular.ttf", 28);
    while (WHBProcIsRunning()) {
        /*You should get past the white WIIU loading screen
          and see a black screen.
          Additionally, you should be seeing logs in your udp reader
        */
        WHBLogPrint("hello world");
    }
    WHBProcShutdown();

    SDL_Quit();
    romfsExit();

    WHBLogUdpDeinit();

    /*  Your exit code doesn't really matter, though that may be changed in
        future. Don't use -3, that's reserved for HBL. */
    return 1;
}
