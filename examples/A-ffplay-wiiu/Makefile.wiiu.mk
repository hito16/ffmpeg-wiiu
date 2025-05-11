# makefile to build ffplay executable on WiiU, as a sanity check
# assumes you configured and built ffmpeg with ffplay enabled
#  
# This file will be copied to and run from $FFMPEG_SRC, the root directory of the
#   ffmpeg master checkout

# ffplay is the same target built by the ffmpeg build scripts
FFPLAY_TARGET = ffplay
FFPLAY_SRC = fftools/ffplay_renderer.c fftools/cmdutils.c fftools/opt_common.c fftools/ffplay.c

# ffplay_lib is a patched version of ffplay, with main() renamed so it
#   can be built as a linkable static library
FFPLAY_LIB_TARGET = libffplay.a
FFPLAY_LIB_SRC = fftools/ffplay_renderer.c fftools/cmdutils.c fftools/opt_common.c fftools/ffplay_cli.c

# a generic main, to call our library.  Only used on MacOS. On WiiU, we link the library into sdlmain.c
FFPLAY_GENERIC_TARGET	= ffplay_generic 
FFPLAY_GENERIC_SRC	=	fftools/generic_main.c 

# Compiler
include $(DEVKITPRO)/wut/share/wut_rules
export LD	:=	$(CC)
$(info # ---- initial wut_root compiller variable values   ----)
$(info included CC		= $(CC))
$(info included CFLAGS		= $(CFLAGS))
$(info # ---- initial wut_root linker variable values   ----)
$(info included LD		= $(LD))
$(info included LDFLAGS		= $(LDFLAGS))
$(info included AR		= $(AR))

CFLAGS	:=	-O3 -ffunction-sections -fdata-sections \
			$(MACHDEP) \
			-D_ISOC11_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -U__STRICT_ANSI__ \
			-D_XOPEN_SOURCE=600 \
			-D__WIIU__ \
			-fomit-frame-pointer \
			-ffast-math -fno-math-errno  -funsafe-math-optimizations -ffinite-math-only -fno-trapping-math \
			-O3 -Ofast -std=c11 -mcpu=750 -meabi -mhard-float 
LDFLAGS :=	-Wl,-q -Wl,-z,nocopyreloc -specs=$(WUT_ROOT)/share/wut.specs


# Path to pkg-config (adjust if necessary)
# export PKG_CONFIG_PATH=/Users/scottstraw/Downloads/build/lib/pkgconfig
#PKG_CONFIG_PATH = $(DEVKITPRO)/portlibs/ppc/lib/pkgconfig:$(DEVKITPRO)/portlibs/wiiu/lib/pkgconfig
#export PKG_CONFIG_PATH
$(info PKG_CONFIG_PATH $(PKG_CONFIG_PATH))
PKGCONF := /usr/bin/pkg-config

# External dependencies
FFMPEG_CFLAGS = $(shell $(PKGCONF) --cflags libavdevice libavfilter libavformat libavcodec libavutil libswresample libswscale)
FFMPEG_LDFLAGS = $(shell $(PKGCONF) --libs libavdevice libavfilter libavformat libavcodec libavutil libswresample libswscale)
SDL_CFLAGS = $(shell $(PKGCONF) --cflags SDL2_ttf sdl2)
SDL_LDFLAGS = $(shell $(PKGCONF) --libs SDL2_ttf sdl2) \
            $(shell $(PKGCONF) --libs harfbuzz freetype2)

# Combine CFLAGS and LDFLAGS (you can choose which ones to apply to ffplay)
FFPLAY_CFLAGS = $(CFLAGS) $(FFMPEG_CFLAGS) $(SDL_CFLAGS) -I.  -I$(WUT_ROOT)/include
FFPLAY_LDFLAGS = $(LDFLAGS) $(FFMPEG_LDFLAGS) $(SDL_LDFLAGS) -L$(WUT_ROOT)/lib 
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
$(info FFPLAY_CFLAGS $(FFPLAY_CFLAGS))
$(info FFPLAY_LDFLAGS $(FFPLAY_LDFLAGS))
$(info SDL_CFLAGS $(FFPLAY_CFLAGS))
$(info SDL_LDFLAGS $(FFPLAY_LDFLAGS))

# Build rule for ffplay
$(FFPLAY_TARGET): $(FFPLAY_OBJ)
	@echo "Linking a non-functional, but generic binary $(FFPLAY_TARGET)..."
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
	$(CC) $(FFPLAY_CFLAGS) $(SDL_CFLAGS) -c $< -o $@

# Clean rule
clean:
	@echo "Cleaning..."
	rm -f $(FFPLAY_TARGET) $(FFPLAY_BUILD_DIR)/*.o
	rm -rf $(FFPLAY_BUILD_DIR)
	@echo "Clean complete!"

.PHONY: clean
