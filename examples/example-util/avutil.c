#include <libavcodec/avcodec.h>
#include <whb/log.h>
#include <whb/log_console.h>

/* list available codecs compiled into ffmpeg */
int av_print_codecs() {
    const AVCodec *codec = NULL;
    void *i = 0;
    WHBLogPrintf("= Available codecs compiled into ffmpeg");
    while ((codec = av_codec_iterate(&i))) {
        const char *codec_type_str;
        switch (codec->type) {
            case AVMEDIA_TYPE_VIDEO:
                codec_type_str = "V";
                break;
            case AVMEDIA_TYPE_AUDIO:
                codec_type_str = "A";
                break;
            default:
                codec_type_str = "?";
                break;
        }

        WHBLogPrintf("  %s%s %s %s %s", 
                     av_codec_is_decoder(codec) ? "D" : "-",
                     av_codec_is_encoder(codec) ? "E" : "-", codec_type_str,codec->name,
                     
                     codec->long_name);
    }
    WHBLogConsoleDraw();

    return 0;
}
