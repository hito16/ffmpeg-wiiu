# original config script by GaryOderNicht
# modified by hito16
# NOTE: This docker image is meant for integration and learning.
# Since you will be building dependences like ffmpeg, this 
# docker file expects you to from build one level above
# ex. cd ..; docker build -t ffmpeg-wiiu ffmpeg-wiiu
# then extracting other depencencies in parallel directories
# ex. cd ..; ls 
#      FFmpeg-master/   <= ffmpeg source  
#      ffmpeg-wiiu/
#      libromfs-wiiu-master/
#       ....
# then you can get your hands dirty by logging into the container
#   docker run -it --rm -v ${PWD}:/project --name ffmpeg-wiiu ffmpeg-wiiu /bin/bash

FROM ghcr.io/wiiu-env/devkitppc:20241128

# satisfy ffmpeg configure requirement
RUN apt-get update && apt-get install -y gcc g++ make 

# temporarily add while we learn
RUN apt-get install -y vim file

# Assuming you downloaded libromfs-wiiu-master and installed in adjacent dir
ENV FFMPEG_SRC=/project/FFmpeg-master
ENV PKG_CONFIG_PATH="$DEVKITPRO/portlibs/ppc/lib/pkgconfig:$DEVKITPRO/portlibs/wiiu/lib/pkgconfig"

WORKDIR /project
