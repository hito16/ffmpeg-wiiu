/*
A simple example showing we can compile and run some basic ffmpeg
routines on the WiiU.

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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>

#include <libavformat/avformat.h>

#include <coreinit/thread.h>
#include <whb/log_console.h>
#include <whb/log.h>
#include <whb/proc.h>

const char *SD_WIIU = "/vol/external01/media";
const char *SD_CEMU = "/vol/storage_mlc01/media";

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

/* TBD migrate to FFMPEG avio_read_dir which does this.  */
bool get_first_filename(const char *targetdir, char *buffer, int size)
{
    DIR *dp = NULL;
    struct dirent *ep = NULL;
    bool found = false;

    dp = opendir(targetdir);
    if (dp != NULL)
    {
        WHBLogPrintf("== Searching for first file in %s", targetdir);
        WHBLogConsoleDraw();
        while ((ep = readdir(dp)) && !found)
        {
            if (ep->d_type == DT_REG)
            {
                snprintf(buffer, size, "%s/%s", targetdir, ep->d_name);
                found = true;
                WHBLogPrintf("== found %s", buffer);
                WHBLogConsoleDraw();
            }
        }
        closedir(dp);
    }
    else
    {
        WHBLogPrintf("Error: readdir() failed on %s", targetdir);
    }
    WHBLogPrint("");
    WHBLogConsoleDraw();

    return found;
}

long get_file_size(char *filename)
{
    struct stat f_stat;
    if (stat(filename, &f_stat) < 0)
    {
        return -1;
    }

    return f_stat.st_size;
}

int print_avformat_stream_info()
{
    char path_buffer[1024];
    bool found = false;
    AVFormatContext *fmt_ctx = NULL;
    int ret;

    found = get_first_filename(SD_WIIU, path_buffer, sizeof(path_buffer)) ||
            get_first_filename(SD_CEMU, path_buffer, sizeof(path_buffer));
    if (!found)
    {
        return -1;
    }
    long f_size = get_file_size(path_buffer);
    WHBLogPrintf("  (%d MBytes)", f_size / 1024 / 1024);
    WHBLogPrint("");
    WHBLogConsoleDraw();

    /* av_log_set_callback(custom_av_log);   // for debugging   */

    /* per api docs, avformat_open_input shouldn't need this, but CEMU crashes w/o */
    fmt_ctx = avformat_alloc_context();
    if ((ret = avformat_open_input(&fmt_ctx, path_buffer, NULL, NULL)) < 0)
    {
        WHBLogPrint("avformat_open_input Cannot open input file");
        return ret;
    }
    WHBLogPrintf("= File Format: %s\n", fmt_ctx->iformat->long_name);
    WHBLogConsoleDraw();

    WHBLogPrint("= opened input, finding stream info");
    WHBLogConsoleDraw();

    if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0)
    {
        WHBLogPrint("avformat_find_stream_info Cannot find stream info");
        return ret;
    }
    WHBLogPrint("= found stream info, detecting streams");
    WHBLogConsoleDraw();
    WHBLogPrintf("= number of streams %d", fmt_ctx->nb_streams);
    WHBLogConsoleDraw();
    OSSleepTicks(OSMillisecondsToTicks(1000));

    /*   debugging
    char *dump = "file:/vol/storage_mlc01/media/av_dump.txt";
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++)
    {
        av_dump_format(fmt_ctx, i, dump, 0);
        WHBLogPrintf("= dumped to %s", dump);
        WHBLogConsoleDraw();
    }
    */

    OSSleepTicks(OSMillisecondsToTicks(1000));
    // Loop through all streams
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++)
    {
        WHBLogPrint("");
        WHBLogPrintf("Stream #%d:", i);
        WHBLogConsoleDraw();

        AVStream *stream = fmt_ctx->streams[i];
        if (stream)
        {
            WHBLogPrintf("Stream #%d: not null ", i);
            WHBLogConsoleDraw();
            OSSleepTicks(OSMillisecondsToTicks(1000));

            /* in CEMU, dereferencing codecpar causes a crash */
            AVCodecParameters *codec_par = stream->codecpar;
            if (codec_par)
            {
                // Print codec information
                WHBLogPrintf("  Codec: %s", avcodec_get_name(codec_par->codec_id));
                WHBLogPrintf("  Type: %s", av_get_media_type_string(codec_par->codec_type));
                WHBLogConsoleDraw();
                OSSleepTicks(OSMillisecondsToTicks(1000));

                // Print additional information based on codec type
                if (codec_par->codec_type == AVMEDIA_TYPE_VIDEO)
                {
                    WHBLogPrintf("  Resolution: %d x %d", codec_par->width, codec_par->height);
                }
                else if (codec_par->codec_type == AVMEDIA_TYPE_AUDIO)
                {
                    WHBLogPrintf("  Sample rate: %d Hz", codec_par->sample_rate);
                }
                WHBLogConsoleDraw();
            }
        }
    }

    OSSleepTicks(OSMillisecondsToTicks(1000));

    // Close the file
    avformat_close_input(&fmt_ctx);
    return 0;
}

int main(int argc, char **argv)
{

    WHBProcInit();

    /*  Use the Console backend for WHBLog - this one draws text with OSScreen
        Don't mix with other graphics code! */
    WHBLogConsoleInit();

    int ret = print_avformat_stream_info();
    WHBLogPrintf("print_avformat_stream_info() exited with code %d", ret);
    WHBLogConsoleDraw();

    int times_left = 10;
    while (WHBProcIsRunning() && times_left > 0)
    {
        times_left--;
        WHBLogPrintf("shutting down in %d seconds", times_left * 10);
        WHBLogConsoleDraw();
        OSSleepTicks(OSMillisecondsToTicks(10000));
    }

    /*  If we get here, ProcUI said we should quit. */
    WHBLogPrint("Shutting Down...");
    OSSleepTicks(OSMillisecondsToTicks(1000));
    WHBLogConsoleDraw();

    WHBLogConsoleFree();
    WHBProcShutdown();

    return 0;
}
