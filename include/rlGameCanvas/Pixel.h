#ifndef RLGAMECANVAS_PIXEL_C
#define RLGAMECANVAS_PIXEL_C





#include "Types.h"



#define RLGAMECANVAS_MAKEPIXEL(r, g, b, a) \
( \
	(rlGameCanvas_Pixel)r << 24 | \
	(rlGameCanvas_Pixel)g << 16 | \
	(rlGameCanvas_Pixel)b << 8  | \
	(rlGameCanvas_Pixel)a << 0    \
)





#endif // RLGAMECANVAS_PIXEL_C