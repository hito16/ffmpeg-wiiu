/*

== WIP
== DONE, added and tested fread routine,
== TBD, add ffpeg decoder

A simple example comparing freads vs read/decodes of media files on
your SD card

Environment:
On your real WII U SD card or CEMU virtual SD card, create a /media folder.
Add a media file supported by ffmepg to /media

Building:
Build this in docker, as these steps change your DEVKITPRO install
- Assume you already built ffmpeg for WiiU per instructions
- In docker, go to the ffmpeg directory and do make install
- The includes, libs, etc will be installed in $DEVKITPRO/portlibs/ppc

UPDATE: this works on the WIIU, but crashes on CEMU.  TBD: debug
 */

#include "readspeed.h"

#include <coreinit/thread.h>
#include <dirent.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <whb/proc.h>

#include "exutil.h"

int print_test_results(struct TestResults res) {
    double datamb = res.data_read / 1024.0 / 1024;
    long duration = 1 * OSTicksToSeconds(res.end_time - res.start_time);
    WHBLogPrintf("  %.1f MB / %ld secs = %.3f MBps ", datamb, duration,
                 (double)(datamb / duration));
    WHBLogPrintf("  ops: %lld, data: %lld, ops/sec: %ld", res.ops,
                 (long long)res.data_read, (long)(res.ops / duration));
}

int fread_test(char *fname, int blk_sz, struct TestResults *res) {
    unsigned char buffer[blk_sz];
    FILE *fptr;
    uint64_t bytes;
    uint64_t data = 0;
    uint64_t ops = 0;

    // Open the binary file for reading. Assumes you tested for access()
    fptr = fopen(fname, "rb");
    if (fptr == NULL) {
        WHBLogPrint("Error opening file");
        WHBLogConsoleDraw();
        return 1;
    }
    WHBLogPrintf("Starting fread file test with buffer size %ld...", blk_sz);
    WHBLogConsoleDraw();

    res->start_time = OSGetTime();
    while ((bytes = fread(buffer, sizeof(unsigned char), blk_sz, fptr)) > 0) {
        ops++;
        data += bytes;
    }
    res->end_time = OSGetTime();
    fclose(fptr);

    res->ops = ops;
    res->data_read = data;
    print_test_results(*res);
    return 0;
}

void print_times(struct TestResults res) {
    OSCalendarTime tm;
    char tm_buffer[128];
    OSTicksToCalendarTime(res.start_time, &tm);
    WHBLogPrintf("start time %2d:%2d:%2d  ", tm.tm_hour, tm.tm_min, tm.tm_sec);
    OSTicksToCalendarTime(res.end_time, &tm);
    WHBLogPrintf("end time   %2d:%2d:%2d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    WHBLogConsoleDraw();
}

void print_header(char *path_buffer, int64_t st_size) {
    WHBLogPrint("== Compare fread() speeds to media decode speeds to get a ");
    WHBLogPrint("== rough measure of how much of an overhead decoding incurs");
    WHBLogPrintf("==  file: %s", path_buffer);
    WHBLogPrintf("  (%lld MBytes)", (long long)(st_size / 1024 / 1024));
    WHBLogPrint("");
    WHBLogConsoleDraw();
    OSSleepTicks(OSMillisecondsToTicks(1000));
}

int runtests() {
    char path_buffer[1024];
    int ret;

    ret = util_get_first_media_file(path_buffer, 1024);
    if (ret != 0) {
        WHBLogPrint("failed to find a file in sd:/media");
        WHBLogConsoleDraw();
        return ret;
    }

    struct TestResults fread_res = {
        .st_size = util_get_file_size(path_buffer),
        .test_type = 0,
    };

    print_header(path_buffer, fread_res.st_size);

    fread_test(path_buffer, 32768, &fread_res);
    /*
    fread_test(path_buffer, 32768, &fread_res);
    fread_test(path_buffer, 8192, &fread_res);
    fread_test(path_buffer, 8192, &fread_res);
    */
    av_print_codecs();

    struct TestResults avdecode_res = {
        .st_size = util_get_file_size(path_buffer),
        .test_type = 1,
    };
    av_decode_test(path_buffer, &avdecode_res);
    print_test_results(avdecode_res);
    OSSleepTicks(OSMillisecondsToTicks(10000));

    return 0;
}

int main(int argc, char **argv) {
    WHBProcInit();

    /*  Use the Console backend for WHBLog - this one draws text with OSScreen
        Don't mix with other graphics code! */
    WHBLogConsoleInit();

    runtests();

    int times_left = 10;
    while (WHBProcIsRunning() && times_left > 0) {
        times_left--;
        WHBLogPrintf("shutting down in %d seconds...Press and hold Home",
                     times_left * 10);
        WHBLogConsoleDraw();
        OSSleepTicks(OSMillisecondsToTicks(10000));
    }

    /*  If we get here, ProcUI said we should quit. */
    WHBLogPrint("Shutting Down... last chance to press home");
    WHBLogConsoleDraw();

    OSSleepTicks(OSMillisecondsToTicks(10000));

    WHBLogConsoleFree();

    // SYSLaunchMenu();  // aroma
    WHBProcShutdown();

    return 0;
}
