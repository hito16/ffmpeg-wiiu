#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return -1;
    }

    const char *input_file = argv[1];

    // 1. Initialize network (if needed)
    avformat_network_init();

    // 2. Open the input file
    AVFormatContext *format_context = NULL;
    if (avformat_open_input(&format_context, input_file, NULL, NULL) != 0) {
        fprintf(stderr, "Could not open input file: %s\n", input_file);
        return -1;
    }

    // 3. Find the stream information
    if (avformat_find_stream_info(format_context, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        avformat_close_input(&format_context);
        return -1;
    }

    // 4. Find the video stream
    int video_stream_index = -1;
    for (int i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        fprintf(stderr, "Could not find a video stream\n");
        avformat_close_input(&format_context);
        return -1;
    }

    // 5. Get the codec parameters for the video stream
    AVCodecParameters *codec_params = format_context->streams[video_stream_index]->codecpar;

    // 6. Find the decoder for the video stream
    const AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
    if (codec == NULL) {
        fprintf(stderr, "Could not find a decoder for codec ID %d\n", codec_params->codec_id);
        avformat_close_input(&format_context);
        return -1;
    }

    // 7. Allocate a codec context
    AVCodecContext *codec_context = avcodec_alloc_context3(codec);
    if (codec_context == NULL) {
        fprintf(stderr, "Could not allocate codec context\n");
        avformat_close_input(&format_context);
        return -1;
    }

    // 8. Copy codec parameters to codec context
    if (avcodec_parameters_to_context(codec_context, codec_params) < 0) {
        fprintf(stderr, "Could not copy codec parameters to codec context\n");
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return -1;
    }

    // 9. Open the codec
    if (avcodec_open2(codec_context, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return -1;
    }

    // 10. Allocate a frame to hold the decoded video data
    AVFrame *frame = av_frame_alloc();
    if (frame == NULL) {
        fprintf(stderr, "Could not allocate frame\n");
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return -1;
    }

    // 11. Read packets and decode video
    AVPacket *packet = av_packet_alloc();
    if (packet == NULL)
    {
        fprintf(stderr, "Could not allocate packet\n");
        av_frame_free(&frame);
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return -1;
    }
    while (av_read_frame(format_context, packet) >= 0) {
        if (packet->stream_index == video_stream_index) {
            // 11.1. Send the packet to the decoder
            if (avcodec_send_packet(codec_context, packet) < 0) {
                fprintf(stderr, "Error sending packet to decoder\n");
                av_packet_unref(packet);
                continue; // IMPORTANT: Continue to next packet
            }

            // 11.2. Receive frames from the decoder
            int ret;
            while ((ret = avcodec_receive_frame(codec_context, frame)) >= 0) {
                if (ret == 0) {
                    // A frame was successfully decoded!  Do something with it.
                    printf("Decoded a frame!  Frame size: %d x %d\n", frame->width, frame->height);
                    // frame->data[0] contains the video data
                    // frame->linesize[0] contains the length of a line (stride) in bytes of the video data.
                    av_frame_unref(frame); //VERY IMPORTANT
                }
                 else if (ret == AVERROR(EAGAIN)) {
                    //Output is not available in this state - user must try to send new input
                    break;
                }
                else if (ret == AVERROR_EOF)
                {
                    // the decoder has been flushed, and no more output.
                    break;
                }
                else {
                    fprintf(stderr, "Error during decoding: %s\n", av_err2str(ret));
                    return -1;
                }
            }
        }
        av_packet_unref(packet);
    }
    av_packet_free(&packet);

    // 12. Flush the decoder (send NULL packet to drain any remaining frames)
    avcodec_send_packet(codec_context, NULL);
    while (avcodec_receive_frame(codec_context, frame) >= 0) {
        printf("Decoded a frame after flush\n");
        av_frame_unref(frame);
    }

    // 13. Clean up
    av_frame_free(&frame);
    avcodec_free_context(&codec_context);
    avformat_close_input(&format_context);
    avformat_network_deinit();
    return 0;
}


