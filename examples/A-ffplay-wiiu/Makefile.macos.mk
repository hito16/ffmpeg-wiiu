# makefile to build ffplay executable on Mac
# assumes you configured and built ffmpeg with ffplay enabled
#  
# This will be copied to and run from $FFMPEG_SRC, the root directory of the
#   ffmpeg master checkout
# 

# ffplay is the same target built by the ffmpeg build scripts
FFPLAY_TARGET = ffplay
FFPLAY_SRC = fftools/ffplay_renderer.c fftools/cmdutils.c fftools/opt_common.c fftools/ffplay.c

# ffplay_lib is a patched version of ffplay with main() renamed so it 
#   can be built as a linkable static library 
FFPLAY_LIB_TARGET = libffplay.a
FFPLAY_LIB_SRC = fftools/ffplay_renderer.c fftools/cmdutils.c fftools/opt_common.c fftools/ffplay_cli.c

# a generic main, to call our library.  Only used on MacOS. On WiiU, we link the library into sdlmain.c
FFPLAY_GENERIC_TARGET	= ffplay_generic 
FFPLAY_GENERIC_SRC	=	fftools/generic_main.c 

# Compiler
CC = gcc

# Compiler flags (you might want to add general flags here)
# for -g debugging
#  gcc -Og -gdwarf-4 helloworld.c -o helloworld
# (gdb) set startup-with-shell off
CFLAGS = -g -Og -gdwarf-4 -Wall -Werror -Wno-switch -Wno-pointer-sign -Wno-parentheses  -std=c11 -I.

# Linker flags

# Path to pkg-config (adjust if necessary)
# export PKG_CONFIG_PATH=/Users/scottstraw/Downloads/build/lib/pkgconfig
PKGCONF_MAC := /usr/local/bin/pkgconf


# External dependencies
FFMPEG_CFLAGS = $(shell $(PKGCONF_MAC) --cflags libavdevice libavfilter libavformat libavcodec libavutil libswresample libswscale)
FFMPEG_LDFLAGS = $(shell $(PKGCONF_MAC) --libs libavdevice libavfilter libavformat libavcodec libavutil libswresample libswscale)
SDL_CFLAGS = $(shell $(PKGCONF_MAC) --cflags SDL2_ttf sdl2)
SDL_LDFLAGS = $(shell $(PKGCONF_MAC) --libs SDL2_ttf sdl2) \
            $(shell $(PKGCONF_MAC) --libs harfbuzz freetype2)

# Combine CFLAGS and LDFLAGS (you can choose which ones to apply to ffplay)
FFPLAY_CFLAGS = $(CFLAGS) $(FFMPEG_CFLAGS) $(SDL_CFLAGS) -I. 
FFPLAY_LDFLAGS = $(LDFLAGS) $(FFMPEG_LDFLAGS) $(SDL_LDFLAGS)
FFPLAY_EXTRALIBS = # Add any extra libraries ffplay might need

# Object file output directory
FFPLAY_BUILD_DIR = build_ffplay

# Helper function to get the object file path
define OBJ_PATH
$(FFPLAY_BUILD_DIR)/$(notdir $(basename $(1))).o
endef

FFPLAY_OBJ = $(foreach src,$(FFPLAY_SRC),$(call OBJ_PATH,$(src)))
FFPLAY_LIB_OBJ = $(foreach src,$(FFPLAY_LIB_SRC),$(call OBJ_PATH,$(src)))
$(info FFPLAY_SRC $(FFPLAY_SRC))
$(info FFPLAY_LIB_SRC $(FFPLAY_LIB_SRC))
$(info FFPLAY_OBJ $(FFPLAY_OBJ))
$(info FFPLAY_LIB_OBJ $(FFPLAY_LIB_OBJ))

# Build rule for ffplay
$(FFPLAY_TARGET): $(FFPLAY_OBJ)
	@echo "Linking $(FFPLAY_TARGET)..."
	@mkdir -p $(FFPLAY_BUILD_DIR)
	$(CC) $(FFPLAY_CFLAGS) $(FFPLAY_OBJ) -o $(FFPLAY_TARGET) $(FFPLAY_LDFLAGS) $(FFPLAY_EXTRALIBS)
	@echo "Build of $(FFPLAY_TARGET) complete!"

$(FFPLAY_GENERIC_TARGET): $(FFPLAY_LIB_TARGET)
	@echo "Linking $(FFPLAY_GENERIC_TARGET)..."
	@mkdir -p $(FFPLAY_BUILD_DIR)
	$(CC) $(FFPLAY_CFLAGS)  $(FFPLAY_GENERIC_SRC) -o $(FFPLAY_GENERIC_TARGET) $(FFPLAY_LDFLAGS) $(FFPLAY_EXTRALIBS) -L. -lffplay


$(FFPLAY_LIB_TARGET): $(FFPLAY_LIB_OBJ)
	@echo "Creating static library $(FFPLAY_LIB_TARGET)..."
	@mkdir -p $(FFPLAY_BUILD_DIR)  
	$(AR) $(ARFLAGS) $(FFPLAY_LIB_TARGET) $(FFPLAY_LIB_OBJ)
	@echo "Build of $(FFPLAY_LIB_TARGET) complete!"

# Explicit rule to compile each object file
$(FFPLAY_BUILD_DIR)/%.o: fftools/%.c
	@echo "Compiling $< to $@"
	@mkdir -p $(FFPLAY_BUILD_DIR)
	$(CC) $(FFPLAY_CFLAGS) -c $< -o $@

# Clean rule
clean:
	@echo "Cleaning..."
	rm -f $(FFPLAY_TARGET) $(FFPLAY_BUILD_DIR)/*.o
	rm -rf $(FFPLAY_BUILD_DIR)
	@echo "Clean complete!"

.PHONY: clean
