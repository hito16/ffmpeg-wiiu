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
#include "rsyslog.h"

static char SYSLOG_IP[18];

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

int find_syslog_ip(char *server_ip_buffer) {
    int port = 9515;
    int i = 0;
    int res = -1;
    for (int i = 0; i < 10; i++) {
        res = client_announce(server_ip_buffer, port);
        if (res == 0) {
            return 0;
        }
    }
    return 1;
}

/*  We use this to pass output from stdout/stderr to our syslog function */
static ssize_t write_msg_to_syslog(struct _reent *r, void *fd, const char *ptr,
                                   size_t len) {
    char buffer[1024];

    snprintf(buffer, 1024, "%*.*s", len, len, ptr);
    rsyslog_send_tcp(SYSLOG_IP, 9514, 14, buffer);
    return len;
}

void init_stdout() {
    /* redirects STDOUT and STDERR to our wrapper */
    static devoptab_t dev;
    dev.name = "STDOUT";
    dev.structSize = sizeof dev;
    dev.write_r = &write_msg_to_syslog;

    devoptab_list[STD_OUT] = &dev;
    devoptab_list[STD_ERR] = &dev;
}

int main(int argc, char **argv) {
    WHBProcInit();

    /*  Use the Console backend for WHBLog - this one draws text with OSScreen
        Don't mix with other graphics code! */
    WHBLogConsoleInit();

    // initialize logging - find syslog ip address
    char server_ip_buffer[18];
    if (find_syslog_ip(server_ip_buffer) == 0) {
        WHBLogPrintfDraw("Found syslog IP %s", server_ip_buffer);
        strncpy(SYSLOG_IP, server_ip_buffer, 17);
    } else {
        WHBLogPrintfDraw("No IP found, Shutting Down...");
        OSSleepTicks(OSMillisecondsToTicks(5000));
        WHBLogConsoleFree();
        WHBProcShutdown();
    }

    WHBLogPrintfDraw("== Starting rsyslog test [%s]...", server_ip_buffer);

    int times_left = 5;
    while (WHBProcIsRunning() && times_left > 0) {
        WHBLogPrintfDraw("== Logging with rsyslog_send_tcp");

        rsyslog_send_tcp(server_ip_buffer, 9514, 14,
                         "Wrote from  rsyslog_send_tcp()");
        times_left--;
        WHBLogPrintf("== done. Running again (attempts = %d)", times_left);
        WHBLogPrintfDraw("");
        OSSleepTicks(OSMillisecondsToTicks(5000));
    }

    /*  The big test.  Where does printf() go??  */
    init_stdout();
    times_left = 5;
    while (WHBProcIsRunning() && times_left > 0) {
        WHBLogPrintfDraw("== Logging with printf...");
        printf("Wrote from printf() -  %d\n", times_left);
        fprintf(stdout, "fprintf(stdout, ...) %d\n", times_left);
        fprintf(stderr, "fprintf(stderr, ...) %d\n", times_left);
        times_left--;
        OSSleepTicks(OSMillisecondsToTicks(8000));
    }

    /*  If we get here, ProcUI said we should quit. */
    WHBLogPrintfDraw("Shutting Down...");

    WHBLogConsoleFree();
    WHBProcShutdown();

    return 0;
}
