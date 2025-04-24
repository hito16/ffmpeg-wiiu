/*
Example of WHBLogConsoleInit & WHBLogPrint from
ProgrammingOnTheU ButtonTester
WHBLogConsoleDraw must be called to draw to the screen.  I overdid it to address
CEMU refresh issues.

 About directories

 /vol/external01 is the CWD of a running app
 On the WiiU, /vol/external01
      is the root of the SD card, and parent directory visibilty is restricted.
 In CEMU,  /vol/external01
     is just part of CEMU virtual storage. You can cd into parent and explore.
     CEMU supports virtual SD  which is a folder on the host. Once you configure
CEMU, the virtual CD folder mapped to /vol/storage_mlc01
 */

#include <coreinit/thread.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <whb/proc.h>

/* Force a draw immediately after WHBLogPrintf
   Nothing will be written to the screen without WHBLogConsoleDraw
*/
int WHBLogPrintfDraw(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = WHBLogPrintf(format, args);
    va_end(args);
    WHBLogConsoleDraw();
    return result;
}

void do_getcwd() {
    char buffer[1024];
    if (getcwd(buffer, 1024) == NULL) {
        WHBLogPrintfDraw(" Error: getcwd() failed");
    } else {
        WHBLogPrintfDraw("== do_getcwd() %s", buffer);
    }
}

void do_readdir(char *targetdir) {
    DIR *dp = NULL;
    struct dirent *ep = NULL;
    int numfiles = 0;

    dp = opendir(targetdir);
    if (dp != NULL) {
        WHBLogPrintfDraw("== do_readdir %s", targetdir);
        while (ep = readdir(dp)) {
            numfiles++;
            if (numfiles < 10) {
                WHBLogPrintfDraw("  %s %s", ep->d_type == DT_DIR ? "D" : " ",
                                 ep->d_name);
            }
        }
    } else {
        WHBLogPrintf("Error: readdir() failed on %s", targetdir);
    }
    closedir(dp);
    WHBLogPrintf(" (%d) entries found (showing first 10)", numfiles);
    WHBLogPrintfDraw("");
    OSSleepTicks(OSMillisecondsToTicks(3000));
}

void do_chdir(char *targetdir) {
    WHBLogPrintfDraw("== do_chdir %s", targetdir);
    if (chdir(targetdir) == -1) {
        WHBLogPrintfDraw("Error: chdir() failed on %s", targetdir);
    } else {
        do_getcwd();
    }
}

int main(int argc, char **argv) {
    WHBProcInit();

    /*  Use the Console backend for WHBLog - this one draws text with OSScreen
        Don't mix with other graphics code! */
    WHBLogConsoleInit();

    int times_left = 5;
    while (WHBProcIsRunning() && times_left > 0) {
        WHBLogPrint("Do directory operations work as expected on WiiU?");
        WHBLogPrint("");
        WHBLogPrint("== Using WBLog Console mode for output (RED screen)");

        OSSleepTicks(OSMillisecondsToTicks(2000));

        char *checkdirs[] = {
            "/vol/external01",     // SD mounted here on WIIU, start dir in both
            "/vol/storage_mlc01",  // SD mounted here on CEMU
            "/vol/",               // only visible on CEMU
            "/"                    // only visible on CEMU
        };
        int access_check[4];

        WHBLogPrintfDraw("== Checking access to interesting directories");
        for (int i = 0; i < 4; i++) {
            access_check[i] = access(checkdirs[i], F_OK);
            WHBLogPrintf("    %s  %s", access_check[i] == 0 ? "y" : "n",
                         checkdirs[i]);
        }
        WHBLogConsoleDraw();
        OSSleepTicks(OSMillisecondsToTicks(2000));

        /*   display CWD and list directory */
        WHBLogPrint("");
        WHBLogPrint("== The starting dir is /vol/external01");
        OSSleepTicks(OSMillisecondsToTicks(3000));
        do_getcwd();
        do_readdir("./");

        /*  attempt to CD and list the parent directory */
        if (access_check[2] == 0) {
            WHBLogPrintfDraw("== Parent dirs are visible in CEMU, not WIIU.");
            do_chdir("/vol");
            do_readdir("/vol");
        }

        /*  attempt to CD and list the root directory */
        if (access_check[3] == 0) {
            do_chdir("/");
            do_readdir("/");
        }

        WHBLogPrintfDraw("");

        /* attempt to read the CEMU SD card mount point*/
        if (access_check[1] == 0) {
            WHBLogPrintfDraw("== In CEMU, your SD card will be mounted here");
            do_readdir("/vol/storage_mlc01");
        }

        /* return to the WIIU homebrew SD card mount point */
        WHBLogPrintfDraw("== return to starting dir /vol/external01");
        do_chdir("/vol/external01");

        times_left--;
        WHBLogPrintf("== done. Running again (attempts = %d)", times_left);
        WHBLogPrintfDraw("");

        OSSleepTicks(OSMillisecondsToTicks(8000));
    }

    /*  If we get here, ProcUI said we should quit. */
    WHBLogPrintfDraw("Shutting Down...");

    WHBLogConsoleFree();
    WHBProcShutdown();

    return 0;
}
