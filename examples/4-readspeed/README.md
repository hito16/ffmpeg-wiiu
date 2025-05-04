# Week 2  Coding with OpenSource API - ffmpeg  

_Caveat: Don't treat these as "proper" examples.  I'm just showing what I did to get up to speed with the homebrew environment.  Look at these, learn and avoid the same bruises_

Let's do something practical. Let's
1. time how long it takes to just read the file
1. time how long it takes to decode the video frames (and throw them away)

## TODO

Build it. Run it. Or, just look at the screenshot.

## Things I learned

1. The lack of solid logging sucks. I'm ready to buy the special USB serial cable or implement logging
2. The WiiU is a capable system.  With 0 optimization, FFPEG was able to decode a 720x480 (Gamepad resolution)
video at about 57fps ... (but threw the packet away after decoding)
3. Makefile setup is crude.  Autotools and CMake assume you already know flags to add.  Perhaps if we can figure out a minimal config later?...

