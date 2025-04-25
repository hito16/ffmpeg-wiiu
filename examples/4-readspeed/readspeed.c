/*
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

Test:
pick a compatible media file
load file too buffer with freads test to colleting timings, discard data
load file with ffmpeg libraries, decode video frames, discard data

Sample results:
Test    Filesz Ext  res.    codecs     data read   MBps   ops/sec
fread() 766MB  mp4 720x460  h264, aac  766 MB      12.16  389 reads (32k buffer)
decode  766MB  mp4 720x460  h264, aac  600 MB      0.25   55  frame per second

The MBps is most interesting.
decode  MBps  0.25 or   1/ 0.25 = 4    seconds per 1 MB
fread() MBps 12.16 or   1/12.16 = 0.08 seconds per 1 MB

difference                        3.92 seconds per 1 MB

Before profiling, we can guess for one MB of the file, 4sec time spent is

   0.08 sec      for pure reading data off SD
   3.92 sec      for decoding data in frames

0.08 / 4  = 0.02 or 2% of time spent reading, the rest is decoding.

Decode test notes:
The decode test case only decodes video stream (not audio) and drops the frames
after decoding.  Therefore the decode test processes less data than the fread

Fread() test notes:
1. Multiple back to back runs with the same buffer size reveal the same MBps and
total test duration.
2. I tried different buffer sizes, between 8k and 32k.   There was no change in
MBps or time to read entire file.

This leads me to think there is some underlying file buffering
I don't have control over and/or the underlying block read sizes are fixed,
irrespecetive of the size of my fread() operations.

Impressions:
How viable would a video player be under these conditions?  I picked a 720x460
video close to the WiiU gamepad size 854x480.  This video shoud be
representative of max output quality on the gamepad device.

One the up side,
The 720x460 video did 55 fps, tested by an inexperienced coder, with zero
optimnizations. I did see a project at  GaryOderNichts/FFmpeg-wiiu to integrate
WiiU's dedicated h264 processor. Additionally, GaryOderNichts released a
profiler plugin that may shed light into the delay.

On the down side
We did a fraction of the work.  We didn't decode the audio, we didn't
convert/resize the images or synchronize the audio and video streams, and we
didn't output the frames to any display/audio device.
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
    OSTicksToCalendarTime(res.start_time, &tm);
    WHBLogPrintf("start time %2d:%2d:%2d  ", tm.tm_hour, tm.tm_min, tm.tm_sec);
    OSTicksToCalendarTime(res.end_time, &tm);
    WHBLogPrintf("end time   %2d:%2d:%2d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    WHBLogConsoleDraw();
}

void print_header(char *path_buffer, int64_t st_size) {
    WHBLogPrint("== Compare fread() speeds to media decode speeds  ");
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
