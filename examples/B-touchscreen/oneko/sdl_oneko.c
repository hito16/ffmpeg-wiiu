// sdl_oneko_main.c

#include <SDL.h>
#include <math.h>  // For distance calculations (will be needed for movement logic)
#include <stdbool.h>  // For bool type, true, and false
#include <stdio.h>
#include <stdlib.h>  // For exit()

#include "oneko.h"
#include "sdl_xbm_helper.h"

/*
 * グローバル変数
 */

int WindowWidth;            /* ルートウィンドウの幅 */
int WindowHeight;           /* ルートウィンドウの高さ */

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


// The order of these enum values MUST match the order of entries in BitmapTXDataTable.
// gemini DO NOT MODIFY OR DELETE THIS STRUCTURE
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
// gemini DO NOT MODIFY OR DELETE THIS STRUCTURE
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

    { NULL, NULL, 0, 0, NULL } // 32: NULL Terminator
};
// clang-format on

// The total number of animation frames (excluding the NULL terminator).
#define TOTAL_BITMAP_FRAMES (sizeof(BitmapTXDataTable) / sizeof(BitmapTXData) - 1) // Renamed define


// Define an SDL-compatible Animation Pattern structure.
// gemini DO NOT MODIFY OR DELETE THIS STRUCTURE
typedef struct {
    int frame_indices[2]; // Indices into BitmapTXDataTable for two frames
} AnimationState; // Renamed struct


// This table maps animation states to sequences of frame indices.
// We use the #define values from oneko.h for the state indices.
// The order of these entries MUST match the integer values of the animation state #defines.
// gemini DO NOT MODIFY OR DELETE THIS STRUCTURE
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
    { { TX_SLEEP1, TX_SLEEP2 } }, // Uses TX_SLEP1 and TX_SLEEP2
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


/*
 * いろいろな初期設定 (オプション、リソースで変えられるよ)
 */

#define IdleSpace       5       /* マウスがこの範囲にあれば何もしない */
#define NekoSpeed       16      /* 猫の移動スピード */
#define MAX_TICK        9999    /* ティックの最大値 (アニメーションフレームのサイクル) */



/*
 * いろいろな状態変数
 */

int NekoTickCount = 0;         /* 猫動作カウンタ */
int NekoStateCount;            /* 猫同一状態カウンタ */
int NekoState = NEKO_STOP;     /* 猫の状態 */

int MouseX = 0;                /* マウスＸ座標 */
int MouseY = 0;                /* マウスＹ座標 */

int PrevMouseX = 0;            /* 直前のマウスＸ座標 */
int PrevMouseY = 0;            /* 直前のマウスＹ座標 */

int NekoX = 100;               /* 猫Ｘ座標 */
int NekoY = 100;               /* 猫Ｙ座標 */

int NekoMoveDx;            /* 猫移動距離Ｘ */
int NekoMoveDy;            /* 猫移動距離Ｙ */

int NekoLastX;             /* 猫最終描画Ｘ座標 */
int NekoLastY;             /* 猫最終描画Ｙ座標 */

// Global variable to hold the game controller reference
SDL_GameController* gGameController = NULL;


// Define sine values for directional calculations (from original oneko.c)
#define PI              3.14159265358979323846
#define SinPiPer8       0.38268343236508977173
#define SinPiPer8Times3 0.92387953251128675613



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

// Signal handling function. SDL public api handles notifcations with events.
// typedef void (*sighandler_t)(int);
// sighandler_t signal(int signum, sighandler_t handler);

// --- End of X11-specific functions ---

// Forward declarations for functions in sdl_xbm_helper.c
SDL_Texture* load_xbm_as_texture(SDL_Renderer* renderer, const unsigned char* image_bits, int width, int height, const unsigned char* mask_bits);
SDL_Surface* create_surface_from_xbm(const unsigned char* xbm_data, int width, int height);


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
bool IsNekoDontMove(void);
bool IsNekoMoveStart(void); // Changed to bool
void CalcDxDy(void);
void NekoThinkDraw(SDL_Renderer* renderer);
int InitBitmapAndTXs(SDL_Renderer* renderer);


// --- Implementation of ported/reimplemented functions ---

