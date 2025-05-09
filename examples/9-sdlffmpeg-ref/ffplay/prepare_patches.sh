#!/bin/bash
# This is a trial run.  Get all the pieces working my mac laptop, THEN once it plays video,
# you have all the basic dependencies.  Move onto the 
#
# approach - convert the CLI to a library, and link it into a separate main().
#  get it working on mac, then we can try WIIU
#

if [[ -z "$FFMPEG_SRC" ]]; then
  echo '$FFMPEG_SRC must be set to the root of your FFMPEG source tree checkout'
  echo 'ex.  export FFMPEG_SRC=$HOME/Documents/code/ffmpeg'
  exit 1
fi

echo FFMPEG_SRC=$FFMPEG_SRC

if ! [ -d "$FFMPEG_SRC/fftools" ]; then
  echo "$FFMPEG_SRC/fftools not found"
  exit 1
fi

generate_main_stub() {

cat << EOF
#include <stdio.h>

/*
 ffplay_main is in our own separate library, so no header. 
 You will link to that library when building.
*/
int ffplay_main(int argc, char **argv);

int main(int argc, char *argv[]) {
    // In a real application, you might get the filename from
    // command-line arguments, but for this example, we will
    // just use a hardcoded value.
    const char *filename = "__filename__";  // change this to a real file
    char *my_argv[] = {
        "ffplay", // argv[0] is the program name.
        (char *)filename,
        NULL // argv must be NULL-terminated.
    };
    int my_argc = 2; // Number of arguments in my_argv

    // Call ffplay_main
    int result = ffplay_main(my_argc, my_argv);

    // Do something with the result (e.g., check for errors)
    if (result != 0) {
        fprintf(stderr, "ffplay_main failed with error code: %d\n", result);
        //  Consider returning a non-zero value from main to indicate failure
        return 1;
    }

    printf("ffplay_main executed successfully.\n");
    return 0;
}
EOF
}

echo "= copy over custom configure_xxx_ffplay"
cp configure_wiiu_ffplay $FFMPEG_SRC
cp configure_macos_ffplay $FFMPEG_SRC


echo '= generating copy of ffplay.c as ffplay_cli.c'
echo 'copy ffplay.c to ffplay_cli.c'
echo 'replace ffplay_cli.c  main() with ffplay_main()'
sed 's/int main(/int ffplay_main(/g' $FFMPEG_SRC/fftools/ffplay.c > $FFMPEG_SRC/fftools/ffplay_cli.c

echo '= generating a stub main "generic_main.c" to call our library '
generate_main_stub > $FFMPEG_SRC/fftools/generic_main.c


cat << EOF

== done, next steps =
1. run the configure_xxx_ffplay in $FFMPEG_SRC

2. Ensure configure detects SDL and filter aresample (needed for sound)

...

External libraries:
appkit                  coreimage               libxcb_shape            libxcb_xfixes
avfoundation            libxcb                  libxcb_shm              sdl2

...

Enabled filters:
aformat                 atrim                   hflip                   transpose
anull                   crop                    null                    trim
aresample               format                  rotate                  vflip


ONLY continue if you see at least sdl2

3. on your mac, 
    cd $FFMPEG_SRC
    make &&  make install

4. on your mac, try out ffplay on your dev 
    cd $FFMPEG_SRC
    ./ffplay <video file>

5. on your mac, try out our rebuilt ffplay
  - cd $FFMPEG_SRC
  - rm -f  ./ffplay and ./fftools/*.o  
  - make -f Makefile.macos.mk
  - ./ffplay <video file>

(should work) We replicated their Makefile to rebuild ffplay.
We are now free of their build process and can integrate with ours.

6. on your mac, build ffplay as a library
  - cd $FFMPEG_SRC
  - make -f Makefile.macos.mk libffmplay
  - confirm libffplay.a was built

At this point, we have succesfully rebuilt ffmplay as a library 
We could ship this library anywhere, and use it our own apps.

8. on your mac, build ffplay_generic with our library
  - edit fftools/generic_main.c and add a real media file path
  - make -f Makefile.macos.mk ffplay_generic
  - run ffplay_generic with no args

See where we're going with this? the ffplay_generic main()
is just like a wiiu homebrew app

We will repeate this on a devkitpro/WUT build.

EOF
