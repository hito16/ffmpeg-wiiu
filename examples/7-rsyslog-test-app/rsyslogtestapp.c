/* An example of headless logging with some added realiablity and ease of use.

rsyslog function logs TCP messages to a syslogd server running in a Docker
container.

  1. Per README.md, start the Dockerfile.rsyslogd and
  2. make SYSLOG_IP=<the ip syslogd is running on>
     ex. make SYSLOG_SERVER=192.168.0.97
  3. wiiload <this file>.rpx
  4. Per README.md, open a termial to the docker instance and tail the generated
     output.
 */

#include <coreinit/thread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/iosupport.h>  // devoptab_list, devoptab_t
#include <whb/log.h>
#include <whb/log_console.h>
#include <whb/proc.h>

#include "announce.h"
#include "rsyslog-wiiu.h"
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

    // initialize logging - find syslog ip address
    if (init_rsyslogger() == 0) {
        WHBLogPrintfDraw("Found syslog IP %s", SYSLOG_IP);
    } else {
        WHBLogPrintfDraw("No IP found, Shutting Down...");
        OSSleepTicks(OSMillisecondsToTicks(5000));
        WHBLogConsoleFree();
        WHBProcShutdown();
    }

    WHBLogPrintfDraw("== Starting rsyslog test [%s]...", SYSLOG_IP);

    int times_left = 5;
    while (WHBProcIsRunning() && times_left > 0) {
        WHBLogPrintfDraw("== Logging with rsyslog_send_tcp");

        rsyslog_send_tcp(SYSLOG_IP, 9514, 14, "Wrote from  rsyslog_send_tcp()");
        times_left--;
        WHBLogPrintf("== done. Running again (attempts = %d)", times_left);
        WHBLogPrintfDraw("");
        OSSleepTicks(OSMillisecondsToTicks(1000));
    }

    /*  The big test.  Where does printf() go??  */

    times_left = 5;
    while (WHBProcIsRunning() && times_left > 0) {
        WHBLogPrintfDraw("== Logging with printf...");
        printf("Wrote from printf() -  %d\n", times_left);
        fprintf(stdout, "fprintf(stdout, ...) %d\n", times_left);
        fprintf(stderr, "fprintf(stderr, ...) %d\n", times_left);
        times_left--;
        OSSleepTicks(OSMillisecondsToTicks(3000));
    }

    /*  If we get here, ProcUI said we should quit. */
    WHBLogPrintfDraw("Shutting Down...");

    WHBLogConsoleFree();
    WHBProcShutdown();

    return 0;
}
