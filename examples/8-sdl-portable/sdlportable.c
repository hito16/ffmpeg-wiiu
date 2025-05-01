/* A generic SDL homebrew stub application.
   You should be able to compile this on other systems with little to
   no code changes.
   Note: his code doesn't have a dedicated main loop "while ()", and
   and will exit immediately.  It assumes whatever SDL code you
   integrate will have its own SDL event loop.
*/

#ifdef __WIIU__
#include <romfs-wiiu.h>

#include "rsyslog-wiiu.h"
#include "rsyslog.h"

const char* RES_ROOT = "romfs:/res/";  // fonts, images, etc.
#else
const char* RES_ROOT = "./romfs/res/";
#endif

int main(int argc, char** argv) {
#ifdef __WIIU__
    romfsInit();  // a romfs mount with fonts, images etc. on the WiiU
    if (init_rsyslogger() != 0) {
        // setup udp or cafe logging
    }
#endif

    // Call your SDL routine here.
    // For example, take an existing SDL example with a main(),
    // rename the SDL example main() to sdl_main(), and call
    // sdl_main() here.
    return 0;
}
