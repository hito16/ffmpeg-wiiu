# Experiments on MacOS

I'm still learning the various APIs from ffmpeg and SDL.
Rather than code and test for the WiiU, I'm coding and testing on MacOS. 

When I want to test one of these files, I copy it up 1 dir and add
#ifdef __WIIU__  in places like main(), #include <SDL> and ROMFS (fonts)
so it compiles.
