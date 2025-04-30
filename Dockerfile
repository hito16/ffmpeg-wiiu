# original config script by GaryOderNicht
# modified by hito16
# NOTE: This docker image is meant for integration and learning.
# Since you will be building dependences like ffmpeg, this 
# docker file expects you to from build one level above
# ex. cd ..; docker build -t ffmpeg-wiiu ffmpeg-wiiu
# then extracting other depencencies in parallel directories
# ex. cd ..; ls 
#      ffmpeg/
#      ffmpeg-wiiu/
#      libromfs/
#       ....
# then you can get your hands dirty by logging into the container
#   docker run -it --rm -v ${PWD}:/project --name ffmpeg-wiiu ffmpeg-wiiu /bin/bash

FROM ghcr.io/wiiu-env/devkitppc:20241128

# satisfy ffmpeg configure requirement
RUN apt-get update && apt-get install -y gcc g++ make 

WORKDIR /project
