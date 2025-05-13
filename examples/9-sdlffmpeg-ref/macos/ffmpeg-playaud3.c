/* ffmpeg-playaud.c
 * Plays audio from a video file using FFmpeg 6.x+ and SDL2.
 * Only audio is played; video frames are decoded but not rendered.
 * WORKS - now with callback, but is choppy
 */

#include <SDL2/SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <stdio.h>
#include <string.h>

#ifndef AUDIO_PLAYER_ONLY
#define AUDIO_PLAYER_ONLY
#endif

#define AUDIO_BUFFER_SIZE 192000
#define RESAMPLE_BUFFER_SIZE 4096 * 4 // Increased resample buffer

typedef struct AudioData {
    uint8_t buffer[AUDIO_BUFFER_SIZE];
    int buffer_size;
    int buffer_index;
    SDL_mutex *mutex;
} AudioData;

void audio_callback(void *userdata, Uint8 *stream, int len) {
    AudioData *audio = (AudioData *)userdata;
    SDL_LockMutex(audio->mutex);
    int remaining = audio->buffer_size - audio->buffer_index;
    int to_copy = (len > remaining) ? remaining : len;

    if (to_copy > 0) {
        memcpy(stream, audio->buffer + audio->buffer_index, to_copy);
        audio->buffer_index += to_copy;
    }
    if (to_copy < len) {
        memset(stream + to_copy, 0, len - to_copy); // silence
    }
    SDL_UnlockMutex(audio->mutex);

    if (audio->buffer_index >= audio->buffer_size) {
        audio->buffer_size = 0;
        audio->buffer_index = 0;
    }
}

void play_audio_only(const char *filename) {
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *codec_ctx = NULL;
    AVCodecParameters *codecpar = NULL;
    const AVCodec *codec = NULL;
    AVPacket *pkt = NULL;
    AVFrame *frame = NULL;
    SwrContext *swr_ctx = NULL;
    int audio_stream_index = -1;

    AudioData audio = {.buffer_size = 0, .buffer_index = 0};
    audio.mutex = SDL_CreateMutex();

    if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", filename);
        return;
    }

    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return;
    }

    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            codecpar = fmt_ctx->streams[i]->codecpar;
            break;
        }
    }

    if (audio_stream_index == -1) {
        fprintf(stderr, "Could not find audio stream\n");
        return;
    }

    codec = avcodec_find_decoder(codecpar->codec_id);
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        fprintf(stderr, "Could not allocate codec context\n");
        return;
    }
    avcodec_parameters_to_context(codec_ctx, codecpar);
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        return;
    }

    SDL_Init(SDL_INIT_AUDIO);

    SDL_AudioSpec wanted_spec = {
        .freq = 44100,
        .format = AUDIO_S16SYS,
        .channels = 2,
        .samples = 1024 * 2, // Increased SDL buffer size
        .callback = audio_callback,
        .userdata = &audio,
    };

    SDL_AudioSpec obtained_spec;
    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &obtained_spec, 0);
    if (!dev) {
        fprintf(stderr, "Failed to open audio device: %s\n", SDL_GetError());
        return;
    }

    AVChannelLayout in_ch_layout;
    av_channel_layout_copy(&in_ch_layout, &codecpar->ch_layout);

    AVChannelLayout out_ch_layout;
    av_channel_layout_default(&out_ch_layout, 2);

    if (!av_channel_layout_check(&out_ch_layout)) {
        fprintf(stderr, "Invalid default channel layout\n");
        return;
    }

    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        fprintf(stderr, "Could not allocate resampler context\n");
        return;
    }
    av_opt_set_chlayout(swr_ctx, "in_chlayout", &in_ch_layout, 0);
    av_opt_set_chlayout(swr_ctx, "out_chlayout", &out_ch_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", codecpar->sample_rate, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", wanted_spec.freq, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", codecpar->format, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

    if (swr_init(swr_ctx) < 0) {
        fprintf(stderr, "Failed to initialize resampler\n");
        return;
    }

    pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "Could not allocate packet\n");
        return;
    }
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate frame\n");
        return;
    }
    SDL_PauseAudioDevice(dev, 0);

    uint8_t *resample_buffer = (uint8_t *)av_malloc(RESAMPLE_BUFFER_SIZE);
    if (!resample_buffer) {
        fprintf(stderr, "Could not allocate resample buffer\n");
        return;
    }

    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == audio_stream_index) {
            if (avcodec_send_packet(codec_ctx, pkt) == 0) {
                while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                    int out_samples = av_rescale_rnd(
                        swr_get_delay(swr_ctx, codecpar->sample_rate) + frame->nb_samples,
                        wanted_spec.freq, codecpar->sample_rate, AV_ROUND_UP);

                    int converted = swr_convert(swr_ctx, &resample_buffer, out_samples,
                                                (const uint8_t **)frame->extended_data, frame->nb_samples);
                    if (converted < 0) {
                        fprintf(stderr, "swr_convert failed\n");
                        continue;
                    }
                    int size = av_samples_get_buffer_size(NULL, 2, converted, AV_SAMPLE_FMT_S16, 1);

                    SDL_LockMutex(audio.mutex);
                    if (size <= AUDIO_BUFFER_SIZE) {
                        memcpy(audio.buffer, resample_buffer, size);
                        audio.buffer_size = size;
                        audio.buffer_index = 0;
                    }
                    SDL_UnlockMutex(audio.mutex);
                }
            }
        }
        av_packet_unref(pkt);
        SDL_Delay(1); // Reduced delay
    }

    SDL_Delay(1500); // Reduced delay
    SDL_CloseAudioDevice(dev);
    SDL_DestroyMutex(audio.mutex);
    SDL_Quit();

    av_free(resample_buffer);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    swr_free(&swr_ctx);
    if (codec_ctx)
      avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
}

#ifdef BUILD_AS_FUNCTION
int ffmpeg_playaud_main(int argc, char *argv[]) {
#else
int main(int argc, char *argv[]) {
#endif
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <video_file>\n", argv[0]);
        return 1;
    }
    play_audio_only(argv[1]);
    return 0;
}
