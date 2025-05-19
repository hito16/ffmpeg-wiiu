#ifndef NEKO_IMAGE_STATES_H
#define NEKO_IMAGE_STATES_H

#include "neko_xbm.h"
// Array of XBM data structures from neko_xbm.h
// Define a struct to hold both bitmap and mask data
typedef struct {
    const unsigned char* bits;
    const unsigned char* mask_bits;
    int width;
    int height;
} XbmImageData;

XbmImageData xbm_images[] = {
    {(const unsigned char*)awake_bits, (const unsigned char*)awake_mask_bits,
     awake_width, awake_height},
    {(const unsigned char*)down1_bits, (const unsigned char*)down1_mask_bits,
     down1_width, down1_height},
    {(const unsigned char*)down2_bits, (const unsigned char*)down2_mask_bits,
     down2_width, down2_height},
    {(const unsigned char*)dtogi1_bits, (const unsigned char*)dtogi1_mask_bits,
     dtogi1_width, dtogi1_height},
    {(const unsigned char*)dtogi2_bits, (const unsigned char*)dtogi2_mask_bits,
     dtogi2_width, dtogi2_height},
    {(const unsigned char*)dwleft1_bits,
     (const unsigned char*)dwleft1_mask_bits, dwleft1_width, dwleft1_height},
    {(const unsigned char*)dwleft2_bits,
     (const unsigned char*)dwleft2_mask_bits, dwleft2_width, dwleft2_height},
    {(const unsigned char*)dwright1_bits,
     (const unsigned char*)dwright1_mask_bits, dwright1_width, dwright1_height},
    {(const unsigned char*)dwright2_bits,
     (const unsigned char*)dwright2_mask_bits, dwright2_width, dwright2_height},
    {(const unsigned char*)jare2_bits, (const unsigned char*)jare2_mask_bits,
     jare2_width, jare2_height},
    {(const unsigned char*)kaki1_bits, (const unsigned char*)kaki1_mask_bits,
     kaki1_width, kaki1_height},
    {(const unsigned char*)kaki2_bits, (const unsigned char*)kaki2_mask_bits,
     kaki2_width, kaki2_height},
    {(const unsigned char*)left1_bits, (const unsigned char*)left1_mask_bits,
     left1_width, left1_height},
    {(const unsigned char*)left2_bits, (const unsigned char*)left2_mask_bits,
     left2_width, left2_height},
    {(const unsigned char*)ltogi1_bits, (const unsigned char*)ltogi1_mask_bits,
     ltogi1_width, ltogi1_height},
    {(const unsigned char*)ltogi2_bits, (const unsigned char*)ltogi2_mask_bits,
     ltogi2_width, ltogi2_height},
    {(const unsigned char*)mati2_bits, (const unsigned char*)mati2_mask_bits,
     mati2_width, mati2_height},
    {(const unsigned char*)mati3_bits, (const unsigned char*)mati3_mask_bits,
     mati3_width, mati3_height},
    {(const unsigned char*)right1_bits, (const unsigned char*)right1_mask_bits,
     right1_width, right1_height},
    {(const unsigned char*)right2_bits, (const unsigned char*)right2_mask_bits,
     right2_width, right2_height},
    {(const unsigned char*)rtogi1_bits, (const unsigned char*)rtogi1_mask_bits,
     rtogi1_width, rtogi1_height},
    {(const unsigned char*)rtogi2_bits, (const unsigned char*)rtogi2_mask_bits,
     rtogi2_width, rtogi2_height},
    {(const unsigned char*)sleep1_bits, (const unsigned char*)sleep1_mask_bits,
     sleep1_width, sleep1_height},
    {(const unsigned char*)sleep2_bits, (const unsigned char*)sleep2_mask_bits,
     sleep2_width, sleep2_height},
    {(const unsigned char*)up1_bits, (const unsigned char*)up1_mask_bits,
     up1_width, up1_height},
    {(const unsigned char*)up2_bits, (const unsigned char*)up2_mask_bits,
     up2_width, up2_height},
    {(const unsigned char*)upleft1_bits,
     (const unsigned char*)upleft1_mask_bits, upleft1_width, upleft1_height},
    {(const unsigned char*)upleft2_bits,
     (const unsigned char*)upleft2_mask_bits, upleft2_width, upleft2_height},
    {(const unsigned char*)upright1_bits,
     (const unsigned char*)upright1_mask_bits, upright1_width, upright1_height},
    {(const unsigned char*)upright2_bits,
     (const unsigned char*)upright2_mask_bits, upright2_width, upright2_height},
    {(const unsigned char*)utogi1_bits, (const unsigned char*)utogi1_mask_bits,
     utogi1_width, utogi1_height},
    {(const unsigned char*)utogi2_bits, (const unsigned char*)utogi2_mask_bits,
     utogi2_width, utogi2_height},
};

