/* A mishmash of coding examples on ffmpeg api site.

WIP - works up to loading a codec.  Before continuing, I need to
build 3rd party codecs such as h264 into ffmpeg for WIIU
 */

#include <coreinit/thread.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <whb/log.h>
#include <whb/log_console.h>

#include "readspeed.h"

#define INBUF_SIZE 4096
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

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

/*
To compile this example, you would typically use a command like:

gcc -o decode_video decode_video.c -lavformat -lavcodec -lavutil -lavresample

Make sure you have FFmpeg development headers and libraries installed.
*/

int av_decode_test(char *input_filename, struct TestResults *res) {
    AVFormatContext *fmt_ctx = NULL;
    AVCodec *codec = NULL;
    AVCodecContext *codec_ctx = NULL;
    int video_stream_index = -1;
    AVFrame *frame = NULL;
    AVPacket *pkt = NULL;
    int ret;
    int i;

    /* avformat_find_stream_info is very slow
    Set the  option BEFORE opening input file. Defaults are currently
      probesize = 5*10^6 "data" bytes?  - only this seems to be set?
      fsprobesize = 5.4*10^13 frames
      format_probesize = 5.4*10^13 bytes
      duration_probesize = ? bytes
     */
    AVDictionary *options = NULL;
    av_dict_set(&options, "probesize", "2048", 0);         // fast!!
    av_dict_set(&options, "fpsprobesize", "1024", 0);      // no effect
    av_dict_set(&options, "formatprobesize", "32768", 0);  // no effect

    ret = avformat_open_input(&fmt_ctx, input_filename, NULL, &options);
    //  1. Open the input file using avformat_open_input.
    //  ret = avformat_open_input(&fmt_ctx, input_filename, NULL, NULL);

    if (ret < 0) {
        WHBLogPrintfDraw("Could not open input file %s\n", input_filename);
        return 1;
    }
    WHBLogConsoleDraw();

    WHBLogPrintfDraw("calling avformat_find_stream_info\n");
    // 2. Find the stream information with avformat_find_stream_info.
    if (avformat_find_stream_info(fmt_ctx, &options) < 0) {
        WHBLogPrintfDraw("Could not find stream information\n");
        avformat_close_input(&fmt_ctx);
        return 1;
    }
    av_dict_free(&options);  // Free the options dictionary

    // 3. Find the video stream.
    for (i = 0; i < fmt_ctx->nb_streams; i++) {
        WHBLogPrintf("  Codec: %s Type: %s",
                     avcodec_get_name(fmt_ctx->streams[i]->codecpar->codec_id),
                     av_get_media_type_string(
                         fmt_ctx->streams[i]->codecpar->codec_type));
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            WHBLogPrintf("  Resolution: %d x %d",
                         fmt_ctx->streams[i]->codecpar->width,
                         fmt_ctx->streams[i]->codecpar->height);
        }
        WHBLogConsoleDraw();
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1) {
        WHBLogPrintfDraw("Could not find a video stream\n");
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 4. Find the decoder for the video stream.
    codec = avcodec_find_decoder(
        fmt_ctx->streams[video_stream_index]->codecpar->codec_id);
    if (codec == NULL) {
        WHBLogPrintfDraw("Could not find decoder\n");
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 5. Allocate a codec context and copy the codec parameters.
    codec_ctx = avcodec_alloc_context3(codec);
    if (codec_ctx == NULL) {
        WHBLogPrintfDraw("Could not allocate codec context\n");
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    if (avcodec_parameters_to_context(
            codec_ctx, fmt_ctx->streams[video_stream_index]->codecpar) < 0) {
        WHBLogPrintfDraw("Could not copy codec parameters to context\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 6. Open the decoder.
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        WHBLogPrintfDraw("Could not open codec\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 7. Allocate an AVPacket for reading the compressed data.
    pkt = av_packet_alloc();
    if (pkt == NULL) {
        WHBLogPrintfDraw("Could not allocate packet\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 8. Allocate an AVFrame for storing the decoded frame data.
    frame = av_frame_alloc();
    if (frame == NULL) {
        WHBLogPrintfDraw("Could not allocate frame\n");
        av_packet_free(&pkt);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    WHBLogPrintfDraw("Decoding VIDEO stream\n");
    int64_t ops = 0;
    int64_t data_sz = 0;
    res->start_time = OSGetTime();
    // 9. Read packets from the input file and decode them.
    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == video_stream_index) {
            // 10. Decode the video frame.
            ret = avcodec_send_packet(codec_ctx, pkt);
            if (ret < 0) {
                WHBLogPrintfDraw("Error sending packet for decoding: %s\n",
                                 av_err2str(ret));
                av_packet_unref(pkt);  // Important: Unref the packet.
                continue;  // Continue to the next packet.  Don't stop
                           // decoding.
            }
            data_sz += pkt->size;
            while ((ret = avcodec_receive_frame(codec_ctx, frame)) >= 0) {
                ops++;
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;  // Need more data or decoder finished.
                } else if (ret < 0) {
                    WHBLogPrintfDraw("Error during decoding: %s\n",
                                     av_err2str(ret));
                    return 1;
                }

                // 11. Process the decoded frame.

                // Important:  av_frame_unref is crucial to release
                // resources
                av_frame_unref(frame);
            }
        }
        av_packet_unref(pkt);  // Unreference the packet after it's used
    }
    res->end_time = OSGetTime();
    res->ops = ops;
    res->data_read = data_sz;

    // 12. Flush the decoder (important for some codecs).
    pkt->data = NULL;
    pkt->size = 0;
    while (avcodec_send_packet(codec_ctx, pkt) >= 0) {
        ret = avcodec_receive_frame(codec_ctx, frame);

        if (ret == 0) {
            WHBLogPrintfDraw(
                "Flush. Frame number: %lld, width: %d, height: %d, "
                "format: %s\n",
                codec_ctx->frame_num, frame->width, frame->height,
                av_get_pix_fmt_name(codec_ctx->pix_fmt));
            av_frame_unref(frame);
        } else if (ret == AVERROR_EOF)
            break;
        else if (ret == AVERROR(EAGAIN))
            continue;
        else if (ret < 0) {
            WHBLogPrintfDraw("Error during flushing: %s\n", av_err2str(ret));
            return 1;
        }
    }

    // 13. Clean up and free resources.
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);

    return 0;
}
