/*
Example of WHBLogConsoleInit & WHBLogPrint from
ProgrammingOnTheU ButtonTester
WHBLogConsoleDraw must be called to draw to the screen.  I overdid it to address CEMU
refresh issues.

 About directories
 
 /vol/external01 is the CWD of a running app
 On the WiiU, /vol/external01
      is the root of the SD card, and parent directory visibilty is restricted.
 In CEMU,  /vol/external01
     is just part of CEMU virtual storage. You can cd into parent and explore.
     CEMU supports virtual SD  which is a folder on the host. Once you configure CEMU,
     the virtual CD folder mapped to /vol/storage_mlc01 
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
    if(chdir(targetdir) == -1) {
        WHBLogPrintf("Error: chdir() failed on %s", targetdir);
        WHBLogConsoleDraw();
    } else {
        do_getcwd(); 
    }
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
        WHBLogPrint("== on wiiu, you'll see the SD card root (/vol/external01)");
        WHBLogPrint("== in cemu, you'll see a 'wiiu' folder");
        WHBLogConsoleDraw();
        OSSleepTicks(OSMillisecondsToTicks(3000));
        do_getcwd();
        do_readdir("./");
        
        WHBLogPrint("== Parent directories are only visible in CEMU, not the WIIU.");
        WHBLogConsoleDraw();
        do_chdir("/vol");
        do_readdir("/vol");

        do_chdir("/");
        do_readdir("/");

        WHBLogPrint("");
        WHBLogConsoleDraw();
        
        WHBLogPrint("== In CEMU, your SD card root will be mounted here");
        do_readdir("/vol/storage_mlc01");

        WHBLogPrint("== return to /vol/external01");
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
