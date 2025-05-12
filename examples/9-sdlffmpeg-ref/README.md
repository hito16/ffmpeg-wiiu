# Week 4+  A new approach.  Normal apps

Now I can build something tha runs on both Mac OS and WiiU with minimal code changes, let's change direction.

_Build out normal, clean, unadulterated code samples of ffmpeg and SDL on MacOS._


### testing
On MacOS: Install brew and the libraries.   Build it, run it. 
* (cd macos; make -f Makefile.macos; ./sdl-ffplay-app)
On WiiU: Build it, run it. 
* make; wiiload *.rpx

## WIP - Here there be dragons 

This may be the last example I publish. Productivity is MUCH improved, but 
now I need to learn more about the 3rd Party apis. ATM, when time allows,  
I'm just playing with 3rd party APIs, learning by trying public cookbooks and
 boiler plate examples code... All compiled and running on the Mac.

At this point, most of these examples only compile and run on MacOS, but they should be
able to run on the WiiU.  

A real testiment to all those who worked on devkitpro, WUT, SDL ports, Aroma, and everything that lowered the barrier to entry.


## TODO

### replicate ffplay video out

1. ffplay builds fine on wiiu and mac.  On mac it runs without issue, but on wiiu it hangs.  The two big implementation differences are WiiU SDL_Thread and WiiU SDL_Texture. I tried to replicate the ffplay logic on my own in ffmpeg-playvid.c, and I do see a similar hang.
2. need gdb

### replicate ffplay audio out

ffplay uses ffmepg + SDL streaming api with threading.  Replicating ffplay requires using
* SDL_OpenAudioDevice  (with a callback function)
* SDL_MixAudioFormat
* avcodec_receive_frame
* swr_convert
TBD. implement this.


## Things I learned

* threading on wiiu is not fun.  By adding delay's, the code runs longer before hanging
* thanks to devkitpro and sdl, must of the stuff compiles.  Unfortunately, examples that run on mac hang or crash on the WIIU
* gdb is 
