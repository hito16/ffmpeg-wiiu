/* ffmpeg-playaud.c
 * Plays audio from a video file using FFmpeg 6.x+ and SDL2.
 * Only audio is played; video frames are decoded but not rendered.
 *  Works finally. timing is perferct.
 * The decoder now waits if the buffer is full, only writing
 * when space is available.  This keeps decoding roughly synchronized with SDL
 * playback.
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

typedef struct AudioData {
    uint8_t buffer[AUDIO_BUFFER_SIZE];
    int buffer_size;
    SDL_mutex *mutex;
} AudioData;

void audio_callback(void *userdata, Uint8 *stream, int len) {
    AudioData *audio = (AudioData *)userdata;
    SDL_LockMutex(audio->mutex);
    int to_copy = (len > audio->buffer_size) ? audio->buffer_size : len;

    if (to_copy > 0) {
        memcpy(stream, audio->buffer, to_copy);
        memmove(audio->buffer, audio->buffer + to_copy,
                audio->buffer_size - to_copy);
        audio->buffer_size -= to_copy;
    }

    if (to_copy < len) {
        memset(stream + to_copy, 0, len - to_copy);  // fill with silence
    }
    SDL_UnlockMutex(audio->mutex);
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

    AudioData audio = {.buffer_size = 0};
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
    avcodec_parameters_to_context(codec_ctx, codecpar);
    avcodec_open2(codec_ctx, codec, NULL);

    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return;
    }

    SDL_AudioSpec wanted_spec = {
        .freq = 44100,               //
        .format = AUDIO_S16SYS,      //
        .channels = 2,               //
        .samples = 1024,             //
        .callback = audio_callback,  //
        .userdata = &audio,
    };

    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, NULL, 0);
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
    frame = av_frame_alloc();
    SDL_PauseAudioDevice(dev, 0);

    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == audio_stream_index) {
            if (avcodec_send_packet(codec_ctx, pkt) == 0) {
                while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                    uint8_t *out_buf;
                    int out_linesize;
                    int out_samples = av_rescale_rnd(
                        swr_get_delay(swr_ctx, codecpar->sample_rate) +
                            frame->nb_samples,
                        wanted_spec.freq, codecpar->sample_rate, AV_ROUND_UP);

                    av_samples_alloc(&out_buf, &out_linesize, 2, out_samples,
                                     AV_SAMPLE_FMT_S16, 0);
                    int converted =
                        swr_convert(swr_ctx, &out_buf, out_samples,
                                    (const uint8_t **)frame->extended_data,
                                    frame->nb_samples);
                    int size = av_samples_get_buffer_size(NULL, 2, converted,
                                                          AV_SAMPLE_FMT_S16, 1);

                    int written = 0;
                    while (written < size) {
                        SDL_LockMutex(audio.mutex);
                        int space_left = AUDIO_BUFFER_SIZE - audio.buffer_size;
                        int to_write = (size - written < space_left)
                                           ? (size - written)
                                           : space_left;

                        if (to_write > 0) {
                            memcpy(audio.buffer + audio.buffer_size,
                                   out_buf + written, to_write);
                            audio.buffer_size += to_write;
                            written += to_write;
                        }
                        SDL_UnlockMutex(audio.mutex);

                        if (written < size) {
                            SDL_Delay(10);  // Wait for buffer to drain
                        }
                    }

                    av_freep(&out_buf);
                }
            }
        }
        av_packet_unref(pkt);
    }

    SDL_Delay(5000);  // Let the last bit play
    SDL_CloseAudioDevice(dev);
    SDL_DestroyMutex(audio.mutex);
    SDL_Quit();

    av_frame_free(&frame);
    av_packet_free(&pkt);
    swr_free(&swr_ctx);
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
