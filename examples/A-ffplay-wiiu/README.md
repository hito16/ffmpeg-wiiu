# A patched recompile of ffplay for WiiU

ffmpeg comes with a reference media player called ffplay.  

I am able to get the code to compile on both Mac OS and WiiU.  At the moment, the MacOS builds run fine, but the WiiU has issues. Specifically, the RPX
1. starts up
2. loads the first file from sd:/media
3. detects the steams
4. sets up main and reader threads
5. begins to read packets and write to a queue.
6. (possibly) the display thread is being called, but no video out.

From my printf's, I suspect threading issues, but I need to learn more about GDB and threading on WiiU.

## building

I have a lot of notes on testing the build on MacOS first to ensure code and the environement 
are sane enough for the code to play video.  You can skip the sanity checks and try your luck doing.

1. Check out ffmpeg master, ensure it is adjacent to this git checkout, and visible in the same devkitpro Docker environment.
2. set FFMPEG_SRC to whereever you extracted ffmpeg master
3. run 
   - (./prepare_patches.sh && cd $FFMPEG_SRC && ./configure_wiiu_ffmplay && make)
4. if the vanilla ffplay built without errors, build a patched rpx of ffplay
   - ./prepare_patches.sh rebuild

## debugging

1. Edit the original $FFMPEG_SRC/fftools/ffplay.c.  The build script will create a patched copy when you 
   - ./prepare_patches.sh rebuild
2. Start Docker.rsyslog (root of this project) and tail /var/log/remote/* 
3. rebuild and push with wiiload
