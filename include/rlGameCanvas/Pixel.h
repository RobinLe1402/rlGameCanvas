/***************************************************************************************************
  rlGameCanvas PIXEL DEFINITIONS
  ==============================

  This file contains pixel-related macros and constants for the rlGameCanvas library.

  (c) 2024 RobinLe
***************************************************************************************************/
#ifndef RLGAMECANVAS_PIXEL_C
#define RLGAMECANVAS_PIXEL_C





#include "Types.h"



#define RLGAMECANVAS_MAKEPIXEL(r, g, b, a) \
( \
	(rlGameCanvas_Pixel)r << 0  | \
	(rlGameCanvas_Pixel)g << 8  | \
	(rlGameCanvas_Pixel)b << 16 | \
	(rlGameCanvas_Pixel)a << 24   \
)

#define RLGAMECANVAS_MAKEPIXEL_RGB(r, g, b) RLGAMECANVAS_MAKEPIXEL(r, g, b, 255)

#define RLGAMECANVAS_MAKEPIXELOPAQUE(px) (px | 0xFF000000)

#ifndef __cplusplus
#define RL_CONST const
#else
#define RL_CONST constexpr
#endif

RL_CONST rlGameCanvas_Pixel rlGameCanvas_Color_Blank = RLGAMECANVAS_MAKEPIXEL(0, 0, 0, 0);
RL_CONST rlGameCanvas_Pixel rlGameCanvas_Color_Black = RLGAMECANVAS_MAKEPIXEL_RGB(0, 0, 0);
RL_CONST rlGameCanvas_Pixel rlGameCanvas_Color_White = RLGAMECANVAS_MAKEPIXEL_RGB(255, 255, 255);

#undef RL_CONST





#endif // RLGAMECANVAS_PIXEL_C