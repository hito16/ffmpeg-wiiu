// sdl_oneko_main.c

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h> // For exit()
#include <math.h>   // For distance calculations (will be needed for movement logic)

// Include your XBM helper header.
// This header MUST contain the declaration for load_xbm_as_texture AND create_surface_from_xbm.
#include "sdl_xbm_helper.h"

// Include the original oneko header.
// This will include the XBM data and define structures like BitmapGCData and AnimationPattern,
// and likely the animation state enums/constants (like NEKO_STOP, NEKO_SLEEP, etc.).
#include "oneko.h"

// Define some basic window dimensions.
#define INITIAL_WINDOW_WIDTH 640
#define INITIAL_WINDOW_HEIGHT 480

// We no longer define CHARACTER_FRAME_COUNT here.
// The size of BitmapTXDataTable will be determined by the initializer list.
// The number of frames will be counted during initialization by looping until the NULL entry.

// Define a structure to hold the SDL-compatible bitmap data for each frame.
// Renamed from SDLBitmapData to BitmapTXData.
typedef struct {
    // Pointers to the original raw XBM pixel and mask data (reused from included XBMs)
    const unsigned char *image_bits;
    const unsigned char *mask_bits;
    int width;
    int height;

    // SDL-specific texture created from the XBM data. Initialized to NULL.
    SDL_Texture *texture;

} BitmapTXData; // Renamed struct


// Temporary enum to provide human-readable names for the indices in BitmapTXDataTable.
// The order of these enum values MUST match the order of entries in BitmapTXDataTable.
typedef enum {
    TX_MATI2 = 0,
    TX_JARE2,
    TX_KAKI1,
    TX_KAKI2,
    TX_MATI3,
    TX_SLEEP1,
    TX_SLEEP2,
    TX_AWAKE,
    TX_UP1,
    TX_UP2,
    TX_DOWN1,
    TX_DOWN2,
    TX_LEFT1,
    TX_LEFT2,
    TX_RIGHT1,
    TX_RIGHT2,
    TX_UPLEFT1,
    TX_UPLEFT2,
    TX_UPRIGHT1,
    TX_UPRIGHT2,
    TX_DWLEFT1,
    TX_DWLEFT2,
    TX_DWRIGHT1,
    TX_DWRIGHT2,
    TX_UTOGI1,
    TX_UTOGI2,
    TX_DTOGI1,
    TX_DTOGI2,
    TX_LTOGI1,
    TX_LTOGI2,
    TX_RTOGI1,
    TX_RTOGI2,
    // Add other texture indices if more entries are added to BitmapTXDataTable
    TX_NULL // Corresponds to the NULL terminator entry
} TextureIndex;


