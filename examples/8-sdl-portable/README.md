# Week 4  A new Day 1, "START HERE" app 

## A basic SDL program, that runs on both MacOS and Wiiu

Looking back, this is the Day 1 Hello World example that I wish "past me" had access to 3 weeks ago when I started.

_Caveat: Don't treat these as "proper" examples.  I'm just showing what I did to get up to speed with the homebrew environment.  Look at these, learn and avoid the same bruises_


A Bold Claim: Last example, I thought the environment was close enough that the same code could be developed on Mac and run on WiiU.  

Below is a first proof of concept:  The only WiiU specific stuff is in
a few small typedefs.  

* thanks to vgmoose for clarifying SDL controller setup for the WiiU
* thanks to DanielKO and Serena for pointers resolving the absence of printf() logging


## TODO
Look at the sdlmain.c file.

On MacOS: Install brew and the libraries.   Build it, run it. 
* make -f Makefile.macos
On WiiU: Build it, run it. 
* make; wiiload *.rpx


## Things I learned
1. I should start building and running examples on Mac first.
2. Ideally, in the real of posix/SDL code, no WiiU specific code is required, beyond constants and #defines in headers.
3. When building existing packages, keep them single threaded for now.