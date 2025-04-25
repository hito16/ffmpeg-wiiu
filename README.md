## ffmpeg-wiiu

### Instructions:
 Make sure you have exported $DEVKITPRO, $DEVKITPPC and $WUT_ROOT vars
 Clone the latest ffmpeg (https://github.com/FFmpeg/FFmpeg)
 Copy the configure script (configure_wiiu) inside repo's root

 sample workflow 
```
 # create a new directory "$HOME/Documents/code"
 # download latest ffmpeg source zip an extract to folder "ffmpeg"
 git clone https://github.com/hito16/ffmpeg-wiiu.git ffmpeg-wiiu
 docker build -t ffmpeg-wiiu ffmpeg-wiiu
 docker run -it --rm -v ${PWD}:/project ffmpeg-wiiu /bin/bash
```

 once in the container
```
  cp ffmpeg-wiiu/configure_wiiu ffmpeg   
  cd ffmpeg
  ./configure_wiiu
  make
  # in docker, you will install ffmpeg to $DEVKITPRO/portlibs
  make install
```

