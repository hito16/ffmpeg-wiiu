#!/bin/bash
# run this before make to copy over patched
# versions of the code, friendly to the WiiU

copy_oneko_h() {
  grep -v '<X11/'  ../oneko.h > oneko.h
}

copy_sdl_oneko_c() {
  sed  's/int main(/int myapp_main(/g'  ../sdl_oneko.c > sdl_oneko.c
}

copy_sdl_xbm_helper() {
  cp ../sdl_xbm_helper.c  ../sdl_xbm_helper.h .
}

copy_oneko_h
copy_sdl_oneko_c
copy_sdl_xbm_helper
