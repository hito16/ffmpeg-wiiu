# Week 1  directory 2025

_Caveat: Don't treat these as "proper" examples.  I'm just showing what I did to get up to 
speed with the library and environment.  Consider these as throwaway code snippets from a 
newbie finding his way, but willing to share his stupid mistakes._

A lot of the homebrew tool chains look very posix-like.  How So?

Let's try using file system checks, and see what we're allowed to view.
Especially, where is the SD card mounted?

## TODO
Warning: it is touching the file system (READONLY). In its state, it just 
does access(), stat() and directory reads. If you change this to do writes and 
screw up your WiiU, that's on you.  

Just note in the code:
1. A bunch of directory checks
2. We use the Console API, which is easier to mess with.

