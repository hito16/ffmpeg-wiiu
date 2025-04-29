/*

 */

#include <coreinit/thread.h>
#include <stdarg.h>
#include <stdio.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <whb/proc.h>

#include "rsyslog.h"

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

int main(int argc, char **argv) {
    WHBProcInit();

    /*  Use the Console backend for WHBLog - this one draws text with OSScreen
        Don't mix with other graphics code! */
    WHBLogConsoleInit();
    WHBLogPrintfDraw("== Starting rsyslog test");

    int times_left = 5;
    while (WHBProcIsRunning() && times_left > 0) {
        WHBLogPrintfDraw("== Logging");
        rsyslog_send_tcp("192.168.0.97", 9514, 14, "Wrote fron the WiiU");
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
