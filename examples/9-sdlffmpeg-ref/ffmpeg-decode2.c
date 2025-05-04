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
int open_file_and_find_stream(const char *input_file, FFmpegContext *ffmpeg_context) {
    // Initialize network (if needed)
    avformat_network_init();

    // Open the input file
    if (avformat_open_input(&ffmpeg_context->format_context, input_file, NULL, NULL) != 0) {
        fprintf(stderr, "Could not open input file: %s\n", input_file);
        return -1;
    }

    // Find the stream information
    if (avformat_find_stream_info(ffmpeg_context->format_context, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        avformat_close_input(&ffmpeg_context->format_context);
        ffmpeg_context->format_context = NULL; // important for cleanup
        return -1;
    }

    // Find the video stream
    ffmpeg_context->video_stream_index = -1;
    for (int i = 0; i < ffmpeg_context->format_context->nb_streams; i++) {
        if (ffmpeg_context->format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            ffmpeg_context->video_stream_index = i;
            break;
        }
    }

    if (ffmpeg_context->video_stream_index == -1) {
        fprintf(stderr, "Could not find a video stream\n");
        avformat_close_input(&ffmpeg_context->format_context);
        ffmpeg_context->format_context = NULL; // important for cleanup
        return -1;
    }
    return 0;
}

// Function to find the codec and open the codec context
int find_and_open_codec(FFmpegContext *ffmpeg_context) {
    // Get the codec parameters for the video stream
    AVCodecParameters *codec_params = ffmpeg_context->format_context->streams[ffmpeg_context->video_stream_index]->codecpar;

    // Find the decoder for the video stream
    const AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
    if (codec == NULL) {
        fprintf(stderr, "Could not find a decoder for codec ID %d\n", codec_params->codec_id);
        avformat_close_input(&ffmpeg_context->format_context);
        ffmpeg_context->format_context = NULL;
        return -1;
    }

    // Allocate a codec context
    ffmpeg_context->codec_context = avcodec_alloc_context3(codec);
    if (ffmpeg_context->codec_context == NULL) {
        fprintf(stderr, "Could not allocate codec context\n");
        avformat_close_input(&ffmpeg_context->format_context);
        ffmpeg_context->format_context = NULL;
        return -1;
    }

    // Copy codec parameters to codec context
    if (avcodec_parameters_to_context(ffmpeg_context->codec_context, codec_params) < 0) {
        fprintf(stderr, "Could not copy codec parameters to codec context\n");
        avcodec_free_context(&ffmpeg_context->codec_context);
        ffmpeg_context->codec_context = NULL; // important
        avformat_close_input(&ffmpeg_context->format_context);
        ffmpeg_context->format_context = NULL;
        return -1;
    }

    // Open the codec
    if (avcodec_open2(ffmpeg_context->codec_context, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        avcodec_free_context(&ffmpeg_context->codec_context);
        ffmpeg_context->codec_context = NULL; // important
        avformat_close_input(&ffmpeg_context->format_context);
        ffmpeg_context->format_context = NULL;
        return -1;
    }
    return 0;
}

// Function to allocate packet and read data.
int allocate_packet_and_read_data(FFmpegContext *ffmpeg_context) {
     // Allocate a frame to hold the decoded video data
    ffmpeg_context->frame = av_frame_alloc();
    if (ffmpeg_context->frame == NULL) {
        fprintf(stderr, "Could not allocate frame\n");
        avcodec_free_context(&ffmpeg_context->codec_context);
        avformat_close_input(&ffmpeg_context->format_context);
        ffmpeg_context->codec_context = NULL;
        ffmpeg_context->format_context = NULL;
        return -1;
    }
      // Allocate packet
    ffmpeg_context->packet = av_packet_alloc();
    if (ffmpeg_context->packet == NULL)
    {
        fprintf(stderr, "Could not allocate packet\n");
        av_frame_free(&ffmpeg_context->frame);
        avcodec_free_context(&ffmpeg_context->codec_context);
        avformat_close_input(&ffmpeg_context->format_context);
        ffmpeg_context->frame = NULL;
        ffmpeg_context->codec_context = NULL;
        ffmpeg_context->format_context = NULL;
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return -1;
    }

    const char *input_file = argv[1];
    FFmpegContext ffmpeg_context = {0}; // Initialize all fields to NULL

    // 1. Open file and find stream
    if (open_file_and_find_stream(input_file, &ffmpeg_context) != 0) {
        // Cleanup is handled within the called function.
        return -1;
    }
     // 2. Find and open codec
    if (find_and_open_codec(&ffmpeg_context) != 0)
    {
        //cleanup
        if(ffmpeg_context.format_context)
            avformat_close_input(&ffmpeg_context.format_context);
        return -1;
    }

     // 3. Allocate packet and read data
    if (allocate_packet_and_read_data(&ffmpeg_context) != 0)
    {
        //cleanup
        if(ffmpeg_context.codec_context)
            avcodec_free_context(&ffmpeg_context.codec_context);
        if(ffmpeg_context.format_context)
            avformat_close_input(&ffmpeg_context.format_context);
        return -1;
    }
    // 4. Read packets and decode video
    while (av_read_frame(ffmpeg_context.format_context, ffmpeg_context.packet) >= 0) {
        if (ffmpeg_context.packet->stream_index == ffmpeg_context.video_stream_index) {
            // 4.1. Send the packet to the decoder
            if (avcodec_send_packet(ffmpeg_context.codec_context, ffmpeg_context.packet) < 0) {
                fprintf(stderr, "Error sending packet to decoder\n");
                av_packet_unref(ffmpeg_context.packet);
                continue; // IMPORTANT: Continue to next packet
            }

            // 4.2. Receive frames from the decoder
            int ret;
            while ((ret = avcodec_receive_frame(ffmpeg_context.codec_context, ffmpeg_context.frame)) >= 0) {
                if (ret == 0) {
                    // A frame was successfully decoded!  Do something with it.
                    printf("Decoded a frame!  Frame size: %d x %d\n", ffmpeg_context.frame->width, ffmpeg_context.frame->height);
                    // frame->data[0] contains the video data
                    // frame->linesize[0] contains the length of a line (stride) in bytes of the video data.
                    av_frame_unref(ffmpeg_context.frame); //VERY IMPORTANT
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
        av_packet_unref(ffmpeg_context.packet);
    }
    av_packet_free(&ffmpeg_context.packet);

    // 5. Flush the decoder (send NULL packet to drain any remaining frames)
    avcodec_send_packet(ffmpeg_context.codec_context, NULL);
    while (avcodec_receive_frame(ffmpeg_context.codec_context, ffmpeg_context.frame) >= 0) {
        printf("Decoded a frame after flush\n");
        av_frame_unref(ffmpeg_context.frame);
    }

    // 6. Clean up
    av_frame_free(&ffmpeg_context.frame);
    avcodec_free_context(&ffmpeg_context.codec_context);
    avformat_close_input(&ffmpeg_context.format_context);
    avformat_network_deinit();
    return 0;
}

