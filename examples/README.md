## Examples

### Are you building media player?

No. It's currently out of my skillset.

But if you're interesting in making your own media player, I learned a bunch
looking at the project GaryOderNichts/FFmpeg-wiiu.  It states it is an 
attempt to add WiiU's built in h264 decoding library to ffmpeg.

### What are these example files for?

I'm learning by doing.  Step up a Dev environment, make some WiiU basic apps, try some ffmpeg API for detection and decoding, learn some WiiU UI code (sdl, etc), try out WiiU Texture Shaders, etc.

### Things I've learned

##### Docker

1. Install Docker Desktop for Windows or MacOS
2. Clone this repo. (per top level readme)
3. in a terminal, build and run docker (per top level readme)
4. in the terminal running docker, you should see something like this.

```
# head -1 /etc/os-release
PRETTY_NAME="Debian GNU/Linux 11 (bullseye)"
```

##### VSCode 

1. Install VSCode
2. Open up VSCode, update the C and Make extensions.
3. Clone/update all repos from github.  
   - ex. $HOME/Documents/code/ffmpeg-wiiu (this repo)
   - ex. $HOME/Documents/code/ffmpeg (actual ffmpeg repo)
4. run Docker guest in separate terminal
   - ex. $HOME/Documents/code  
5. Edit homebrew files on VSCode running on host OS.
6. Run "make" in separate Docker terminal (ie Debian container)

##### Testing

A. Cemu - test .wuhb file in CEMU.   
   - The CEMU option takes seconds to test, and has a host of debugging features
   - On your remote windows machine (or VM), create a windows share, copy .wuhb to share, load .wuhb file in CEMU

B. Wiiload - needed for final integration tests on Wii U system. Quickly push changes to a running WiiU system.
   - install the wiiload plugin, and following the instructions

Note: So far, in my limited experience, homebrew runs on CEMU Windows, 
but hangs on CEMU for Mac
