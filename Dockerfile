# original config script by GaryOderNicht
# modified by hito16
FROM ghcr.io/wiiu-env/devkitppc:20241128

# satisfy ffmpeg configure requirement
RUN apt-get update && apt-get install -y gcc make

WORKDIR /project
