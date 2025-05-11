# Debugging notes

The rpx builds and runs to the point I see one video frame.  No sound.  I switched to a more
complete build, which addresses hangs due to missing filters.

### reproducing
Since we build ffplay as a library, I am able to build and run the same code on both mac and WiiU
from simple main() wrappers.  On the Mac, the video plays as expected. 

* download ffmpeg adjacent to this git checkout, make visible in the same Docker instance
* build ffplay on mac following instructions in ./prepare_patches.sh
* build ffplay on WiiU following instructions in ./prepare_patches.sh
  - after the first successful build, you can run ./prepare_patches.sh rebuild to automate the library & rpx builds

### Files
* ffplay.c - a copy of ffplay.c modified slightly with printf.  This will grow out of sync 
with the real ffmpeg ffplay utility, so its just a reference.  During desting, modify the
original Ffmpeg_master/fftools/ffplay.c file.  The ./prepare_patches.sh with handle
making a copy, patching it and building the pkg for you.
* gdb_mac.txt - notes for myself on the preferred mac debugger symbol version and startup options.

### WiiU logging

I'm using syslog (TCP) logging.  See the readme in ../../rsyslog on how to run 
and view logs via Docker.

# Issues
### major problems
On the Mac, the code runs fine.  On the WiiU... 

ATM, I suspect something with audio or video

### minor problems 
On the Mac, the code runs fine.  On the WiiU... 

SDL_CreateWindow and SDL_CreateRenderer calls don't seem to work.
I added the following immediate after initialization

```
   SDL_SetRenderDrawColor(renderer, 0, 128, 128, 255);
   SDL_RenderClear(renderer);
   SDL_RenderPresent(renderer);
```

But the screen never updated the color.  However, when I temporarily 
comment their SDL_CreateXXX code block  out and replace it with the following.

```
   SDL_CreateWindowAndRenderer(640, 480, SDL_WINDOW_SHOWN, &window, &renderer);
```

the screen color does change. TBD. Followup.  This is not worth pursuing without
fixing the major problem.
