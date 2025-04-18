## Examples

### Are you building media player?

No. But if you're interesting in making your own media player, I highly recommend 
looking at the project GaryOderNichts/FFmpeg-wiiu.  It states it is an 
attempt to add WiiU's built in h264 decoding library to ffmpeg.

### What are these example files for?

I don't know what I'm doing.  

This is my first time messing around with gaming hardware and homebrew api.   
I'm going through examples to learn the API and development environment.  
I may not have dedicated blocks of time to get much progress, but at least I 
can save what I learn for future me.

Using ffmpeg, I plan to try and test out file format detection, and video format
decoding speeds.  If I have time, I'd like to replicate the work done with 
Shaders that were done for Moonlight.  The code looks very similar to the 
OpenGL tutorials, but again, its mostly new to me.


### Things I learned

#### Environment 

##### Docker

Docker is the PERFECT solution for cross compilers. (see Dockerfile in root)

I've been able to build on mac and windows hosts systems, with little more than 
Docker Desktop installed and my listed docker commands (on the main page)

I have built out Mac OS and Ubuntu enviroments following the DevkitPro and WUT sites,
but honestly it isn't worth it.

##### VSCode 

Check out  this repo.  I put it under $HOME/Documents/code/ffmpeg-wiiu.

Open up VSCode, update the C and Make extensions.

Edit on host, compile on guest Docker container.

Almost identical experience on Mac and Windows.

##### Testing

You can test with Wiiload (publish compiled package a running Wii U) or simply load 
the .wuhb file in CEMU.   The CEMU option takes seconds, and provides easy to access
logfiles.  If you can run CEMU on the same host as
your development environment, you can just point CEMU at your local git checkout.

Note: So far, in my limited experience, homebrew runs on CEMU Windows, 
but on CEMU for Mac, homebrew just hangs.
