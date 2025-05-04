# Week 3 an actual SDL Hello world, that uses graphics to write on the screen

_Caveat: Don't treat these as "proper" examples.  I'm just showing what I did to get up to 
speed with the library and environment.  Consider these as throwaway code snippets from a 
newbie finding his way, but willing to share his stupid mistakes._

From example 5-xxxx   We know our environment and Makefile is sane. 

Take some boilerplate SDL code, compile it to the WiiU with as few changes as possible.


## TODO
Build it, run it.


## Things I learned
1. SDL is very, very portable.  Internet code samples look like 
they can run on the WiiU  basically unaltered.
2. Key differences are 
 - ROMFS is needed for fonts/images,  (ifdef needed)
 - SDL headers includes look different than most examples (ifdef needed)
 - WiiU specific headers (#ifdef needed)
 - standard SDL Controller events need to be configured (next example)
3. UDP logging is basically useless, but I have a plan...


### external deps
At the time of writing, there are required dependencies.

1. libromfs-wiiu
  - Google search "libromfs-wiiu"
  - download the zip file and extract to the same top directory where fmpeg and this ffmpeg-wiiu workspace are.
     - the top level directory should look like this
     - ffmpeg/
     - ffmepg-wiiu/
     - libromfs-wiiu/
  - Follow build instructions on the github page
     - make; make install
     - (assumes you are doing this in the same docker environment as the examples)
2. Roboto-Regular.ttf
  - google the term "Google Fonts"
  - find and download the Roboto font set containing Roboto-Regular.ttf
  - in this example folder make a romfs/res and copy Roboto-Regular.ttf
      - mkdir -p romfs/res
      - cp Roboto-Regular.ttf ffmpeg-wiiu/examples/5-sdlcheckenv/romfs/res
      - see Makefile for more

### Logging

With SDL, we can no longer rely on console screen logging.  Instead, we will log to UDP.

1. Compile a server
 - google "wiiu udp debug" or "udp debug reader". download and extract
 - build for your platform
   - to reuse this running Linux docker container, attach a new terminal 
   - ex. docker exec -it ffmpeg-wiiu /bin/bash
   - follow build intructions on the github for Linux
   - Run and watch output
   -  ex. open a termal on your desktop
   -  run netcat  "nc -ul 4402" or /opt/devkitpro/tools/bin/udplogserver

2. Test Homebrew
   - wiiload <this app>.rpx  and test.