typedef enum {
    NEKO_STATE_STOP = 0, /* 立ち止まった */
    NEKO_STATE_JARE,     /* 顔を洗っている */
    NEKO_STATE_KAKI,     /* 頭を掻いている */
    NEKO_STATE_AKUBI,    /* あくびをしている */
    NEKO_STATE_SLEEP,    /* 寝てしまった */
    NEKO_STATE_AWAKE,    /* 目が覚めた */
    NEKO_STATE_U_MOVE,   /* 上に移動中 */
    NEKO_STATE_D_MOVE,   /* 下に移動中 */
    NEKO_STATE_L_MOVE,   /* 左に移動中 */
    NEKO_STATE_R_MOVE,   /* 右に移動中 */
    NEKO_STATE_UL_MOVE,  /* 左上に移動中 */
    NEKO_STATE_UR_MOVE,  /* 右上に移動中 */
    NEKO_STATE_DL_MOVE,  /* 左下に移動中 */
    NEKO_STATE_DR_MOVE,  /* 右下に移動中 */
    NEKO_STATE_U_TOGI,   /* 上の壁を引っ掻いている */
    NEKO_STATE_D_TOGI,   /* 下の壁を引っ掻いている */
    NEKO_STATE_L_TOGI,   /* 左の壁を引っ掻いている */
    NEKO_STATE_R_TOGI,   /* 右の壁を引っ掻いている */
    NEKO_STATE_NUM_STATES
} NekoRealStates;

typedef enum {
    NEKO_XBM_AWAKE = 0,
    NEKO_XBM_DOWN1,
    NEKO_XBM_DOWN2,
    NEKO_XBM_DTOGI1,
    NEKO_XBM_DTOGI2,
    NEKO_XBM_DWLEFT1,
    NEKO_XBM_DWLEFT2,
    NEKO_XBM_DWRIGHT1,
    NEKO_XBM_DWRIGHT2,
    NEKO_XBM_JARE2,
    NEKO_XBM_KAKI1,
    NEKO_XBM_KAKI2,
    NEKO_XBM_LEFT1,
    NEKO_XBM_LEFT2,
    NEKO_XBM_LTOGI1,
    NEKO_XBM_LTOGI2,
    NEKO_XBM_MATI2,
    NEKO_XBM_MATI3,
    NEKO_XBM_RIGHT1,
    NEKO_XBM_RIGHT2,
    NEKO_XBM_RTOGI1,
    NEKO_XBM_RTOGI2,
    NEKO_XBM_SLEEP1,
    NEKO_XBM_SLEEP2,
    NEKO_XBM_UP1,
    NEKO_XBM_UP2,
    NEKO_XBM_UPLEFT1,
    NEKO_XBM_UPLEFT2,
    NEKO_XBM_UPRIGHT1,
    NEKO_XBM_UPRIGHT2,
    NEKO_XBM_UTOGI1,
    NEKO_XBM_UTOGI2,
    NEKO_XBM_NUM_IMAGES  // total number of images
} NekoXBMImageDataIndex;

typedef struct {
    NekoRealStates state;
    int image_one;
    int image_two;
} NekoStateAnimations;

NekoStateAnimations neko_animations[] = {
    {NEKO_STATE_AWAKE, NEKO_XBM_AWAKE, -1},
    {NEKO_STATE_JARE, NEKO_XBM_JARE2, -1},
    {NEKO_STATE_D_MOVE, NEKO_XBM_DOWN1, NEKO_XBM_DOWN2},
    {NEKO_STATE_DL_MOVE, NEKO_XBM_DWLEFT1, NEKO_XBM_DWLEFT2},
    {NEKO_STATE_DR_MOVE, NEKO_XBM_DWRIGHT1, NEKO_XBM_DWRIGHT2},
    {NEKO_STATE_L_MOVE, NEKO_XBM_LEFT1, NEKO_XBM_LEFT2},
    {NEKO_STATE_R_MOVE, NEKO_XBM_RIGHT1, NEKO_XBM_RIGHT2},
    {NEKO_STATE_U_MOVE, NEKO_XBM_UP1, NEKO_XBM_UP2},
    {NEKO_STATE_UL_MOVE, NEKO_XBM_UPLEFT1, NEKO_XBM_UPLEFT2},
    {NEKO_STATE_UR_MOVE, NEKO_XBM_UPRIGHT1, NEKO_XBM_UPRIGHT2},
    {NEKO_STATE_KAKI, NEKO_XBM_KAKI1, NEKO_XBM_KAKI2},
    {NEKO_STATE_D_TOGI, NEKO_XBM_DTOGI1, NEKO_XBM_DTOGI2},
    {NEKO_STATE_L_TOGI, NEKO_XBM_LTOGI1, NEKO_XBM_LTOGI2},
    {NEKO_STATE_R_TOGI, NEKO_XBM_RTOGI1, NEKO_XBM_RTOGI2},
    {NEKO_STATE_U_TOGI, NEKO_XBM_UTOGI1, NEKO_XBM_UTOGI2},
    {NEKO_STATE_SLEEP, NEKO_XBM_SLEEP1, NEKO_XBM_SLEEP2},
    {NEKO_STATE_STOP, NEKO_XBM_MATI2, NEKO_XBM_MATI3},
};

#endif  // NEKO_IMAGE_STATES_H
