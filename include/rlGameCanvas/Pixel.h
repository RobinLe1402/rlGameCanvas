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

const rlGameCanvas_Pixel rlGameCanvas_Color_Blank = RLGAMECANVAS_MAKEPIXEL(0, 0, 0, 0);
const rlGameCanvas_Pixel rlGameCanvas_Color_Black = RLGAMECANVAS_MAKEPIXEL_RGB(0, 0, 0);
const rlGameCanvas_Pixel rlGameCanvas_Color_White = RLGAMECANVAS_MAKEPIXEL_RGB(255, 255, 255);





#endif // RLGAMECANVAS_PIXEL_C