// Declare and declaratively initialize our BitmapTXData table.
// Renamed from character_bitmap_data_table to BitmapTXDataTable.
// We use the named XBM data variables directly here, mirroring the original
// BitmapGCDataTable initialization. The 'texture' member is initialized to NULL.
// The InitBitmapData function will later populate the 'texture' members.
// Based on the provided BitmapGCDataTable initialization snippet.
// The compiler will automatically determine the size of the array based on the initializer list.
// clang-format off
BitmapTXData BitmapTXDataTable[] = { // Renamed array
    // { image_bits, mask_bits, width, height, texture (NULL) }
    // Added explicit casts to const unsigned char* to resolve pointer-sign warnings.
    { (const unsigned char*)mati2_bits, (const unsigned char*)mati2_mask_bits, mati2_width, mati2_height, NULL }, // 0: Standing (Mati2)
    { (const unsigned char*)jare2_bits, (const unsigned char*)jare2_mask_bits, jare2_width, jare2_height, NULL }, // 1: Walking 1 Right (Jare2)
    { (const unsigned char*)kaki1_bits, (const unsigned char*)kaki1_mask_bits, kaki1_width, kaki1_height, NULL }, // 2: Walking 2 Right (Kaki1)
    { (const unsigned char*)kaki2_bits, (const unsigned char*)kaki2_mask_bits, kaki2_width, kaki2_height, NULL }, // 3: Walking 3 Right (Kaki2)
    { (const unsigned char*)mati3_bits, (const unsigned char*)mati3_mask_bits, mati3_width, mati3_height, NULL }, // 4: Standing (Mati3) - Often another standing frame
    { (const unsigned char*)sleep1_bits, (const unsigned char*)sleep1_mask_bits, sleep1_width, sleep1_height, NULL }, // 5: Sleeping 1
    { (const unsigned char*)sleep2_bits, (const unsigned char*)sleep2_mask_bits, sleep2_width, sleep2_height, NULL }, // 6: Sleeping 2
    { (const unsigned char*)awake_bits, (const unsigned char*)awake_mask_bits, awake_width, awake_height, NULL }, // 7: Awake (often after sleep)
    { (const unsigned char*)up1_bits, (const unsigned char*)up1_mask_bits, up1_width, up1_height, NULL }, // 8: Up 1
    { (const unsigned char*)up2_bits, (const unsigned char*)up2_mask_bits, up2_width, up2_height, NULL }, // 9: Up 2
    { (const unsigned char*)down1_bits, (const unsigned char*)down1_mask_bits, down1_width, down1_height, NULL }, // 10: Down 1
    { (const unsigned char*)down2_bits, (const unsigned char*)down2_mask_bits, down2_width, down2_height, NULL }, // 11: Down 2
    { (const unsigned char*)left1_bits, (const unsigned char*)left1_mask_bits, left1_width, left1_height, NULL }, // 12: Left 1
    { (const unsigned char*)left2_bits, (const unsigned char*)left2_mask_bits, left2_width, left2_height, NULL }, // 13: Left 2
    { (const unsigned char*)right1_bits, (const unsigned char*)right1_mask_bits, right1_width, right1_height, NULL }, // 14: Right 1
    { (const unsigned char*)right2_bits, (const unsigned char*)right2_mask_bits, right2_width, right2_height, NULL }, // 15: Right 2
    { (const unsigned char*)upleft1_bits, (const unsigned char*)upleft1_mask_bits, upleft1_width, upleft1_height, NULL }, // 16: Up-Left 1
    { (const unsigned char*)upleft2_bits, (const unsigned char*)upleft2_mask_bits, upleft2_width, upleft2_height, NULL }, // 17: Up-Left 2
    { (const unsigned char*)upright1_bits, (const unsigned char*)upright1_mask_bits, upright1_width, upright1_height, NULL }, // 18: Up-Right 1
    { (const unsigned char*)upright2_bits, (const unsigned char*)upright2_mask_bits, upright2_width, upright2_height, NULL }, // 19: Up-Right 2
    { (const unsigned char*)dwleft1_bits, (const unsigned char*)dwleft1_mask_bits, dwleft1_width, dwleft1_height, NULL }, // 20: Down-Left 1
    { (const unsigned char*)dwleft2_bits, (const unsigned char*)dwleft2_mask_bits, dwleft2_width, dwleft2_height, NULL }, // 21: Down-Left 2
    { (const unsigned char*)dwright1_bits, (const unsigned char*)dwright1_mask_bits, dwright1_width, dwright1_height, NULL },
    { (const unsigned char*)dwright2_bits, (const unsigned char*)dwright2_mask_bits, dwright2_width, dwright2_height, NULL }, // 23: Down-Right 2
    { (const unsigned char*)utogi1_bits, (const unsigned char*)utogi1_mask_bits, utogi1_width, utogi1_height, NULL }, // 24: Up Scratching 1 (utogi)
    { (const unsigned char*)utogi2_bits, (const unsigned char*)utogi2_mask_bits, utogi2_width, utogi2_height, NULL }, // 25: Up Scratching 2
    { (const unsigned char*)dtogi1_bits, (const unsigned char*)dtogi1_mask_bits, dtogi1_width, dtogi1_height, NULL }, // 26: Down Scratching 1
    { (const unsigned char*)dtogi2_bits, (const unsigned char*)dtogi2_mask_bits, dtogi2_width, dtogi2_height, NULL }, // 27: Down Scratching 2
    { (const unsigned char*)ltogi1_bits, (const unsigned char*)ltogi1_mask_bits, ltogi1_width, ltogi1_height, NULL }, // 28: Left Scratching 1
    { (const unsigned char*)ltogi2_bits, (const unsigned char*)ltogi2_mask_bits, ltogi2_width, ltogi2_height, NULL }, // 29: Left Scratching 2
    { (const unsigned char*)rtogi1_bits, (const unsigned char*)rtogi1_mask_bits, rtogi1_width, rtogi1_height, NULL }, // 30: Right Scratching 1
    { (const unsigned char*)rtogi2_bits, (const unsigned char*)rtogi2_mask_bits, rtogi2_width, rtogi2_height, NULL }, // 31: Right Scratching 2

    // The final NULL entry as provided in the snippet
    { NULL, NULL, 0, 0, NULL } // 32: NULL Terminator
};
// clang-format on

