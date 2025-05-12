/* A generic SDL homebrew stub application.
   You should be able to compile this on other systems with little to
   no code changes.
   Note: his code doesn't have a dedicated main loop "while ()", and
   and will exit immediately.  It assumes whatever SDL code you
   integrate will have its own SDL event loop.
*/

#ifdef __WIIU__
#include <sysapp/launch.h>

#ifdef DEBUG
#include "rsyslog-wiiu.h"
#include "rsyslog.h"
#endif  // DEBUG

#endif
#include <stdio.h>

#include <whb/proc.h>

#include "exutil.h"
#include "sdlportables.h"

int main(int argc, char** argv) {
#ifdef __WIIU__
    printf("initialized romfs");
#ifdef DEBUG
    if (init_rsyslogger() != 0) {
        // setup udp or cafe logging
    }
#endif  // DEBUG
#endif  // __WIIU__

    WHBProcInit();


    printf("starting main\n");
    // Call your SDL routine here.
    char buffer[1024];
    if (util_get_first_media_file(buffer, 1024) == 0) {
        printf("found media file %s\n", buffer);
        // ffmpeg_sync2_main(buffer);
        //  ffmpeg_decode4_main(buffer);

        char* my_argv[] = {
            "ffmpeg_playvid",  // argv[0] is the program name.
            buffer,
            NULL  // argv must be NULL-terminated.
        };
        int my_argc = 2;  // Number of arguments in my_argv
        ffmpeg_playvid_main(my_argc, my_argv);
    }


    WHBProcShutdown();
    printf("exiting main\n");
    return 0;
}
