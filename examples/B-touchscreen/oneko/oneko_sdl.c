#include <SDL.h>
#include <math.h>  // For sqrt()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // For memset
#include "neko_image_states.h"
#include "neko_xbm.h"           // Your XBM data
#include "sdl_xbm_helper.h"

// Neko structure to hold neko's state
typedef struct {
    int x, y;              // Current position
    int dx, dy;            // Direction of movement
    NekoRealStates state;  // Current direction, now from xbm_images.h
    int frame;             // Current animation frame
    int sleeping;          // Is neko sleeping?
    int dont_move;
    int move_start;
    int frame_delay;
    int edge_timer;  // Timer for edge animation
    int last_x, last_y;
} Neko;

// Function declarations
NekoRealStates NekoDirectionCalc(int dx, int dy, int is_dont_move,
                                 NekoRealStates prev_state);
int IsNekoDontMove(Neko *neko, int mouse_x, int mouse_y);
int IsNekoMoveStart(Neko *neko, int mouse_x, int mouse_y);
void CalcDxDy(Neko *neko, int mouse_x, int mouse_y);
void NekoThinkDraw(Neko *neko, int mouse_x, int mouse_y, SDL_Renderer *renderer,
                   XbmImageData *images);
void ProcessNeko(Neko *neko, int mouse_x, int mouse_y, SDL_Renderer *renderer,
                   XbmImageData *images);

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window =
        SDL_CreateWindow("Neko", SDL_WINDOWPOS_UNDEFINED,
                         SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize Neko's state
    Neko neko;
    neko.x = 320; //initial x
    neko.y = 240; //initial y
    neko.dx = 0;
    neko.dy = 0;
    neko.state = NEKO_STATE_AWAKE;
    neko.frame = 0;
    neko.sleeping = 0;
    neko.dont_move = 0;
    neko.move_start = 0;
    neko.frame_delay = 0;
    neko.edge_timer = 0;
    neko.last_x = 0;
    neko.last_y = 0;

    SDL_Event event;
    int quit = 0;
    int sleeping_frame_count = 0;

    // Main loop
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    neko.sleeping = !neko.sleeping;
                    neko.dont_move = 0;
                    sleeping_frame_count = 0;
                }
            }
        }

        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);

        // If Neko is sleeping, increment the timer.
        if (neko.sleeping) {
            sleeping_frame_count++;
        }
        // After 5 seconds of sleeping, play the snoring animation
        if (sleeping_frame_count > 5 * 60) {
            neko.state = NEKO_STATE_SLEEP;
        }

        ProcessNeko(&neko, mouse_x, mouse_y, renderer, xbm_images);

        // Clear the renderer...
        SDL_RenderClear(renderer);

        // Draw Neko
        NekoThinkDraw(&neko, mouse_x, mouse_y, renderer, xbm_images);

        // Update the screen
        SDL_RenderPresent(renderer);

        // Cap frame rate to 60 FPS, adjusted for 3x slower rendering
        SDL_Delay(1000 / 20); // Change 60 to 20 (60 / 3 = 20)
    }

    // Clean up
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

// Function to calculate Neko's direction, same as the original oneko.c
NekoRealStates NekoDirectionCalc(int dx, int dy, int is_dont_move,
                                 NekoRealStates prev_state) {
    if (is_dont_move) {
        return prev_state;
    }
    if (dx == 0 && dy == 0) {
        return NEKO_STATE_AWAKE;
    } else if (dx == 0 && dy > 0) {
        return NEKO_STATE_MOVE_DOWN;
    } else if (dx > 0 && dy > 0) {
        return NEKO_STATE_MOVE_DWRIGHT;
    } else if (dx < 0 && dy > 0) {
        return NEKO_STATE_MOVE_DWLEFT;
    } else if (dx < 0 && dy == 0) {
        return NEKO_STATE_MOVE_LEFT;
    } else if (dx > 0 && dy == 0) {
        return NEKO_STATE_MOVE_RIGHT;
    } else if (dx == 0 && dy < 0) {
        return NEKO_STATE_MOVE_UP;
    } else if (dx < 0 && dy < 0) {
        return NEKO_STATE_MOVE_UPLEFT;
    } else if (dx > 0 && dy < 0) {
        return NEKO_STATE_MOVE_UPRIGHT;
    }
    return NEKO_STATE_AWAKE;
}

// Function to check if Neko should stop moving
int IsNekoDontMove(Neko *neko, int mouse_x, int mouse_y) {
    int distance = sqrt(pow(neko->x - mouse_x, 2) + pow(neko->y - mouse_y, 2));
    if (distance < 5) {
        return 1;
    } else {
        return 0;
    }
}

// Function to check if Neko should start moving
int IsNekoMoveStart(Neko *neko, int mouse_x, int mouse_y) {
    int distance = sqrt(pow(neko->x - mouse_x, 2) + pow(neko->y - mouse_y, 2));
    if (distance > 10) {
        return 1;
    } else {
        return 0;
    }
}

