# Week 4  A new approach.  Normal apps

Now I can build something tha runs on both Mac OS and WiiU with minimal code changes, let's change direction.

_Build out normal, clean, unadulterated code samples of ffmpeg and SDL on MacOS._


## WIP - Here there be dragons 

This may be the last example I publish. Productivity is MUCH improved, but 
now I need to learn more about the 3rd Party apis. ATM, when time allows,  
I'm just playing with 3rd party APIs, learning through public cookbooks and
example boiler plate code.

At this point, these examples only compile and run on MacOS, but they should be
able to run if I take away their main() call them from the "generic" SDL WiiU app stub.
They either print status to the rsyslogd server and exit, or they Draw something.

A real testiment to all those who worked on devkitpro, WUT, SDL ports, Aroma, and 
everything that lowered the barrier to entry.


## TODO

On MacOS: Install brew and the libraries.   Build it, run it. 
* make -f Makefile.macos
On WiiU: Build it, run it. 
* make; wiiload *.rpx


## Things I learned
TBD
