#!/bin/bash
# This is a trial run.  Get all the pieces working my mac laptop, THEN once it plays video,
# you have all the basic dependencies.  Move onto the 
# Build ffmpeg locally
# 1. Assume you built a fresh local copy of ffmpeg using the right configure script.
# 2. Assume you did a make install to a safe local (non system directory)
# copy the right FFMPEG source files to a subdir here.

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

