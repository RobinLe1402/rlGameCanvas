#ifndef RLGAMECANVAS_BITMAP_CPP
#define RLGAMECANVAS_BITMAP_CPP





#include "Types.hpp"



namespace rlGameCanvasLib
{

	enum class BitmapOverlayStrategy
	{
		Replace,
		Blend
	};

	bool ApplyBitmapOverlay(
		Bitmap               *poBase,
		const Bitmap         *poOverlay,
		Int                   iOverlayX,
		Int                   iOverlayY,
		BitmapOverlayStrategy eOverlayStrategy
	);

}





#endif // RLGAMECANVAS_BITMAP_CPP