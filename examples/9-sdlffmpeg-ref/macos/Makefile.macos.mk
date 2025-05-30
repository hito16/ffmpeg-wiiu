# Makefile for compiling SDL2 animation program on macOS
# Install homebrew
#  "/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
# Install SDL2, FFMPEG and pkgconfig
#  brew install sdl2
#  brew install sdl2_image
#  brew install sdl2_ttf
#  brew install ffmpeg
#  brew install pkgconf

TARGET = sdl-ffmpeg-macos

# Source file
#SRC	=  sdl-display4.c
#SRC	=  ffmpeg-decode5.c
#SRC	=  ffmpeg-sync2.c
#SRC	=  sdlfilepicker4.c
#SRC	=  ffmpeg-playvid.c
SRC	=  ffmpeg-playaud6.c

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Werror -g

# Linker flags

#PKGCONF_MAC	:= /opt/homebrew/bin/pkgconf
PKGCONF_MAC	:= /usr/local/bin/pkgconf
# general order -lavformat -lavcodec -lavutil -lswresample -lswscale -lm -lz
FFMPEG_CFLAGS = $(shell $(PKGCONF_MAC) --cflags libavformat libavcodec libavutil libswresample libswscale)
FFMPEG_LDFLAGS = $(shell $(PKGCONF_MAC) --libs libavformat libavcodec libavutil libswresample libswscale)
SDL_CFLAGS = $(shell $(PKGCONF_MAC) --cflags SDL2_ttf sdl2)
SDL_LDFLAGS = $(shell $(PKGCONF_MAC) --libs SDL2_ttf sdl2) \
              $(shell $(PKGCONF_MAC) --libs harfbuzz freetype2)

# Combine CFLAGS and LDFLAGS
CFLAGS += $(FFMPEG_CFLAGS) $(SDL_CFLAGS)
LDFLAGS += $(FFMPEG_LDFLAGS) $(SDL_LDFLAGS)


# Object file output directory
BUILD_DIR = build_mac

# Output executable name
# Object file
OBJ = $(BUILD_DIR)/$(SRC:.c=.o)

# Default rule: build the executable
$(TARGET): $(OBJ)
	@echo "Linking..."
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete!"

# Rule to compile the source file to an object file
$(BUILD_DIR)/%.o: %.c
	@echo "Compiling $< to $(BUILD_DIR)/$@"
	@mkdir -p $(BUILD_DIR) # Create the directory if it doesn't exist
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule: remove the executable and object files
clean:
	@echo "Cleaning..."
	rm -f $(TARGET) $(BUILD_DIR)/$(OBJ) #remove the .o from the build directory
	rm -rf $(BUILD_DIR) # Remove the build directory
	@echo "Clean complete!"

.PHONY: clean # Declare 'clean' as not a real file.

