/* A generic SDL homebrew stub application.
   You should be able to compile this on other systems with little to
   no code changes.
   Note: his code doesn't have a dedicated main loop "while ()", and
   and will exit immediately.  It assumes whatever SDL code you
   integrate will have its own SDL event loop.
*/

#ifdef __WIIU__
#include <romfs-wiiu.h>
#include <sysapp/launch.h>

#ifdef DEBUG
#include "rsyslog-wiiu.h"
#include "rsyslog.h"
#endif  // DEBUG

const char* RES_ROOT = "romfs:/res/";  // fonts, images, etc.
#else
const char* RES_ROOT = "./romfs/res/";
#endif
#include <stdio.h>

#include "sdlportables.h"

int main(int argc, char** argv) {
#ifdef __WIIU__
    romfsInit();  // a romfs mount with fonts, images etc. on the WiiU
    printf("initialized romfs");
#ifdef DEBUG
    if (init_rsyslogger() != 0) {
        // setup udp or cafe logging
    }
#endif  // DEBUG
#endif  // __WIIU__

    printf("starting main\n");
    // Call your SDL routine here.

    sdltriangle_main();
    sdlanimate_main();

    printf("exiting main\n");
    return 0;
}