// Function to calculate Neko's movement direction
void CalcDxDy(Neko *neko, int mouse_x, int mouse_y) {
    if (neko->dont_move) {
        neko->dx = 0;
        neko->dy = 0;
    } else {
        neko->dx = mouse_x - neko->x;
        neko->dy = mouse_y - neko->y;
    }
}

// Function to draw Neko based on its state
void NekoThinkDraw(Neko *neko, int mouse_x, int mouse_y, SDL_Renderer *renderer,
                   XbmImageData *images) {
    int image_index = 0;
    int frame_count = 2; // Default to 2 frames for most animations

    // Determine Neko's state and animation frame
     if (neko->sleeping)
    {
        if (neko->frame < 30)
        {
             image_index = 0;
        }
        else if (neko->frame < 60)
        {
            image_index = 17;
        }
        else
        {
            image_index = 18;
        }
    }
    else
    {
        switch (neko->state) {
        case NEKO_STATE_AWAKE:
            image_index = 0;
            frame_count = 1;
            break;
        case NEKO_STATE_MOVE_DOWN:
            image_index = (neko->frame < 15) ? 1 : 2;
            break;
        case NEKO_STATE_MOVE_DWLEFT:
            image_index = (neko->frame < 15) ? 5 : 6;
            break;
        case NEKO_STATE_MOVE_DWRIGHT:
            image_index = (neko->frame < 15) ? 7 : 8;
            break;
        case NEKO_STATE_MOVE_LEFT:
            image_index = (neko->frame < 15) ? 9 : 10;
            break;
         case NEKO_STATE_MOVE_RIGHT:
            image_index = (neko->frame < 15) ? 13 : 14;
            break;
        case NEKO_STATE_MOVE_UP:
            image_index = (neko->frame < 15) ? 19 : 20;
            break;
        case NEKO_STATE_MOVE_UPLEFT:
             image_index = (neko->frame < 15) ? 21 : 22;
            break;
        case NEKO_STATE_MOVE_UPRIGHT:
            image_index = (neko->frame < 15) ? 23 : 24;
            break;
        case NEKO_STATE_SLEEP:
            image_index = 17;
            frame_count = 3;
            break;
        case NEKO_STATE_FROLIC:
             image_index = 30;
             frame_count = 1;
             break;
        default:
            image_index = 0;
            frame_count = 1;
            break;
        }
    }


    // Ensure image_index is within bounds
    if (image_index < 0 || image_index >= sizeof(xbm_images) / sizeof(xbm_images[0])) {
        fprintf(stderr, "Error: image_index out of bounds: %d\n", image_index);
        return; // Or handle the error as appropriate for your application
    }
    SDL_Surface *image_surface = create_surface_from_xbm(
        images[image_index].bits, images[image_index].width,
        images[image_index].height);
    if (!image_surface) {
        fprintf(stderr, "Failed to create surface for image index: %d\n",
                image_index);
        return;
    }

    SDL_Texture *texture =
        SDL_CreateTextureFromSurface(renderer, image_surface);
    SDL_FreeSurface(image_surface); // Clean up the surface

    if (!texture) {
        fprintf(stderr, "SDL_CreateTextureFromSurface failed: %s\n",
                SDL_GetError());
        return;
    }

    SDL_Rect dest_rect;
    dest_rect.x = neko->x - images[image_index].width / 2;
    dest_rect.y = neko->y - images[image_index].height / 2;
    dest_rect.w = images[image_index].width;
    dest_rect.h = images[image_index].height;

    SDL_RenderCopy(renderer, texture, NULL, &dest_rect);
    SDL_DestroyTexture(texture);

    // Update frame counter, handle sleep frame animation,
    neko->frame++;
    if (neko->frame >= 60)
    {
        neko->frame = 0;
    }
    if (neko->sleeping)
    {
         if (neko->frame == 30 || neko->frame == 60)
         {
            neko->frame = 0;
         }
    }

}

// Function to update Neko's state and position
void ProcessNeko(Neko *neko, int mouse_x, int mouse_y, SDL_Renderer *renderer,
                   XbmImageData *images) {
    if (neko->sleeping) {
        return;
    }

    if (IsNekoDontMove(neko, mouse_x, mouse_y)) {
        neko->dont_move = 1;
    } else {
        neko->dont_move = 0;
    }

    if (IsNekoMoveStart(neko, mouse_x, mouse_y)) {
        neko->move_start = 1;
    } else {
        neko->move_start = 0;
    }

    CalcDxDy(neko, mouse_x, mouse_y);
    neko->state = NekoDirectionCalc(neko->dx, neko->dy, neko->dont_move,
                                     neko->state);

    if (neko->dont_move) {
        return;
    }

    neko->x += neko->dx * 0.1;
    neko->y += neko->dy * 0.1;

    // Keep Neko within bounds (assuming window size of 640x480)
    if (neko->x < 0)
        neko->x = 0;
    if (neko->x > 640)
        neko->x = 640;
    if (neko->y < 0)
        neko->y = 0;
    if (neko->y > 480)
        neko->y = 480;
}

