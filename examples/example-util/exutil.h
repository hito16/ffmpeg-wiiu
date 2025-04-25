#ifndef EXUTIL_H
#define EXUTIL_H

#include <stdint.h>

/* file */
int util_get_first_media_file(char *buffer, int size);
int64_t util_get_file_size(char *filename);

/* ffmpeg av */
int av_print_codecs();

struct TestResults {
    int32_t st_size;
    int test_type; /* 0 file, 1 decode */
    uint64_t ops;  /* freads vs packet decodes */
    uint64_t data_read;
    uint64_t start_time;
    uint64_t end_time;
};

#endif  // EXUTIL_H