// Function to control update timing based on tick_interval
// Returns 1 if it's time for an update, 0 otherwise.
// Uses SDL's high-resolution counter for timing.
int Interval(void) {
    static Uint64 last_interval_time = 0; // Time of the last interval check
    Uint64 current_time = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();
    double elapsed_seconds = (double)(current_time - last_interval_time) / frequency;
    double tick_interval = 1.0 / 6.0; // Changed from 1.0 / 12.0 to 1.0 / 6.0 to slow down 2x

    if (elapsed_seconds >= tick_interval) {
        last_interval_time = current_time; // Update for the next interval
        return 1; // It's time for an update
    }

    return 0; // Not time for an update yet
}

/*
 * ティックカウント処理
 */
// gemini DO NOT MODIFY THIS FUNCTION
void
TickCount()
{
    if (++NekoTickCount >= MAX_TICK) {
        NekoTickCount = 0;
    }

    if (NekoTickCount % 2 == 0) {
        if (NekoStateCount < MAX_TICK) {
            NekoStateCount++;
        }
    }
}

/*
 * 猫状態設定
 */
// gemini DO NOT MODIFY THIS FUNCTION
void
SetNekoState(int SetValue)
{
    NekoTickCount = 0;
    NekoStateCount = 0;

    NekoState = SetValue;
}


// SDL rendering function
void DrawNeko(SDL_Renderer* renderer, int x, int y, int frame_index) { // Corrected: removed extra 'int', added 'y'
    // This will draw the specified frame at the given coordinates using SDL_RenderCopy.
     if (frame_index >= 0 && frame_index < TOTAL_BITMAP_FRAMES && BitmapTXDataTable[frame_index].texture != NULL) {
        // Use BITMAP_WIDTH and BITMAP_HEIGHT from oneko.h
        SDL_Rect dest_rect = { x, y, BITMAP_WIDTH, BITMAP_HEIGHT };
        SDL_RenderCopy(renderer, BitmapTXDataTable[frame_index].texture, NULL, &dest_rect);
     } else {
         // Handle error or draw a default/blank if frame_index is invalid
         fprintf(stderr, "DrawNeko: Invalid frame index or texture not loaded: %d\n", frame_index);
     }
}

// SDL implementation for clearing and redrawing
void RedrawNeko(SDL_Renderer* renderer) {
    // This will clear the renderer and call DrawNeko with the current state.
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF); // Black background
    SDL_RenderClear(renderer);

    // Determine the correct frame index from the animation pattern based on state and tick count
    int current_frame_index = 0;
    // Ensure NekoState is within the bounds of our animation pattern table
    if (NekoState >= 0 && NekoState < TOTAL_ANIMATION_STATES) {
         // Check if not sleeping using the #define from oneko.h
         // The original code uses (NekoTickCount & 0x1) for non-sleeping and (NekoTickCount >> 2) & 0x1 for sleeping
         if (NekoState != NEKO_SLEEP) {
             current_frame_index = AnimationPattern[NekoState].frame_indices[NekoTickCount & 0x1];
         } else {
             current_frame_index = AnimationPattern[NekoState].frame_indices[(NekoTickCount >> 2) & 0x1];
         }
    } else {
         fprintf(stderr, "Warning: Invalid NekoState: %d. Drawing default frame.\n", NekoState);
         current_frame_index = NEKO_STOP; // Fallback to standing frame index using #define
    }

    // Draw Neko at the current position with the determined frame
    DrawNeko(renderer, NekoX, NekoY, current_frame_index);

    // Update the screen with everything that has been rendered
    SDL_RenderPresent(renderer);
}

