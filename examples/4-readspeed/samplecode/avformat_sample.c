/* WIB - one of the many coding examples on ffmpeg api.    TBD, integrate this into 
  the readspeed example code.
*/

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/samplefmt.h>
#include <libavutil/imgutils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INBUF_SIZE 4096
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

/* Print out the steps to compile.  Use this as a comment in the code */
/*
To compile this example, you would typically use a command like:

gcc -o decode_video decode_video.c -lavformat -lavcodec -lavutil -lavresample

Make sure you have FFmpeg development headers and libraries installed.
*/

int main(int argc, char *argv[]) {
    AVFormatContext *fmt_ctx = NULL;
    AVCodec *codec = NULL;
    AVCodecContext *codec_ctx = NULL;
    int video_stream_index = -1;
    AVFrame *frame = NULL;
    AVPacket *pkt = NULL;
    int ret;
    const char *input_filename = NULL;
    int i;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    input_filename = argv[1];

    // 1. Open the input file using avformat_open_input.
    if (avformat_open_input(&fmt_ctx, input_filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open input file %s\n", input_filename);
        return 1;
    }

    // 2. Find the stream information with avformat_find_stream_info.
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 3. Find the video stream.
    for (i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1) {
        fprintf(stderr, "Could not find a video stream\n");
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 4. Find the decoder for the video stream.
    codec = avcodec_find_decoder(fmt_ctx->streams[video_stream_index]->codecpar->codec_id);
    if (codec == NULL) {
        fprintf(stderr, "Could not find decoder\n");
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 5. Allocate a codec context and copy the codec parameters.
    codec_ctx = avcodec_alloc_context3(codec);
    if (codec_ctx == NULL) {
        fprintf(stderr, "Could not allocate codec context\n");
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    if (avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[video_stream_index]->codecpar) < 0) {
        fprintf(stderr, "Could not copy codec parameters to context\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }
    // 6. Open the decoder.
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 7. Allocate an AVPacket for reading the compressed data.
    pkt = av_packet_alloc();
    if (pkt == NULL) {
        fprintf(stderr, "Could not allocate packet\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 8. Allocate an AVFrame for storing the decoded frame data.
    frame = av_frame_alloc();
    if (frame == NULL) {
        fprintf(stderr, "Could not allocate frame\n");
        av_packet_free(&pkt);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 9. Read packets from the input file and decode them.
    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == video_stream_index) {
            // 10. Decode the video frame.
            ret = avcodec_send_packet(codec_ctx, pkt);
            if (ret < 0) {
                fprintf(stderr, "Error sending packet for decoding: %s\n", av_err2str(ret));
                av_packet_unref(pkt); // Important: Unref the packet.
                continue; // Continue to the next packet.  Don't stop decoding.
            }

            while ((ret = avcodec_receive_frame(codec_ctx, frame)) >= 0) {
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break; // Need more data or decoder finished.
                } else if (ret < 0) {
                    fprintf(stderr, "Error during decoding: %s\n", av_err2str(ret));
                    return 1;
                }

                // 11. Process the decoded frame.
                printf("Frame decoded! Frame number: %lld, width: %d, height: %d, format: %s\n",
                       codec_ctx->frame_number, frame->width, frame->height,
                       av_get_pix_fmt_name(codec_ctx->pix_fmt));

                // Important:  av_frame_unref is crucial to release resources
                av_frame_unref(frame);
            }
        }
        av_packet_unref(pkt); // Unreference the packet after it's used
    }

    // 12. Flush the decoder (important for some codecs).
    pkt->data = NULL;
    pkt->size = 0;
    while (avcodec_send_packet(codec_ctx, pkt) >= 0) {
        ret = avcodec_receive_frame(codec_ctx, frame);
        if (ret == 0)
        {
             printf("Frame decoded! Frame number: %lld, width: %d, height: %d, format: %s\n",
                       codec_ctx->frame_number, frame->width, frame->height,
                       av_get_pix_fmt_name(codec_ctx->pix_fmt));
             av_frame_unref(frame);
        }
        else if (ret == AVERROR_EOF)
            break;
        else if (ret == AVERROR(EAGAIN))
            continue;
        else if (ret < 0)
        {
             fprintf(stderr, "Error during flushing: %s\n", av_err2str(ret));
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
