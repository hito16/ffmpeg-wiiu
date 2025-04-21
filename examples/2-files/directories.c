/*
#include <stdint.h>
#include <stdbool.h>
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
        while (ep = readdir(dp))
        {
            numfiles++;
            WHBLogPrintf("%s %s", ep->d_type == DT_DIR ? "D" : " ", ep->d_name);
        }
        closedir(dp);
    }
    else
    {
        WHBLogPrintf("Error: readdir() failed on %s", targetdir);
    }
    WHBLogPrintf(" (%d) entries found", numfiles);
    WHBLogConsoleDraw();
    OSSleepTicks(OSMillisecondsToTicks(2000));
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

    WHBLogPrint("Do directory operations work as expected on WiiU?");
    WHBLogPrint("");
    WHBLogPrint("== Using WBLog Console mode for output ");
    WHBLogPrint("== ");
    WHBLogConsoleDraw();
    OSSleepTicks(OSMillisecondsToTicks(1000));

    do_getcwd();
    do_chdir("..");
    do_readdir("./");

    do_chdir("/");
    do_readdir("./");

    WHBLogPrint("== done");
    WHBLogConsoleDraw();

    while (WHBProcIsRunning())
    {
        /*  block forever */
    }

    /*  If we get here, ProcUI said we should quit. */
    WHBLogPrint("Shutting Down...");
    WHBLogConsoleDraw();

    WHBLogConsoleFree();
    WHBProcShutdown();

    return 0;
}
