# Week 1: Posix api test - directory

_Caveat: Don't treat these as "proper" examples.  I'm just showing what I did to get up to speed with the homebrew environment.  Look at these, learn and avoid the same bruises_

A lot of the homebrew tool chain look very posix-like.  How So?

Let's try using file system checks, and see what we're allowed to view.
Especially, where is the SD card mounted?

## TODO
Warning: it is touching the file system (READONLY). In its state, it just 
does access(), stat() and directory reads. If you change this to do writes and 
screw up your WiiU, that's on you.  

Just note in the code:
1. A bunch of directory checks
2. We use the Console API, which is easier to mess with.