// The total number of animation frames (excluding the NULL terminator).
// We can calculate this from the size of the BitmapTXDataTable.
#define TOTAL_BITMAP_FRAMES (sizeof(BitmapTXDataTable) / sizeof(BitmapTXData) - 1) // Renamed define


// Define an SDL-compatible Animation Pattern structure.
// Renamed from SDLAnimationState to AnimationState.
// This will store indices into the BitmapTXDataTable.
// It mirrors the structure of the original AnimationPattern[][2].
typedef struct {
    int frame_indices[2]; // Indices into BitmapTXDataTable for two frames
} AnimationState; // Renamed struct

// Declare and initialize the SDL-compatible animation pattern table.
// Renamed from sdl_animation_pattern to AnimationPattern.
// This table maps animation states to sequences of frame indices.
// We use the #define values from oneko.h for the state indices.
// !! VERIFY THESE INDICES AGAINST THE ORIGINAL ONEKO.C AnimationPattern INITIALIZATION !!
// The order of these entries MUST match the integer values of the animation state #defines.
// clang-format off
AnimationState AnimationPattern[] = { // Renamed array
    // State 0: NEKO_STOP (Standing)
    { { TX_MATI2, TX_MATI2 } }, // Uses TX_MATI2 for both ticks
    // State 1: NEKO_JARE (Washing Face) - Assuming this corresponds to TX_JARE2 and TX_KAKI1 based on index
    { { TX_JARE2, TX_KAKI1 } },
    // State 2: NEKO_KAKI (Scratching Head) - Assuming this corresponds to TX_KAKI1 and TX_KAKI2 based on index
    { { TX_KAKI1, TX_KAKI2 } },
    // State 3: NEKO_AKUBI (Yawning) - Assuming this corresponds to TX_MATI3 and TX_AWAKE based on index
    { { TX_MATI3, TX_AWAKE } },
    // State 4: NEKO_SLEEP
    { { TX_SLEEP1, TX_SLEEP2 } }, // Uses TX_SLEEP1 and TX_SLEEP2
    // State 5: NEKO_AWAKE
    { { TX_AWAKE, TX_AWAKE } }, // Uses TX_AWAKE for both ticks
    // State 6: NEKO_U_MOVE (Walking Up)
    { { TX_UP1, TX_UP2 } }, // Uses TX_UP1 and TX_UP2
    // State 7: NEKO_D_MOVE (Walking Down)
    { { TX_DOWN1, TX_DOWN2 } }, // Uses TX_DOWN1 and TX_DOWN2
    // State 8: NEKO_L_MOVE (Walking Left)
    { { TX_LEFT1, TX_LEFT2 } }, // Uses TX_LEFT1 and TX_LEFT2
    // State 9: NEKO_R_MOVE (Walking Right)
    { { TX_RIGHT1, TX_RIGHT2 } }, // Uses TX_RIGHT1 and TX_RIGHT2
    // State 10: NEKO_UL_MOVE (Walking Up-Left)
    { { TX_UPLEFT1, TX_UPLEFT2 } }, // Uses TX_UPLEFT1 and TX_UPLEFT2
    // State 11: NEKO_UR_MOVE (Walking Up-Right)
    { { TX_UPRIGHT1, TX_UPRIGHT2 } }, // Uses TX_UPRIGHT1 and TX_UPRIGHT2
    // State 12: NEKO_DL_MOVE (Walking Down-Left)
    { { TX_DWLEFT1, TX_DWLEFT2 } }, // Uses TX_DWLEFT1 and TX_DWLEFT2
    // State 13: NEKO_DR_MOVE (Walking Down-Right)
    { { TX_DWRIGHT1, TX_DWRIGHT2 } }, // Uses TX_DWRIGHT1 and TX_DWRIGHT2
    // State 14: NEKO_U_TOGI (Scratching Up)
    { { TX_UTOGI1, TX_UTOGI2 } }, // Uses TX_UTOGI1 and TX_UTOGI2
    // State 15: NEKO_D_TOGI (Scratching Down)
    { { TX_DTOGI1, TX_DTOGI2 } }, // Uses TX_DTOGI1 and TX_DTOGI2
    // State 16: NEKO_L_TOGI (Scratching Left)
    { { TX_LTOGI1, TX_LTOGI2 } }, // Uses TX_LTOGI1 and TX_LTOGI2
    // State 17: NEKO_R_TOGI (Scratching Right)
    { { TX_RTOGI1, TX_RTOGI2 } } // Uses TX_RTOGI1 and TX_RTOGI2
    // Add other states if they exist in the original oneko.h/c and map them to frame indices
};
// clang-format on