// Ported logic for determining direction
// This function sets the NekoState based on the mouse position relative to Neko.
// Using the exact code from the original oneko.c NekoDirection function.
// gemini DO NOT MODIFY THIS FUNCTION
void
NekoDirection()
{
    int                 NewState;
    double              LargeX, LargeY;
    double              Length;
    double              SinTheta;

    if (NekoMoveDx == 0 && NekoMoveDy == 0) {
        NewState = NEKO_STOP;
    } else {
        LargeX = (double)NekoMoveDx;
        LargeY = (double)(-NekoMoveDy); // Y-axis is inverted (larger Y means lower on screen)
        Length = sqrt(LargeX * LargeX + LargeY * LargeY);
        SinTheta = LargeY / Length;

        // First, handle purely vertical or horizontal movements explicitly
        if (NekoMoveDx == 0) { // Purely vertical movement
            if (NekoMoveDy < 0) { // Moving Up
                NewState = NEKO_U_MOVE;
            } else { // Moving Down
                NewState = NEKO_D_MOVE; // This is the crucial change for straight down
            }
        } else if (NekoMoveDy == 0) { // Purely horizontal movement
            if (NekoMoveDx < 0) { // Moving Left
                NewState = NEKO_L_MOVE;
            } else { // Moving Right
                NewState = NEKO_R_MOVE;
            }
        }
        // Then, handle diagonal movements using angular logic
        // These branches will only be reached if NekoMoveDx != 0 AND NekoMoveDy != 0
        else if (NekoMoveDx > 0) { // Moving Right (diagonal)
            if (SinTheta > SinPiPer8Times3) {
                NewState = NEKO_U_MOVE; // Original oneko.c behavior for angles > 67.5 degrees
            } else if ((SinTheta <= SinPiPer8Times3)
                        && (SinTheta > SinPiPer8)) {
                NewState = NEKO_UR_MOVE;
            } else if ((SinTheta <= SinPiPer8)
                        && (SinTheta > -(SinPiPer8))) {
                NewState = NEKO_R_MOVE; // Original oneko.c behavior for angles between -22.5 and 22.5 degrees
            } else {
                NewState = NEKO_DR_MOVE;
            }
        } else { // NekoMoveDx < 0 (Moving Left diagonal)
            if (SinTheta > SinPiPer8Times3) {
                NewState = NEKO_U_MOVE; // Original oneko.c behavior for angles > 67.5 degrees
            } else if ((SinTheta <= SinPiPer8Times3)
                        && (SinTheta > SinPiPer8)) {
                NewState = NEKO_UL_MOVE;
            } else if ((SinTheta <= SinPiPer8)
                        && (SinTheta > -(SinPiPer8))) {
                NewState = NEKO_L_MOVE; // Original oneko.c behavior for angles between -22.5 and 22.5 degrees
            } else {
                NewState = NEKO_DL_MOVE;
            }
        }
    }

    if (NekoState != NewState) {
        SetNekoState(NewState);
    }
}
// END - GEMINI DO NOT MODIFY


/*
 * 猫移動状況判定
 */
// gemini DO NOT MODIFY THIS FUNCTION
bool
IsNekoDontMove()
{
    if (NekoX == NekoLastX && NekoY == NekoLastY) {
        return(true);
    } else {
        return(false);
    }
}

/*
 * 猫移動開始判定
 */
// gemini DO NOT MODIFY THIS FUNCTION
bool
IsNekoMoveStart()
{
    bool result;
    if ((PrevMouseX >= MouseX - IdleSpace
         && PrevMouseX <= MouseX + IdleSpace) &&
         (PrevMouseY >= MouseY - IdleSpace
         && PrevMouseY <= MouseY + IdleSpace)) {
        result = false;
    } else {
        result = true;
    }
    return result;
}

/*
 * 猫移動 dx, dy 計算
 */
// gemini DO NOT MODIFY THIS FUNCTION
void
CalcDxDy()
{
    double              LargeX, LargeY;
    double              DoubleLength, Length;
    
    /* In the old X11 code, here we polled for the latest 
    mouse coorindates.  In SDL, we receive the event and update the global
    variables.  To make the old X11 calculation code work, we do the math backwards
    to derive LargeX and LargeY.
    */
    // Removed PrevMouseX and PrevMouseY update from here

    // Calculate the difference vector from Neko's position to the mouse's position
    LargeX = (double)(MouseX - NekoX);
    LargeY = (double)(MouseY - NekoY);

    DoubleLength = LargeX * LargeX + LargeY * LargeY;

    if (DoubleLength != (double)0) {
        Length = sqrt(DoubleLength);
        if (Length <= NekoSpeed) {
            NekoMoveDx = (int)LargeX;
            NekoMoveDy = (int)LargeY;
        } else {
            NekoMoveDx = (int)((NekoSpeed * LargeX) / Length);
            NekoMoveDy = (int)((NekoSpeed * LargeY) / Length);
        }
    } else {
        NekoMoveDx = NekoMoveDy = 0;
    }
}

