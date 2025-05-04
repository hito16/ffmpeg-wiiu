# Week 2:  OpenSource capatibility check - ffmpeg api 

_Caveat: Don't treat these as "proper" examples.  I'm just showing what I did to get up to speed with the homebrew environment.  Look at these, learn and avoid the same bruises_

Try out some existing open source APIs on the WiiU.  Specifically, 
1. open a media file
2. extract metadata

Implies we found out where the SD card is mounted from the last example.

## TODO

Build it. Run it. Or, just look at the screenshot.

## Things I learned

1.  Developing WiiU by only testing code on WIIU is time consuming 
  - Add lots of WHBPrintf() statements everywhere
  - UDP logging is rough. Very lossy.  Seeing the errors requires patience
2. FFMPEG can be tuned to go faster (addressed in a later example).  Right now,
the stream detection can appear to hang.
3. Lack of a arg passing is rough.  In this example, we just pick the first file in "/media"

