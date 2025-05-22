/* Reusable WiiU homebrew app to wrap "main()"

In an ideal situation, our CLI or SDL program can compile and 
run on our Native OS.

With any luck, the "porting" is just calling the program's main 
with this wrapper.

  Step 1. Compile your program in linux, mac, etc. and see it working
      as a normal cli or server.
  Step 2. Get it compiled for Devkitpro PPC w/ WUT.  It won't run yet,
      but you figured out your build dependencies.
  Step 3: In your other program, make these changes to main()
     #ifdef __WIIU__
         int myapp_main(int argc, char* argv[]) {
     #else
         int main(int argc, char* argv[]) {
     #endif
  Step 4: Build myapp_main as a library or just link it in here.
*/

#include <coreinit/thread.h>
#include <sysapp/launch.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <whb/proc.h>

#ifdef DEBUG
#include "rsyslog-wiiu.h"
#include "rsyslog.h"
#endif  // DEBUG

int myapp_main(int my_argc, char** my_argv);

int main(int argc, char** argv) {
    WHBProcInit();

#ifdef DEBUG
    // The app we're calling may spew printfs, so redirect stdout to syslog
    // The code will block here until the syslog service Docker is up.
    if (init_rsyslogger() != 0) {
        // setup udp or cafe logging
    }
#endif  // DEBUG

    WHBLogConsoleInit();
    WHBLogPrint("Generic WiiU wrapper for CLIs or Servers");
    WHBLogConsoleDraw();

    // Step 3: construct args you app needs
    const char* buffer = "Some argv input from the user";
    char* my_argv[] = {
        "myapp",  // argv[0] is the program name.
        (char*)buffer,   //(char *)filename,
        NULL      // argv must be NULL-terminated.
    };
    int my_argc = 2;  // Number of arguments in my_argv

    // ----- Step 4: call your main code here
    myapp_main(my_argc, my_argv);
    // -----

    // To quit cleanly and return to the system window
    int quit = 1;
    while (WHBProcIsRunning()) {
        // for instance, user selected "quit" from a menu
        // you need to implement the quit.
        if (quit) {
            SYSLaunchMenu();  // will cause WHBProcIsRunning() to return
                              // false
        }
    }

    WHBLogConsoleFree();
    return 0;
}