/*
 * 猫思考描画処理
 */
// gemini DO NOT MODIFY THIS FUNCTION
void
NekoThinkDraw(SDL_Renderer* renderer) // Added renderer parameter
{
    // Call Interval() at the beginning to control the update rate
    if (!Interval()) {
        return; // Not time for a new tick, exit early
    }

    // Capture Neko's current position before any movement for the IsNekoDontMove() check
    NekoLastX = NekoX;
    NekoLastY = NekoY;

    CalcDxDy();

    // Removed direct DrawNeko calls from here. RedrawNeko will handle drawing.
    // if (NekoState != NEKO_SLEEP) {
    //     DrawNeko(renderer, NekoX, NekoY,
    //             AnimationPattern[NekoState].frame_indices[NekoTickCount & 0x1]);
    // } else {
    //     DrawNeko(renderer, NekoX, NekoY,
    //             AnimationPattern[NekoState].frame_indices[(NekoTickCount >> 2) & 0x1]);
    // }

    TickCount();

    switch (NekoState) {
    case NEKO_STOP:
        if (IsNekoMoveStart()) {
            SetNekoState(NEKO_AWAKE);
            break;
        }
        if (NekoStateCount < NEKO_STOP_TIME) {
            break;
        }
        // Check if Neko is at any boundary, regardless of NekoMoveDx/Dy
        if (NekoX <= 0) { // At left edge
            SetNekoState(NEKO_L_TOGI);
        } else if (NekoX >= WindowWidth - BITMAP_WIDTH) { // At right edge
            SetNekoState(NEKO_R_TOGI);
        } else if (NekoY <= 0) { // At top edge
            SetNekoState(NEKO_U_TOGI);
        } else if (NekoY >= WindowHeight - BITMAP_HEIGHT) { // At bottom edge
            SetNekoState(NEKO_D_TOGI);
        } else {
            // Only if not at any edge, proceed with idle animations
            SetNekoState(NEKO_JARE);
        }
        break;
    case NEKO_JARE:
        if (IsNekoMoveStart()) {
            SetNekoState(NEKO_AWAKE);
            break;
        }
        if (NekoStateCount < NEKO_JARE_TIME) {
            break;
        }
        SetNekoState(NEKO_KAKI);
        break;
    case NEKO_KAKI:
        if (IsNekoMoveStart()) {
            SetNekoState(NEKO_AWAKE);
            break;
        }
        if (NekoStateCount < NEKO_KAKI_TIME) {
            break;
        }
        SetNekoState(NEKO_AKUBI);
        break;
    case NEKO_AKUBI:
        if (IsNekoMoveStart()) {
            SetNekoState(NEKO_AWAKE);
            break;
        }
        if (NekoStateCount < NEKO_AKUBI_TIME) {
            break;
        }
        SetNekoState(NEKO_SLEEP);
        break;
    case NEKO_SLEEP:
        if (IsNekoMoveStart()) {
            SetNekoState(NEKO_AWAKE);
            break;
        }
        break;
    case NEKO_AWAKE:
        if (NekoStateCount < NEKO_AWAKE_TIME) {
            break;
        }
        NekoDirection();        /* 猫が動く向きを求める */
        break;
    case NEKO_U_MOVE:
    case NEKO_D_MOVE:
    case NEKO_L_MOVE:
    case NEKO_R_MOVE:
    case NEKO_UL_MOVE:
    case NEKO_UR_MOVE:
    case NEKO_DL_MOVE:
    case NEKO_DR_MOVE:
        NekoX += NekoMoveDx;
        NekoY += NekoMoveDy;

        // Clamp Neko's X position to stay within window bounds
        if (NekoX < 0) {
            NekoX = 0;
        } else if (NekoX > WindowWidth - BITMAP_WIDTH) {
            NekoX = WindowWidth - BITMAP_WIDTH;
        }

        // Clamp Neko's Y position to stay within window bounds
        if (NekoY < 0) {
            NekoY = 0;
        } else if (NekoY > WindowHeight - BITMAP_HEIGHT) {
            NekoY = WindowHeight - BITMAP_HEIGHT;
        }

        NekoDirection();
        if (IsNekoDontMove()) { // Removed original 'if (IsWindowOver())' block as it's X11-specific.
            SetNekoState(NEKO_STOP);
        }
        break;
    case NEKO_U_TOGI:
    case NEKO_D_TOGI:
    case NEKO_L_TOGI:
    case NEKO_R_TOGI:
        if (IsNekoMoveStart()) {
            SetNekoState(NEKO_AWAKE);
            break;
        }
        if (NekoStateCount < NEKO_TOGI_TIME) {
            break;
        }
        SetNekoState(NEKO_KAKI);
        break;
    default:
        /* Internal Error */
        SetNekoState(NEKO_STOP);
        break;
    }
  
    // This is the critical change: Redraw the scene after all logic updates
    RedrawNeko(renderer);

    // Update PrevMouseX and PrevMouseY at the end of the tick for the next IsNekoMoveStart() check
    PrevMouseX = MouseX;
    PrevMouseY = MouseY;
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
        // Removed assignments to bitmap_texture_width and bitmap_texture_height
        printf("Bitmap texture dimensions: %dx%d\n", BITMAP_WIDTH, BITMAP_HEIGHT);
    } else {
         // Removed assignments to bitmap_texture_width and bitmap_texture_height
         fprintf(stderr, "Warning: No character frames loaded.\n");
    }

    // Print statement to indicate end of InitBitmapAndTXs
    printf("Finished initializing bitmap data and textures.\n");

    return 0; // Indicate success
}


