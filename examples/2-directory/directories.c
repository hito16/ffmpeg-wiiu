/*
Example of WHBLogConsoleInit & WHBLogPrint from
ProgrammingOnTheU ButtonTester
WHBLogConsoleDraw must be called to draw to the screen.  I overdid it to address CEMU
refresh issues.
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>

#include <coreinit/thread.h>
#include <whb/log_console.h>
#include <whb/log.h>
#include <whb/proc.h>

void do_getcwd()
{
    char buffer[1024];
    if (getcwd(buffer, 1024) == NULL)
    {
        WHBLogPrint(" Error: getcwd() failed");
    }
    else
    {
        WHBLogPrintf("== do_getcwd() %s", buffer);
    }
    WHBLogConsoleDraw();
}

void do_readdir(char *targetdir)
{
    DIR *dp = NULL;
    struct dirent *ep = NULL;
    int numfiles = 0;

    dp = opendir(targetdir);
    if (dp != NULL)
    {
        WHBLogPrintf("== do_readdir %s", targetdir);
        WHBLogConsoleDraw();
        while (ep = readdir(dp))
        {
            numfiles++;
            WHBLogPrintf("%s %s", ep->d_type == DT_DIR ? "D" : " ", ep->d_name);
            WHBLogConsoleDraw();
        }
        closedir(dp);
    }
    else
    {
        WHBLogPrintf("Error: readdir() failed on %s", targetdir);
    }
    WHBLogPrintf(" (%d) entries found", numfiles);
    WHBLogPrint("");
    WHBLogConsoleDraw();
    OSSleepTicks(OSMillisecondsToTicks(3000));
}

void do_chdir(char *targetdir)
{
    WHBLogPrintf("== do_chdir %s", targetdir);
    WHBLogConsoleDraw();
    chdir(targetdir);
    do_getcwd();
}

int main(int argc, char **argv)
{
    WHBProcInit();

    /*  Use the Console backend for WHBLog - this one draws text with OSScreen
        Don't mix with other graphics code! */
    WHBLogConsoleInit();

    int times_left = 5;
    while (WHBProcIsRunning() && times_left > 0)
    {
        WHBLogPrint("Do directory operations work as expected on WiiU?");
        WHBLogPrint("");
        WHBLogPrint("== Using WBLog Console mode for output (RED screen)");
        WHBLogPrint("== ");
        WHBLogConsoleDraw();
        OSSleepTicks(OSMillisecondsToTicks(3000));

        WHBLogPrint("");
        WHBLogPrint("== on wiiu, you'll see the SD card root (mount /vol/external01)");
        WHBLogPrint("== in cemu, you'll see an empty dir, unless you set up emulated SD");
        WHBLogConsoleDraw();
        OSSleepTicks(OSMillisecondsToTicks(3000));
        do_getcwd();
        do_readdir("./");
        
        WHBLogPrint("== on wiiu, only the SD card root and below are visible");
        WHBLogPrint("== in cemu, this will work.");
        WHBLogConsoleDraw();
        do_chdir("..");
        do_readdir("./");

        do_chdir("/");
        do_readdir("./");

        WHBLogPrint("== return to the SD card root");
        WHBLogConsoleDraw();
        do_chdir("/vol/external01");

        times_left--;
        WHBLogPrintf("== done. runs remaining = %d", times_left);
        WHBLogPrint("");
        WHBLogConsoleDraw();
        OSSleepTicks(OSMillisecondsToTicks(8000));
    }

    /*  If we get here, ProcUI said we should quit. */
    WHBLogPrint("Shutting Down...");
    WHBLogConsoleDraw();

    WHBLogConsoleFree();
    WHBProcShutdown();

    return 0;
}