// The total number of animation states.
#define TOTAL_ANIMATION_STATES (sizeof(AnimationPattern) / sizeof(AnimationState)) // Updated define


// Game state variables (mirroring original oneko)
int NekoX = 100; // Initial X position
int NekoY = 100; // Initial Y position
// Initial state using the #define from oneko.h
int NekoState = NEKO_STOP;
int NekoTickCount = 0; // Animation tick counter
int MouseX = 0; // Mouse X position
int MouseY = 0; // Mouse Y position


// Variables to store the dimensions of the currently selected character's textures (assuming they are uniform)
// Renamed from character_texture_width/height to bitmap_texture_width/height.
// These will be assigned in InitBitmapData.
int bitmap_texture_width = 0;
int bitmap_texture_height = 0;

// Forward declarations for functions in sdl_xbm_helper.c
SDL_Texture* load_xbm_as_texture(SDL_Renderer* renderer, const unsigned char* image_bits, int width, int height, const unsigned char* mask_bits);
SDL_Surface* create_surface_from_xbm(const unsigned char* xbm_data, int width, int height);


// --- X11-specific functions that are not needed in the SDL port ---
// These functions handle X11-specific tasks like resource management,
// window selection, cursor handling, and window checks.
// Their functionality is replaced by SDL APIs.

// Function to get default settings or resources from the X server's resource database.
// Not needed in SDL.
// void NekoGetDefault(void);

// Function for Xlib resource management.
// Not needed in SDL.
// void GetResources(void);

// Function to specify which X window to receive events from.
// Replaced by SDL's event handling tied to the SDL window.
// void SelectWindow(void);

// Function to find an X window by its name.
// Not needed in SDL applications interacting with a single SDL window.
// Window WindowWithName(char *name);

// Function for managing the mouse cursor appearance in X.
// Replaced by SDL's cursor functions (SDL_CreateCursor, SDL_SetCursor, etc.).
// void RestoreCursor(void);

// Function to check if the mouse pointer is over a specific X window.
// In SDL, this is done by checking mouse coordinates relative to the SDL window.
// Bool IsWindowOver(Window w, int x, int y);

// X11 error handler function. Not needed with SDL.
// int XsetErrorHandler(XErrorHandler handler);

// Signal handling function. SDL handles signals differently or they might not be needed for basic operation.
// typedef void (*sighandler_t)(int);
// sighandler_t signal(int signum, sighandler_t handler);

// --- End of X11-specific functions ---


// Forward declarations for SDL port functions
int InitScreen(char *DisplayName, SDL_Window** window, SDL_Renderer** renderer);
void ProcessEvent(SDL_Event* e, int* quit);
void ProcessNeko(SDL_Renderer* renderer);
void DeInitAll(SDL_Window* window, SDL_Renderer* renderer); // Forward declaration for deinitialization function

