# Week 3: Back up.  Let's find a minimal "Vanilla" environment 2025

_Caveat: Don't treat these as "proper" examples.  I'm just showing what I did to get up to speed with the homebrew environment.  Look at these, learn and avoid the same bruises_

A simple example to check the readiness of your environment before continuing.
* learn where things are
* learn wut Make dependencies
* try out UDP logging - you're about to lose access to reliable onscreen WHBPrintf() once we move to SDL graphics.

To save yourself an afternoon, first confirm this basic build works.

## TODO

Follow instructions on dependencies below.  Build it.  Don't even need to run it.

Success: means my local WUT envionment and Makefile are not completely messed up.

Failure: means something is off in the environment or the toolchain changed something.  Fix it before moving on.

## Things I learned
1. **Most WiiU SDL examples I tried pre 2021-ish don't compile.  There's some change in dependencies or API.**
2. The external deps like libromfs-wiiu moved around, but work with the latest instructions.  Adding pkgconfig could really improve
integration with Makefile, CMake and autodeps.
3. Logging is not fun.  In this example, I set up a UDP logging server.  This is good becuase it works out of the box, however it is lossy. 
4. SDL Portlibs - somehow SDL is installed in a different directory than most header includes would expect. (SDL.h vs sdl/SDL.h)
   - pkgconf saves the day.  Just eats up time figuring this out.

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
