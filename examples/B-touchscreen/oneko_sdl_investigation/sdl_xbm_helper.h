#ifndef SDL_XBM_HELPER_H
#define SDL_XBM_HELPER_H

#include <SDL.h>

// Function to create an SDL surface from XBM data
SDL_Surface* create_surface_from_xbm(const unsigned char* xbm_data, int width,
                                     int height);

#endif // SDL_XBM_HELPER_H