// Forward declarations for functions to be ported/reimplemented
int Interval(void);
void TickCount(void);
void SetNekoState(int SetValue);
void DrawNeko(SDL_Renderer* renderer, int x, int y, int frame_index);
void RedrawNeko(SDL_Renderer* renderer);
void NekoDirection(void);
int IsNekoDontMove(void);
int IsNekoMoveStart(void);
void CalcDxDy(void);
void NekoThinkDraw(SDL_Renderer* renderer);
int InitBitmapAndTXs(SDL_Renderer* renderer);


// Placeholder for ported/reimplemented functions implementations

// Function to control update timing based on tick_interval
// Returns 1 if it's time for an update, 0 otherwise.
int Interval(void) {
    // This logic will be integrated into NekoThinkDraw's timing check.
    // For now, it's a placeholder.
    return 1; // Always update for now
}

// Function to increment NekoTickCount and handle wrapping
void TickCount(void) {
    // This logic will be integrated into NekoThinkDraw's update section.
    // For now, it's a placeholder.
}

// Function to set the NekoState
void SetNekoState(int SetValue) {
    // This will be implemented to update the global NekoState variable.
    // For now, it's a placeholder.
    NekoState = SetValue; // Simple assignment for now
}

// SDL rendering function
void DrawNeko(SDL_Renderer* renderer, int x, int y, int frame_index) {
    // This will draw the specified frame at the given coordinates using SDL_RenderCopy.
    // For now, it's a placeholder.
     if (frame_index >= 0 && frame_index < TOTAL_BITMAP_FRAMES && BitmapTXDataTable[frame_index].texture != NULL) {
        SDL_Rect dest_rect = { x, y, bitmap_texture_width, bitmap_texture_height };
        SDL_RenderCopy(renderer, BitmapTXDataTable[frame_index].texture, NULL, &dest_rect);
     } else {
         // Handle error or draw a default/blank if frame_index is invalid
         fprintf(stderr, "DrawNeko: Invalid frame index or texture not loaded: %d\n", frame_index);
     }
}

// SDL implementation for clearing and redrawing
void RedrawNeko(SDL_Renderer* renderer) {
    // This will clear the renderer and call DrawNeko with the current state.
    // For now, it's a placeholder.
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF); // Black background
    SDL_RenderClear(renderer);
    // Call DrawNeko with current NekoX, NekoY, and the appropriate frame index
    // This part needs the animation state logic.
    int current_frame_index = 0; // Placeholder
     if (NekoState >= 0 && NekoState < TOTAL_ANIMATION_STATES) {
         if (NekoState != NEKO_SLEEP) {
             current_frame_index = AnimationPattern[NekoState].frame_indices[NekoTickCount & 0x1];
         } else {
             current_frame_index = AnimationPattern[NekoState].frame_indices[(NekoTickCount >> 2) & 0x1];
         }
     }
    DrawNeko(renderer, NekoX, NekoY, current_frame_index);
    SDL_RenderPresent(renderer);
}

// Ported logic for determining direction
void NekoDirection(void) {
    // This will contain the logic to update NekoState based on mouse position relative to NekoX, NekoY.
    // For now, it's a placeholder.
}

// Ported logic for checking if Neko should not move
int IsNekoDontMove(void) {
    // This will contain the logic to check if Neko should stay still (e.g., mouse is close).
    // For now, it's a placeholder.
    return 0; // Always allow movement for now
}

// Ported logic for checking if Neko should start moving
int IsNekoMoveStart(void) {
    // This will contain the logic to check if Neko should start moving (e.g., mouse is far enough).
    // For now, it's a placeholder.
    return 1; // Always allow movement start for now
}

// Ported logic for calculating movement and state
void CalcDxDy(void) {
    // This will contain the core movement and state calculation logic,
    // using MouseX, MouseY, NekoX, NekoY, and updating NekoState.
    // It will also calculate dx and dy. These should probably be global or
    // returned by the function if we strictly follow the original signature.
    // For now, it's a placeholder.
}

// Ported main game logic and drawing function
void NekoThinkDraw(SDL_Renderer* renderer) {
    // This function will call Interval() to check timing,
    // then CalcDxDy(), TickCount(), update NekoX/NekoY,
    // and finally call RedrawNeko().
    // For now, it's a placeholder.

    // Check if it's time to update
    if (Interval()) {
        // Call game logic functions (placeholders for now)
        CalcDxDy();
        TickCount();
        // Update position (based on CalcDxDy results, which are placeholders)
        // NekoX += dx; // dx will come from CalcDxDy
        // NekoY += dy; // dy will come from CalcDxDy

        // Redraw the screen with the updated state
        RedrawNeko(renderer);
    }
}


