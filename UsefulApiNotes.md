
# Recipes

### wrapping threads

Threading on the WiiU is cooperative, unlike pthreads which are scheduled by the os (preemptive).

It really depends on what the threads are doing.   
However, one thing to play with is adding a max run time to each thread.

```
    SDL_Thread* my_thread = SDL_CreateThread(...);
// cpp
    auto native_handle = reinterpret_cast<OSThread*>(SDL_GetThreadID(my_thread));
    OSSetThreadRunQuantum(native_handle, 1000);
// C
    OSThread* native_handle = (OSThread*) SDL_GetThreadID(my_sdl_thread);
    OSSetThreadRunQuantum(native_handle, 1000);
```

courtesy DanielKO

### exiting back to system menu


```
while (WHBProcIsRunning()) {

    if (user_wants_to_quit()) // for instance, user selected "quit" from a menu
        SYSLaunchMenu(); // will cause WHBProcIsRunning() to return false later

    // ...

}
```

coutesy captialsim, DanilKO, vgmoose

### SDL DRC touchscreen events

The WiiU gamepad "DRC" has a precise stylus touch screen.  
SDL2 has a base set of APIs for mouse(accurate) and touch screens (less accurate),
but no stylus touch streen (accurate).

At the time of writing, the released SDL2 package for WiiU does not have 
easy clear touchscreen support.

```
hito16 -  I'm looking at the SDL-wiiu-sdl2-2.28 source, but I don't see any obvious support for touch screen (SDL_FINGERDOWN, etc).   Am I missing something?
mm... maybe found something.

./SDL-wiiu-sdl2-2.28/src/joystick/wiiu/SDL_wiiujoystick.c:        VPADGetTPCalibratedPoint(VPAD_CHAN_0, &tpdata, &vpad.tpNormal);

Abood — 5/12/25, 11:29 PM
If you want another reference for touch controls, check RIO
It's made based on reverse engineering Nintendo's own "engine" (they call it engine but it's more like a framework, or at least it's used that way)
https://github.com/aboood40091/rio/blob/master/src/controller/cafe/rio_CafeDRCControllerCafe.cpp#L73-L120

DanielKO — 5/13/25, 12:04 AM
You'll need to build SDL2 from the devkitPro repo, they have not released since they merged my PR for better touch support.
You can just use mouse events. It'll be reported as a special mouse.

hito16 — 5/13/25, 12:06 AM
mmm.. so without rebuilding SDL2 and using that, I can't get WIIU touchpad events in any form (mouse or touch) ?

DanielKO — 5/13/25, 12:07 AM
Finger events are normalized, since usually the digitizer operates on its own coordinates, own resolution nothing to do with rendering. SDL leaves it up to you to multiply finger coords to screen coords.
But the "touch" mouse does it for you.
You can get finger events with latest release. You just don't get the automatic mouse emulation.
Make sure you initialize the Game Controller, and you open the gamepad.
Alternatively, you can simply not use the SDL Joystick/Controllers, and call VPADRead() directly.

DanielKO — 5/13/25, 12:18 AM
I was going to say, here's an example: https://github.com/dkosmari/devkitpro-autoconf/blob/main/examples/wiiu/sdl2-wiimotes/src/main.cpp
But I only call KPADRead(), not VPADRead().

Abood — 5/13/25, 12:24 AM
KPAD is for Wiimotes, as the folder name suggests

DanielKO — 5/13/25, 12:25 AM
It's just to show how you can choose to not use SDL input processing, and read inputs yourself.
You pretty much have to do it, if you want access to everything the gamepad has, like magnetometer, and the built-in orientation tracking. None of that is accessible through SDL.

Abood — 5/13/25, 12:34 AM
Yeah

DanielKO — 5/13/25, 12:43 AM
Maybe I'll add another example for that... A game of chicken demo. Get points for the longest free fall you can get from the gamepad.
hito16 — 5/13/25, 1:00 AM
you mentioned there is a newer version of SDL that hasn't been released.  Is this the repo, or is it somewhere else?  https://github.com/devkitPro/SDL
```

### sleep mode

The drc goes to sleep.  Try 

```
#include <coreinit/energysaver.h>

IMStartAPDVideoMode();
```

Or the other functions in that header. 
APD = Auto Power Down

coutesey of DanielKO



# Questions

me: thx. question.  If I don't configure WHBLogging,  do stdout and error do to some kind of /dev/null?  Say, I import a library with a bunch of fprintf()'s, I should have to worry about the printed out data queuing up somewhere until I run out of memory, right?

tl;dr - effectivley all messages are simply dropped.  They are not queued.

DanielKO - `stdin`, `stdout`, `stderr`, are all initialized properly to `dotab_stdnull`. The other entries shouldn't even need to be initialized, because no file handle should be created before initializing the devoptab it uses. At that point you have a memory corruption somewhere.
