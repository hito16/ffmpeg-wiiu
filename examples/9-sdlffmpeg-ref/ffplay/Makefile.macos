FFPLAY_TARGET = ffplay
FFPLAY_SRC = fftools/ffplay_renderer.c fftools/cmdutils.c fftools/opt_common.c fftools/ffplay.c

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
FFPLAY_CFLAGS = $(CFLAGS) $(FFMPEG_CFLAGS) $(SDL_CFLAGS) -I. -I../../../../ffmpeg
FFPLAY_LDFLAGS = $(LDFLAGS) $(FFMPEG_LDFLAGS) $(SDL_LDFLAGS)
FFPLAY_EXTRALIBS = # Add any extra libraries ffplay might need

# Object file output directory
FFPLAY_BUILD_DIR = build_ffplay

# Helper function to get the object file path
define OBJ_PATH
$(FFPLAY_BUILD_DIR)/$(notdir $(basename $(1))).o
endef

FFPLAY_OBJ = $(foreach src,$(FFPLAY_SRC),$(call OBJ_PATH,$(src)))

# Build rule for ffplay
$(FFPLAY_TARGET): $(FFPLAY_OBJ)
	@echo "Linking $(FFPLAY_TARGET)..."
	@mkdir -p $(FFPLAY_BUILD_DIR)
	$(CC) $(FFPLAY_CFLAGS) $(FFPLAY_OBJ) -o $(FFPLAY_TARGET) $(FFPLAY_LDFLAGS) $(FFPLAY_EXTRALIBS)
	@echo "Build of $(FFPLAY_TARGET) complete!"

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
