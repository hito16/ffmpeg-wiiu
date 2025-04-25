#ifndef EXUTIL_H
#define EXUTIL_H

/* file */
int util_get_first_media_file(char *buffer, int size);
int64_t util_get_file_size(char *filename);

/* ffmpeg av */
int av_print_codecs();

#endif  // EXUTIL_H
