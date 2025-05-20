gcc webform.c mongoose.c -o webform_server \
    -I. \
    -D_DARWIN_C_SOURCE \
    $(pkg-config --cflags sdl2) \
    $(pkg-config --libs sdl2) \
    -framework CoreFoundation -framework IOKit -framework Carbon -framework Metal -framework OpenGL -framework Cocoa -framework AudioToolbox -framework AudioUnit \
    -D_POSIX_C_SOURCE=200809L
