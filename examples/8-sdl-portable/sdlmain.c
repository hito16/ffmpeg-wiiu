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
#include <string.h>

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


    char* mp3filename =
        "mp3/notification-ping-335500-pixabay-dragon-studios-2sec.mp3";
    int len = strlen(mp3filename) + strlen(RES_ROOT);
    char buffer[len];
    strncpy(buffer, RES_ROOT, len);
    strncat(buffer, mp3filename, len);
    printf("attempting to play %s", buffer);
    char* my_argv[] = {
        "sdlaudio1_main",  // argv[0] is the program name.
        buffer,
        NULL  // argv must be NULL-terminated.
    };
    int my_argc = 2;  // Number of arguments in my_argv
    sdlaudio1_main(my_argc, my_argv);

    sdlanimate_main();
    
    printf("exiting main\n");
    return 0;
}
