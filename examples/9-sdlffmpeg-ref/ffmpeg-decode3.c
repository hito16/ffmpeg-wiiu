#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>

// Struct to hold FFmpeg context
typedef struct {
    AVFormatContext *format_context;
    int video_stream_index;
    AVCodecContext *codec_context;
    AVFrame *frame;
    AVPacket *packet;
} FFmpegContext;

// Function to open the file and find the video stream
int open_file_and_find_stream(const char *input_file, FFmpegContext *ffctx) {
    // Initialize network (if needed)
    avformat_network_init();

    // Open the input file
    if (avformat_open_input(&ffctx->format_context, input_file, NULL, NULL) != 0) {
        fprintf(stderr, "Could not open input file: %s\n", input_file);
        return -1;
    }

    // Find the stream information
    if (avformat_find_stream_info(ffctx->format_context, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return -1;
    }

    // Find the video stream
    ffctx->video_stream_index = -1;
    for (int i = 0; i < ffctx->format_context->nb_streams; i++) {
        if (ffctx->format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            ffctx->video_stream_index = i;
            break;
        }
    }

    if (ffctx->video_stream_index == -1) {
        fprintf(stderr, "Could not find a video stream\n");
        return -1;
    }
    return 0;
}

// Function to find the codec and open the codec context
int find_and_open_codec(FFmpegContext *ffctx) {
    // Get the codec parameters for the video stream
    AVCodecParameters *codec_params = ffctx->format_context->streams[ffctx->video_stream_index]->codecpar;

    // Find the decoder for the video stream
    const AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
    if (codec == NULL) {
        fprintf(stderr, "Could not find a decoder for codec ID %d\n", codec_params->codec_id);
        return -1;
    }

    // Allocate a codec context
    ffctx->codec_context = avcodec_alloc_context3(codec);
    if (ffctx->codec_context == NULL) {
        fprintf(stderr, "Could not allocate codec context\n");
        return -1;
    }

    // Copy codec parameters to codec context
    if (avcodec_parameters_to_context(ffctx->codec_context, codec_params) < 0) {
        fprintf(stderr, "Could not copy codec parameters to codec context\n");
        return -1;
    }

    // Open the codec
    if (avcodec_open2(ffctx->codec_context, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        return -1;
    }
    return 0;
}

// Function to allocate packet and read data.
int allocate_packet_and_read_data(FFmpegContext *ffctx) {
    // Allocate a frame to hold the decoded video data
    ffctx->frame = av_frame_alloc();
    if (ffctx->frame == NULL) {
        fprintf(stderr, "Could not allocate frame\n");
        return -1;
    }
    // Allocate packet
    ffctx->packet = av_packet_alloc();
    if (ffctx->packet == NULL) {
        fprintf(stderr, "Could not allocate packet\n");
        return -1;
    }
    return 0;
}

// Function to free all FFmpeg resources
void free_ffmpeg_resources(FFmpegContext *ffctx) {
    if (ffctx->packet) {
        av_packet_free(&ffctx->packet);
        ffctx->packet = NULL;
    }
    if (ffctx->frame) {
        av_frame_free(&ffctx->frame);
        ffctx->frame = NULL;
    }
    if (ffctx->codec_context) {
        avcodec_free_context(&ffctx->codec_context);
        ffctx->codec_context = NULL;
    }
    if (ffctx->format_context) {
        avformat_close_input(&ffctx->format_context);
        ffctx->format_context = NULL;
    }
    avformat_network_deinit();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return -1;
    }

    const char *input_file = argv[1];
    FFmpegContext ffctx = {0}; // Initialize all fields to NULL
    int ret = 0;

    // 1. Open file and find stream
    if ((ret = open_file_and_find_stream(input_file, &ffctx)) != 0) {
        // Cleanup is handled within the called function.
        free_ffmpeg_resources(&ffctx);
        return ret;
    }
    // 2. Find and open codec
    if ((ret = find_and_open_codec(&ffctx)) != 0) {
        //cleanup
        free_ffmpeg_resources(&ffctx);
        return ret;
    }

    // 3. Allocate packet and read data
    if ((ret = allocate_packet_and_read_data(&ffctx)) != 0) {
        //cleanup
        free_ffmpeg_resources(&ffctx);
        return ret;
    }
    // 4. Read packets and decode video
    while (av_read_frame(ffctx.format_context, ffctx.packet) >= 0) {
        if (ffctx->packet->stream_index == ffctx.video_stream_index) {
            // 4.1. Send the packet to the decoder
            if (avcodec_send_packet(ffctx.codec_context, ffctx.packet) < 0) {
                fprintf(stderr, "Error sending packet to decoder\n");
                av_packet_unref(ffctx.packet);
                continue; // IMPORTANT: Continue to next packet
            }

            // 4.2. Receive frames from the decoder
            int ret;
            while ((ret = avcodec_receive_frame(ffctx.codec_context, ffctx.frame)) >= 0) {
                if (ret == 0) {
                    // A frame was successfully decoded!  Do something with it.
                    printf("Decoded a frame!  Frame size: %d x %d\n", ffctx.frame->width, ffctx->frame->height);
                    // frame->data[0] contains the video data
                    // frame->linesize[0] contains the length of a line (stride) in bytes of the video data.
                    av_frame_unref(ffctx->frame); //VERY IMPORTANT
                } else if (ret == AVERROR(EAGAIN)) {
                    //Output is not available in this state - user must try to send new input
                    break;
                } else if (ret == AVERROR_EOF) {
                    // the decoder has been flushed, and no more output.
                    break;
                } else {
                    fprintf(stderr, "Error during decoding: %s\n", av_err2str(ret));
                    free_ffmpeg_resources(&ffctx);
                    return -1;
                }
            }
        }
        av_packet_unref(ffctx->packet);
    }
    av_packet_free(&ffctx->packet);

    // 5. Flush the decoder (send NULL packet to drain any remaining frames)
    avcodec_send_packet(ffctx.codec_context, NULL);
    while (avcodec_receive_frame(ffctx->codec_context, ffctx.frame) >= 0) {
        printf("Decoded a frame after flush\n");
        av_frame_unref(ffctx->frame);
    }

    // 6. Clean up
    free_ffmpeg_resources(&ffctx);
    return 0;
}

