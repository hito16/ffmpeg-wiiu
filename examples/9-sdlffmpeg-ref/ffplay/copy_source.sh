#!/bin/bash
# Build ffmpeg locally
# 1. Assume you built a fresh local copy of ffmpeg using the right configure script.
# 2. Assume you did a make install to a safe local (non system directory)
# copy the right FFMPEG source files to a subdir here.

if [[ -z "$FFMPEG_SRC" ]]; then
  echo '$FFMPEG_SRC must be set to the root of your FFMPEG source tree checkout'
  echo 'ex.  export FFMPEG_SRC=$HOME/Documents/code/ffmpeg'
  exit 1
fi

if [[ -z "$PKG_CONFIG_PATH" ]]; then
  echo '$PKG_CONFIG_PATH should be set to a custom "make install" directory'
  echo ' for compatibility reasons, we will rebuild ffmpeg and install '
  echo ' them to a local user directory.  We will then build ffmpeg off these'
  echo ' locally installed libraries, rather than use the system verions.  '
  exit 1
fi

echo FFMPEG_SRC=$FFMPEG_SRC
echo PKG_CONFIG_PATH=$PKG_CONFIG_PATH

cp configure_wiiu_ffplay $FFMPEG_SRC


if ! [ -d "$FFMPEG_SRC/fftools" ]; then
  echo "$FFMPEG_SRC/fftools not found"
  exit 1
fi

echo "Copying ffplay and dependencies from source"
cp -av "${FFMPEG_SRC}/fftools" .
rm  fftools/*.d  fftools/*.o fftools/textformat/*.d fftools/textformat/*.o
