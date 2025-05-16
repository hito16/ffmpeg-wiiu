#ifndef SDLPORTABLES_H
#define SDLPORTABLES_H

// standalone programs we load into our main with minor changes
//int ffmpeg_decode4_main(const char *input_file);
//int ffmpeg_sync2_main(char *filename);
//int ffmpeg_playvid_main(int argc, char *argv[]);
//int ffmpeg_playaud_main(int argc, char *argv[]);
int ffmpeg_play_media(int argc, char *argv[]);

#endif  // SDLPORTABLES_H
