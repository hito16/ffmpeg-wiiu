## Examples

Me, with almost zero homebrew experience, cutting my teeth on the environment setup and porting, and leaving my notes behind for others.

Pay attention to the "What I learned" at the bottom of each README.md for mistakes to avoid and lessons learned at each step.

### Skip to the good stuff?

Of all the "examples", 3 are probably worth cleaning up and publishing separately.

1. 8-sdl-portable   - A simple example that runs on both MacOS and WiiU
   - If you have a bit of experience in Unix C/C++, but 0 experience in WiiU homebrew (or SDL), start here.
2. 6-sdlhelloworld  - A true, minimal SDL Hello World Example. 
3. rsyslog - TCP logging.  
   - (1 docker command) Starts up a rsyslogd server
   - (1 function call) Configures stdout/stderr redirection from the client to that remote syslog server.

### What are these example files for?

To learn by doing. Specifically

* Learn dev environment
* [Done] Step up and get comfortable with a homebrew Dev environment,
* [Done] make some WiiU basic apps, 
* [Done] update ffmepg_wiiu config script, and link with WiiU example code.
* [Done] try some ffmpeg API for detection on WiiU
* [Done] compare file system speed to ffmpeg decoding speed on WiiU
* Learn applcation stuff - ideally portable apps first
* [Done] set up remote logging (tcp, udp, etc) 
* [Done] learn some basic WiiU UI code (sdl, etc), 
* [Done] learn how to build "portable" apps that cross compile on both Mac and WiiU
* [WIP]  try some ffmpeg sdl player code samples on the WiiU
* try out GX Texture Shaders, etc.  Needed for performance unless SDL can do it.
* learn queue systems used for decoder apps

What I learn, I'll take notes to leave behind for others.

### Things I've learned

##### Docker

In general, you should use the Dockerfile, but start it from one directory above so you can access multiple
different git repos.  Ex. after starting docker, your directory could look like

/project/
  ffmpeg/
  ffmpeg-wiiu/
    - Dockerfile
  libromfs-wiiu-master/
  ProgrammingOnTheWiiU/


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
   - Aroma wiiload plugin takes away most of the pain.  At this point, no reason to not upgrade Aroma.

Note: So far, in my limited experience, homebrew runs on CEMU Windows, 
but hangs on CEMU for Mac

##### Logging / Debugging
 I've reached to end of printing to the screen and saving to files.
 TBD - set up udp logging and gdb plug in.
