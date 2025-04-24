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

#include <coreinit/thread.h>
#include <dirent.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <whb/proc.h>

// from retroarch
#define ticks_to_sec(ticks) \
    (((uint64_t)(ticks)) / (17 * 13 * 5 * 5 * 5 * 5 * 5 * 5 * 3 * 3 * 2))

struct Results {
    int32_t filesize;
    int test_type; /* 0 file, 1 decode */
    uint64_t ops;  /* freads vs packet decodes */
    uint64_t data_read;
    uint64_t start_time;
    uint64_t end_time;
};

const int BLK_SZ = 32768;

/*   debugging only
void custom_av_log(void *ptr, int level, const char *fmt, va_list vl)
{
    FILE *fp = fopen("/vol/storage_mlc01/media/av_log.txt", "a+");
    if (fp)
    {
        vfprintf(fp, fmt, vl);
        fflush(fp);
        fclose(fp);
    }
}
*/

int get_first_filename(char *buffer, int size) {
    /* try to find /media in the WIIU SD mount, then the CEMU SD mount */
    char *sdmounts[] = {
        "/vol/external01/media",
        "/vol/storage_mlc01/media",
    };
    int sd_idx = (access(sdmounts[0], F_OK) == 0)   ? 0
                 : (access(sdmounts[1], F_OK) == 0) ? 1
                                                    : -1;
    if (sd_idx < 0) {
        return -1;
    }

    DIR *dp = NULL;
    struct dirent *ep = NULL;

    dp = opendir(sdmounts[sd_idx]);
    if (dp != NULL) {
        while (ep = readdir(dp)) {
            if (ep->d_type == DT_REG) {
                snprintf(buffer, size, "%s/%s", sdmounts[sd_idx], ep->d_name);
                return 0;
            }
        }
        closedir(dp);
    }
    return -2;
}

int64_t get_file_size(char *filename) {
    struct stat f_stat;
    if (stat(filename, &f_stat) < 0) {
        return -1;
    }
    return f_stat.st_size;
}

int fread_file(char *fname, int blk_sz, struct Results *res) {
    unsigned char buffer[blk_sz];
    FILE *fptr;
    uint64_t ops = 0;
    uint64_t data = 0;

    WHBLogConsoleDraw();

    // Open the binary file for reading. Assumes you tested for access()
    fptr = fopen(fname, "rb");
    WHBLogConsoleDraw();
    if (fptr == NULL) {
        WHBLogPrint("Error opening file");
        WHBLogConsoleDraw();
        return 1;
    }
    WHBLogPrintf("Starting fread file test with buffer size %ld", blk_sz);
    WHBLogConsoleDraw();

    res->start_time = OSGetTime();
    int64_t bytes_read;
    while ((bytes_read = fread(buffer, sizeof(unsigned char), blk_sz, fptr)) >
           0) {
        ops++;
        data += bytes_read;
    }
    res->end_time = OSGetTime();

    fclose(fptr);

    res->ops = ops;
    res->data_read = data;

    double datamb = (double)(data / 1024 / 1024);
    WHBLogPrintf("ops  %lld, data %lld ", res->ops, (long long)res->data_read);
    WHBLogPrintf("data MB %.3f MB,  %.3f MBps ", datamb,
                 (double)(datamb / (long long)OSTicksToSeconds(
                                       res->end_time - res->start_time)));

    return 0;
}

void print_times(struct Results res) {
    OSCalendarTime tm;
    char tm_buffer[128];
    OSTicksToCalendarTime(res.start_time, &tm);
    WHBLogPrintf("start time %2d:%2d:%2d  ", tm.tm_hour, tm.tm_min, tm.tm_sec);
    OSTicksToCalendarTime(res.end_time, &tm);
    WHBLogPrintf("end time   %2d:%2d:%2d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    WHBLogConsoleDraw();
}

int runtests() {
    char path_buffer[1024];
    int64_t f_size;
    int ret;

    ret = get_first_filename(path_buffer, 1024);
    if (ret != 0) {
        WHBLogPrint("failed to find a file in sd:/media");
        WHBLogConsoleDraw();
        return ret;
    }

    struct Results fread_res;

    f_size = get_file_size(path_buffer);
    WHBLogPrintf("==  file: %s", path_buffer);
    WHBLogPrintf("  (%lld MBytes)", (long long)(f_size / 1024 / 1024));
    WHBLogPrint("");
    WHBLogConsoleDraw();

    OSSleepTicks(OSMillisecondsToTicks(1000));

    fread_file(path_buffer, 32768, &fread_res);
    print_times(fread_res);

    fread_file(path_buffer, 32768, &fread_res);
    print_times(fread_res);

    fread_file(path_buffer, 32768, &fread_res);
    print_times(fread_res);

    WHBLogPrintf("==  file: %s", path_buffer);
    WHBLogPrintf("  (%lld MBytes)", (long long)(f_size / 1024 / 1024));
    WHBLogPrint("");
    WHBLogConsoleDraw();

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
    WHBProcShutdown();

    return 0;
}
