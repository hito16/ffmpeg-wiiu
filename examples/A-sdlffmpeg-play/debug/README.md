# Debugging notes

The rpx builds and runs, but no audio or video.   The latest log from the rpx shows it reading a few 
audio frames and one video frame, while also calling display.  After the first video frame, 
logging output stops.

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

WiiU logging indicates both threads start, and (probably) both produce output
before silence. I suspect threading issue (see below)  Again, no sound or video
are produced

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


### Major problem theory - thread deadlocking.

From printf's in the code, I can tell that two threads are running simultanously.

1. Read thread - reads raw frames writes to a queue
2. Display thread - displays decoded frames.

If you look at the file latest_logs.txt, you'll see 

```
Thread 2:
5-05-09T11:31:05-07:00 192.168.65.1 WIIU: SDL_CreateThread - readthread
2025-05-09T11:31:05-07:00 192.168.65.1 WIIU: SDL_CreateThread - readthread created

Thread 1: (main)
2025-05-09T11:31:05-07:00 192.168.65.1 WIIU: Finished Stream open, starting event_loop
2025-05-09T11:31:06-07:00 192.168.65.1 WIIU: avformat_find_stream_info start
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: avformat_find_stream_info end
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: av_dump_format start
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: av_dump_format  end
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: avformat_match_stream_specifier
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: avformat_match_stream_specifier
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: avformat_match_stream_specifier
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: av_find_best_stream video
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: av_find_best_stream audio
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: av_find_best_stream subtitle
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: av_guess_sample_aspect_ratio and set_default_window_size
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: stream_component_open AUDIO
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: stream_component_open VIDEO
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: stream_component_open SUBTITLE
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: Beginning read_thread main loop
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: is->queue_attachments_req
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: display: 0 0 1 0
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: av_read_frame >=0 put audio 
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: av_read_frame >=0 put audio 
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: display: 0 0 1 0
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: av_read_frame >=0 put audio 
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: av_read_frame >=0 put audio 
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: display: 0 0 1 0
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: av_read_frame >=0 put audio 
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: av_read_frame >=0 put audio 
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: display: 0 0 1 0
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: av_read_frame >=0 put audio 
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: display: 0 0 1 0
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: av_read_frame >=0 put audio 
2025-05-09T11:31:32-07:00 192.168.65.1 WIIU: av_read_frame >=0 put video 
```


## Next steps,
* redirect av_log to stdout?
* if possible in devkitpro, print thread ids.
* print queue sizes, and all queue push/pull events
* gdb resources for WiiU?
