# Makefile for macOS using pkg-config

# Compiler
CC = gcc

# Project Name
TARGET = oneko_sdl  # Changed from xbm_helper to oneko_sdl

# Source Files
SRC = oneko_sdl.c sdl_xbm_helper.c # Added sdl_xbm_helper.c


# Object Files
OBJ = $(SRC:.c=.o)

# Compiler Flags
CFLAGS = -Wall -Wextra -pedantic

# Linker Flags
LDFLAGS =

# Use pkg-config to get SDL2 and SDL2_image flags
CFLAGS += $(shell pkg-config --cflags sdl2 sdl2_image)
LDFLAGS += $(shell pkg-config --libs sdl2 sdl2_image)

# Default target
all: $(TARGET)

# Compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link the executable
$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $(TARGET)

# Clean up object files
clean:
	rm -f $(OBJ)

# Remove the executable
distclean: clean
	rm -f $(TARGET)

# Run the executable
run: $(TARGET)
	./$(TARGET)

# Create a debug build (with debugging symbols)
debug:
	$(MAKE) CFLAGS="$(CFLAGS) -g" LDFLAGS="$(LDFLAGS)" TARGET=$(TARGET)_debug all
