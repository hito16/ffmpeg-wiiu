## SDL check environment

A simple example to check the readiness of your environment before continuing.

Dependencies amd build tools change over time.  Many code samples online get outdated.  To save yourself an afternoon, first confirm this basic build works.

### external deps
At the time of writing, there are required dependencies.

1. libromfs-wiiu
  -  Google search "libromfs-wiiu"
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
   - ex. ./UDP-Debug-Reader 

2. Homebrew
   - wiiload and test.
