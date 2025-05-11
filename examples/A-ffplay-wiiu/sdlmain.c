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
#include <whb/log.h>
#include <whb/log_console.h>
#include <whb/proc.h>

#include "exutil.h"
#include "sdlportables.h"

int ffplay_main(int argc, char **argv);

int main(int argc, char **argv) {
#ifdef __WIIU__
    printf("initialized romfs");
#ifdef DEBUG
    // code will block util syslog service Docker is up
    if (init_rsyslogger() != 0) {
        // setup udp or cafe logging
    }
#endif  // DEBUG
#endif  // __WIIU__

    // Something to boot up, clear the screen, wait for any button press.
    // While waiting, bring up syslog window and tail the logs.
    // From example 8, we know we can do SDL_Init and SDL_Quit several times
    // in a row and run SDL UI's back to back.
    sdltriangle_main();

    printf("starting main\n");
    // Call your SDL routine here.
    char buffer[1024];
    if (util_get_first_media_file(buffer, 1024) == 0) {
        printf("found media file %s\n", buffer);
        char *my_argv[] = {
            "ffplay",  // argv[0] is the program name.
            "-v",
            "warning",
            "-fs",  // force full screen
            "-threads",
            "1",  // single threaded?
            "-probesize",
            "1024",  // bytes to detect streams
            "-analyzeduration",
            "0", //"1000000",  // microsec to detect streams
            buffer,     //(char *)filename,
            NULL        // argv must be NULL-terminated.
        };
        int my_argc = 11;  // Number of arguments in my_argv

        // Call ffplay_main
        int result = ffplay_main(my_argc, my_argv);

        // Do something with the result (e.g., check for errors)
        if (result != 0) {
            fprintf(stderr, "ffplay_main failed with error code: %d\n", result);
            sdltriangle_main();
            //  Consider returning a non-zero value from main to indicate
            //  failure
            return 1;
        }
    }

    sdltriangle_main();

    printf("exiting main\n");
    return 0;
}