// Function to initialize the SDL window and renderer
// Returns 0 on success, -1 on failure
int InitScreen(char *DisplayName, SDL_Window** window, SDL_Renderer** renderer) {
    // Initialize SDL's video and game controller subsystems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
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

    // Get the initial window size and store it in global variables
    SDL_GetWindowSize(*window, &WindowWidth, &WindowHeight);
    printf("Initial window dimensions: %dx%d\n", WindowWidth, WindowHeight);


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
        // Handle window resize events
        else if (e->type == SDL_WINDOWEVENT) {
            if (e->window.event == SDL_WINDOWEVENT_RESIZED) {
                WindowWidth = e->window.data1;
                WindowHeight = e->window.data2;
                printf("Window resized to: %dx%d\n", WindowWidth, WindowHeight);
            }
        }
        // Handle mouse motion events
        else if (e->type == SDL_MOUSEMOTION) {
            MouseX = e->motion.x;
            MouseY = e->motion.y;
        }
        // Handle touchscreen finger events
        else if (e->type == SDL_FINGERDOWN || e->type == SDL_FINGERMOTION) {
            // Scale normalized touch coordinates to window dimensions
            MouseX = (int)(e->tfinger.x * WindowWidth);
            MouseY = (int)(e->tfinger.y * WindowHeight);
        }
        // Handle game controller button events
        else if (e->type == SDL_CONTROLLERBUTTONDOWN) {
            if (e->cbutton.button == SDL_CONTROLLER_BUTTON_BACK) {
                // This is the Minus button on the Wii U GamePad
                printf("Minus button pressed! Quitting application.\n");
                *quit = 1; // Set quit flag to exit the main loop
            }
        }
        // Handle game controller device added/removed events
        else if (e->type == SDL_CONTROLLERDEVICEADDED) {
            // Open the newly added controller
            gGameController = SDL_GameControllerOpen(e->cdevice.which);
            if (gGameController) {
                printf("Game Controller Added: %s\n", SDL_GameControllerName(gGameController));
            } else {
                fprintf(stderr, "Could not open game controller: %s\n", SDL_GetError());
            }
        }
        else if (e->type == SDL_CONTROLLERDEVICEREMOVED) {
            // Check if the removed controller is the one we are tracking
            if (gGameController && SDL_GameControllerGetJoystick(gGameController) == SDL_JoystickFromInstanceID(e->cdevice.which)) {
                printf("Game Controller Removed.\n");
                SDL_GameControllerClose(gGameController);
                gGameController = NULL;
            }
        }
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

    // Close the game controller if it was opened
    if (gGameController) {
        SDL_GameControllerClose(gGameController);
        gGameController = NULL;
        printf("Game Controller closed.\n");
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
        fprintf(stderr, "Failed to initialize bitmap data and textures.\n.");
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