// Function to initialize bitmap data and create SDL textures for the selected character
// Renamed from InitBitmapData to InitBitmapAndTXs
// Returns 0 on success, -1 on failure
int InitBitmapAndTXs(SDL_Renderer* renderer) {
    // Print statement to indicate start of InitBitmapAndTXs
    printf("Initializing bitmap data and textures...\n");

    // Iterate through the pre-initialized BitmapTXDataTable and create SDL textures.
    // We stop when we encounter the NULL entry.

    for (int i = 0; BitmapTXDataTable[i].image_bits != NULL; ++i) { // Loop until the NULL entry
        // Call your helper function to load XBM data and create an SDL_Texture
        // We use the data already present in the table entry.
        // Add explicit casts to const unsigned char* to resolve pointer-sign warnings
        if (BitmapTXDataTable[i].image_bits != NULL) {
             BitmapTXDataTable[i].texture = load_xbm_as_texture(renderer,
                                                                   (const unsigned char*)BitmapTXDataTable[i].image_bits,
                                                                   BitmapTXDataTable[i].width,
                                                                   BitmapTXDataTable[i].height,
                                                                   (const unsigned char*)BitmapTXDataTable[i].mask_bits);

            if (BitmapTXDataTable[i].texture == NULL) {
                fprintf(stderr, "Failed to create texture for frame %d!\n", i);
                // Clean up textures loaded so far before returning failure
                // Loop up to the current failed index 'i'
                for (int j = 0; j < i; ++j) {
                    if (BitmapTXDataTable[j].texture) {
                        SDL_DestroyTexture(BitmapTXDataTable[j].texture);
                        BitmapTXDataTable[j].texture = NULL;
                    }
                }
                return -1; // Indicate failure
            }
             // Print statement for successful texture loading (optional, can be noisy)
             // printf("Successfully loaded texture for frame %d.\n", i);
        } else {
             // This case should not be reached if the table is correctly terminated with NULL
             fprintf(stderr, "Unexpected NULL image_bits for frame %d during initialization.\n", i);
        }
    }

    // Assign dimensions after the table is populated
    // Check if the table has at least one entry before accessing index 0
    if (BitmapTXDataTable[0].image_bits != NULL) {
        bitmap_texture_width = BitmapTXDataTable[0].width;
        bitmap_texture_height = BitmapTXDataTable[0].height;
        printf("Bitmap texture dimensions: %dx%d\n", bitmap_texture_width, bitmap_texture_height);
    } else {
         bitmap_texture_width = 0;
         bitmap_texture_height = 0;
         fprintf(stderr, "Warning: No character frames loaded.\n");
    }

    // Print statement to indicate end of InitBitmapAndTXs
    printf("Finished initializing bitmap data and textures.\n");

    return 0; // Indicate success
}


// Function to initialize the SDL window and renderer
// Returns 0 on success, -1 on failure
int InitScreen(char *DisplayName, SDL_Window** window, SDL_Renderer** renderer) {
    // Initialize SDL's video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1; // Indicate an error
    }
     printf("SDL initialized successfully.\n");

    // Create the SDL window with initial dimensions.
    *window = SDL_CreateWindow(DisplayName,
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (*window == NULL) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit(); // Clean up SDL before exiting
        return -1;
    }
    printf("SDL window created successfully.\n");

    // Create a hardware accelerated renderer for the window
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (*renderer == NULL) {
        fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(*window); // Clean up window
        SDL_Quit(); // Clean up SDL
        return -1;
    }
     printf("SDL renderer created successfully.\n");

    return 0; // Indicate success
}

