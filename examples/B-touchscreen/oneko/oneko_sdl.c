#include <SDL.h>
#include <math.h>  // For sqrt()
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For memset
#include "neko_image_states.h"
#include "neko_xbm.h" // Your XBM data
#include "sdl_xbm_helper.h"

// Neko structure to hold neko's state
typedef struct {
    int x, y;              // Current position
    int dx, dy;            // Direction of movement
    NekoRealStates state;  // Current direction, now from neko_image_states.h
    int frame;             // Current animation frame
    int sleeping;          // Is neko sleeping?
    int dont_move;
    int move_start;
    int frame_delay;
    int edge_timer; // Timer for edge animation.  It is used for wall collision.
    int last_x, last_y;
    int animation_delay_counter;
    int direction_change_counter;
} Neko;

// Global variables for animation timing
static Uint32 global_animation_timer = 0;

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

// Rules
// Never redefine neko_animations, import from neko_image_states.h
// Never access images using index number or ENUM.  Access using
//.   neko_animations.image_one and .image_two

int main(int argc, char *argv[]) {
    (void)argc; // Suppress unused parameter warning
    (void)argv; // Suppress unused parameter warning
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("Neko", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, 640, 480,
                              SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize Neko structure
    Neko neko;
    neko.x = 320; // Initial position
    neko.y = 240;
    neko.dx = 0;
    neko.dy = 0;
    neko.state = NEKO_STATE_AWAKE;
    neko.frame = 0;
    neko.sleeping = 0;
    neko.dont_move = 0;
    neko.move_start = 0;
    neko.frame_delay = 0;
    neko.edge_timer = 0;
    neko.last_x = 320;
    neko.last_y = 240;
    neko.animation_delay_counter = 0;
    neko.direction_change_counter = 0;

    SDL_Point mouse_pos;
    XbmImageData *images = xbm_images;

    SDL_Event event;
    int quit = 0;

    // Initialize global timer
    global_animation_timer = SDL_GetTicks();

    while (!quit) {
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            } else if (event.type == SDL_MOUSEMOTION) {
                mouse_pos.x = event.motion.x;
                mouse_pos.y = event.motion.y;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // printf("Mouse position: %d, %d\n", mouse_pos.x, mouse_pos.y);
        ProcessNeko(&neko, mouse_pos.x, mouse_pos.y, renderer, images);
        NekoThinkDraw(&neko, mouse_pos.x, mouse_pos.y, renderer, images);

        SDL_RenderPresent(renderer);
        SDL_Delay(32); // Increased delay to ~30 FPS (from 16 to 32)
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

NekoRealStates NekoDirectionCalc(int dx, int dy, int is_dont_move,
                                 NekoRealStates prev_state) {
    if (is_dont_move) {
        return NEKO_STATE_AWAKE;
    }

    if (dx == 0 && dy == 0) {
        return prev_state;
    }

    if (abs(dx) > abs(dy)) {
        if (dx > 0) {
            return NEKO_STATE_R_MOVE;
        } else {
            return NEKO_STATE_L_MOVE;
        }
    } else if (abs(dy) > abs(dx)) {
        if (dy > 0) {
            return NEKO_STATE_D_MOVE;
        } else {
            return NEKO_STATE_U_MOVE;
        }
    } else {
        if (dx > 0 && dy > 0) {
            return NEKO_STATE_DR_MOVE;
        } else if (dx > 0 && dy < 0) {
            return NEKO_STATE_UR_MOVE;
        } else if (dx < 0 && dy > 0) {
            return NEKO_STATE_DL_MOVE;
        } else {
            return NEKO_STATE_UL_MOVE;
        }
    }
}

int IsNekoDontMove(Neko *neko, int mouse_x, int mouse_y) {
    double dist = sqrt(pow(neko->x - mouse_x, 2) + pow(neko->y - mouse_y, 2));
    if (dist < 16) {
        return 1;
    }
    return 0;
}

int IsNekoMoveStart(Neko *neko, int mouse_x, int mouse_y) {
    double dist = sqrt(pow(neko->x - mouse_x, 2) + pow(neko->y - mouse_y, 2));
    if (dist > 16 && dist < 200) {
        return 1;
    }
    return 0;
}

void CalcDxDy(Neko *neko, int mouse_x, int mouse_y) {
    neko->dx = mouse_x - neko->x;
    neko->dy = mouse_y - neko->y;
}

void NekoThinkDraw(Neko *neko, int mouse_x, int mouse_y, SDL_Renderer *renderer,
                   XbmImageData *images) {
    int image_index = 0;
    int frame_count = 2;
    (void)mouse_x;
    (void)mouse_y;
    static int animation_frame = 0; // Static to keep track between calls

    // Use the global timer to switch between images
    Uint32 current_time = SDL_GetTicks();
    if (current_time - global_animation_timer >= 2000) { // 2000 milliseconds = 2 seconds
        global_animation_timer = current_time;
        animation_frame = 1 - animation_frame; // Toggle between 0 and 1
    }

    // Find the correct animation entry and get the image indices.
    for (int i = 0; i < sizeof(neko_animations) / sizeof(neko_animations[0]); i++) {
        if (neko_animations[i].state == neko->state) {
            if (neko_animations[i].image_two == -1) {
                image_index = neko_animations[i].image_one;
                frame_count = 1; // Only one frame
            } else {
                // Alternate between image_one and image_two based on animation_frame
                image_index = (animation_frame == 0) ? neko_animations[i].image_one : neko_animations[i].image_two;
                frame_count = 2;
            }
            break; // Found the entry, exit the loop
        }
    }

    // Handle sleeping as a special case.
    if (neko->sleeping) {
        if (neko->frame < 30) {
             for (int i = 0; i < sizeof(neko_animations) / sizeof(neko_animations[0]); i++) {
                if (neko_animations[i].state == NEKO_STATE_AWAKE)
                {
                    image_index = neko_animations[i].image_one;
                    frame_count = 1;
                    break;
                }
             }
        } else {
            for (int i = 0; i < sizeof(neko_animations) / sizeof(neko_animations[0]); i++) {
                if (neko_animations[i].state == NEKO_STATE_SLEEP)
                {
                     image_index = neko_animations[i].image_two;
                      frame_count = 1;
                      break;
                }
            }
        }
    }

    // Ensure image_index is within bounds
    if (image_index < 0 ||
        image_index >= sizeof(xbm_images) / sizeof(xbm_images[0])) {
        fprintf(stderr, "Error: image_index out of bounds: %d\n", image_index);
        return;
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
    SDL_FreeSurface(image_surface);

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
}

// Function to update Neko's state and position
void ProcessNeko(Neko *neko, int mouse_x, int mouse_y, SDL_Renderer *renderer,
                   XbmImageData *images) {
    (void)renderer;
    (void)images;

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
    // Increment direction change counter
    neko->direction_change_counter++;

    // Change direction (and thus state) only every 15 frames (for 2 changes per second)
    if (neko->direction_change_counter >= 15) {
        neko->state = NekoDirectionCalc(neko->dx, neko->dy, neko->dont_move, neko->state);
        neko->direction_change_counter = 0;
    }

    // Limit Neko's movement speed
    int max_speed = 2; // Adjust for desired max speed - reduced from 5 to 2
    double distance = sqrt(pow(neko->dx, 2) + pow(neko->dy, 2));
    if (distance > max_speed) {
        neko->dx = (int)(neko->dx * max_speed / distance);
        neko->dy = (int)(neko->dy * max_speed / distance);
    }

    neko->x += neko->dx;
    neko->y += neko->dy;

    // Wall collision detection and handling
    if (neko->x < 0) {
        neko->x = 0;
        neko->dx = -neko->dx;
        neko->edge_timer = 1;
    } else if (neko->x > 640) {
        neko->x = 640;
        neko->dx = -neko->dx;
        neko->edge_timer = 1;
    }
    if (neko->y < 0) {
        neko->y = 0;
        neko->dy = -neko->dy;
        neko->edge_timer = 1;
    } else if (neko->y > 480) {
        neko->y = 480;
        neko->dy = -neko->dy;
        neko->edge_timer = 1;
    }

    if (neko->edge_timer > 0) {
        neko->edge_timer++;
        if (neko->edge_timer > 30) {
            neko->edge_timer = 0;
        }
    }
}
