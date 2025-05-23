# Makefile for compiling SDL2 animation program on macOS
# Install homebrew
#  "/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
# Install SDL2
#  brew install sdl2
#  brew install sdl2_image
#  brew install sdl2_ttf
#  brew install pkgconf
#
#  Usage: make -f (this_file)

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g 

# Linker flags
#LDFLAGS = -framework SDL2 -framework SDL2_image
PKGCONF_MAC	:= /usr/local/Cellar/pkgconf/2.4.3/bin/pkgconf
CFLAGS		+=	$(shell $(PKGCONF_MAC) --cflags SDL2_mixer SDL2_image SDL2_ttf sdl2)
CXXFLAGS	+=	$(shell $(PKGCONF_MAC) --cflags SDL2_mixer SDL2_image SDL2_ttf sdl2)
LDFLAGS		+=	$(shell $(PKGCONF_MAC) --libs SDL2_mixer SDL2_image SDL2_ttf sdl2) \
				$(shell $(PKGMAC) --libs harfbuzz freetype2)

# Source file
SRC = sdlmain.c sdltriangle.c sdlanimate.c sdlaudio1.c


# Output executable name
TARGET = sdl-portable-app-macos

# Default rule: Build the executable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

# Clean rule: Remove the executable
clean:
	rm -f $(TARGET)
	rm -rf $(TARGET).dSYM