// Function to process SDL events
void ProcessEvent(SDL_Event* e, int* quit) {
    // Process all pending events in the event queue
    while (SDL_PollEvent(e) != 0) {
        // If the user closes the window (e.g., clicks the 'X' button)
        if (e->type == SDL_QUIT) {
            *quit = 1; // Set the quit flag to exit the main loop
             printf("Quit event received.\n");
        }
        // --- Handle mouse motion and other events here ---
        if (e->type == SDL_MOUSEMOTION) {
            MouseX = e->motion.x;
            MouseY = e->motion.y;
            // printf("Mouse moved to: %d, %d\n", MouseX, MouseY); // Optional: print mouse movement
        }
        // Add other event types as needed (e.g., mouse clicks, keyboard events)
    }
}

// Function containing the main game loop logic (timing, calling NekoThinkDraw)
void ProcessNeko(SDL_Renderer* renderer) {
    int quit = 0; // Local quit flag
    SDL_Event e;  // Local event structure

    // Game loop timing variables (for consistent animation speed)
    static Uint64 last_tick_time = 0; // Use static to retain value between calls
    Uint64 current_tick_time;
    double delta_time = 0;
    double tick_interval = 1.0 / 12.0; // Assuming 12 ticks per second for animation (adjust as needed)

    // Initialize last_tick_time on the first call
    if (last_tick_time == 0) {
        last_tick_time = SDL_GetPerformanceCounter();
    }

    // --- Main Game Loop ---
    // This loop processes events and calls the main game logic function.

    printf("Entering main game loop within ProcessNeko.\n");

    while (!quit) { // Use local quit flag
        // Process SDL events
        ProcessEvent(&e, &quit); // Pass address of local quit

        // --- Game Logic Update (based on timing) ---
        current_tick_time = SDL_GetPerformanceCounter();
        delta_time = (double)(current_tick_time - last_tick_time) / SDL_GetPerformanceFrequency();
        // Note: delta_time is not accumulated here, as Interval will handle the check.
        last_tick_time = current_tick_time; // Update for the next frame

        // --- Call NekoThinkDraw to handle thinking, animation, and drawing ---
        // NekoThinkDraw will internally use Interval() to decide if it's time to update.
        NekoThinkDraw(renderer);

        // No explicit SDL_Delay here, VSync and the Interval() check in NekoThinkDraw
        // will manage the frame rate.
    }

    printf("Exiting main game loop within ProcessNeko.\n");
}

// Function to deinitialize SDL window, renderer, and quit SDL
void DeInitAll(SDL_Window* window, SDL_Renderer* renderer) {
    printf("Deinitializing SDL window, renderer, and quitting SDL.\n");

    // Cleanup Textures
    // Loop through the table until the NULL entry
    for(int i = 0; BitmapTXDataTable[i].image_bits != NULL; ++i) { // Use new table name
        if (BitmapTXDataTable[i].texture) { // Use new table name
            SDL_DestroyTexture(BitmapTXDataTable[i].texture); // Use new table name
            BitmapTXDataTable[i].texture = NULL; // Use new table name
        }
    }

    // Destroy the renderer
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    // Destroy the window
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }

    SDL_Quit(); // Shut down all SDL subsystems
    printf("SDL shut down successfully.\n");
}


int main(int argc, char* argv[]) {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    // --- Phase 1: SDL Initialization and Window Creation ---
    // Call the new InitScreen function
    if (InitScreen("oneko (SDL)", &window, &renderer) != 0) {
        fprintf(stderr, "Failed to initialize SDL and create window/renderer.\n");
        return 1; // Indicate failure
    }


    // --- Phase 2: Initialize Bitmap Data and Load Textures ---
    // Call the renamed initialization function
    if (InitBitmapAndTXs(renderer) != 0) {
        fprintf(stderr, "Failed to initialize bitmap data and textures.\n");
        DeInitAll(window, renderer);
        return 1; // Indicate failure
    }
    printf("Bitmap data and textures initialized.\n");

    // --- End Initialize Bitmap Data and Load Textures ---

    // --- Main Application Loop ---
    // Call ProcessNeko to run the main game loop
    printf("Entering main application loop (calling ProcessNeko).\n");
    ProcessNeko(renderer);
    printf("ProcessNeko returned. Application loop finished.\n");

    // Deinitialize SDL window, renderer, and quit SDL
    DeInitAll(window, renderer);


    return 0; // Indicate successful execution
}
