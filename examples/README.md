## Examples

### Are you building media player?

No. But if you're interesting in making your own media player, I highly recommend 
looking at the project GaryOderNichts/FFmpeg-wiiu.  It states it is an 
attempt to add WiiU's built in h264 decoding library to ffmpeg.

### What are these example files for?

Learn WiiU programming by doing.   Dev environment, WiiU basic apps, ffmpeg API for detection and decoding, WiiU UI code, WiiU Texture Shaders, etc.

### Things I've learned

##### Docker

There is little reason to not use Docker.
The Dockerfile I used comes from other WiiU projects that I stripped down.
I believe the OS image is what the WUT toolchain team uses to build the core
dependencies. (I'll correct this as I learn more)


1. Install Docker Desktop for Windows or MacOS
2. Check out this repo code. (per top level readme)
3. in a terminal, build and run docker (per top level readme)
4. in the terminal running docker, you should see something like this.

```
head -1 /etc/os-release
PRETTY_NAME="Debian GNU/Linux 11 (bullseye)"
```

##### VSCode 

1. Check out  this repo.  
   - I put it under $HOME/Documents/code/ffmpeg-wiiu.
2. Open up VSCode, update the C and Make extensions.
3. Edit homebrew files on host.
4. Start Docker guest in separate terminal, starting with $HOME/Documents/code 
   -  Start Docker one directory above because we're including multiple git repos for ffmpeg, etc.
5. Run "make" in separate Docker terminal (ie Debian container)

##### Testing

A. Cemu - test .wuhb file in CEMU.   
   - The CEMU option takes seconds to test, and has a host of debugging features
   - On your remote windows machine (or VM), create a windows share, copy .wuhb to share, load .wuhb file in CEMU
B  Wiiload - needed for final integration tests on Wii U system. Quickly push changes to a running WiiU system.

Note: So far, in my limited experience, homebrew runs on CEMU Windows, 
but hangs on CEMU for Mac
