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



	enum class BitmapScalingStrategy
	{
		NearestNeighbor,
		Bilinear
	};

	bool ApplyBitmapOverlay_Scaled(
		Bitmap               *poBase,
		const Bitmap         *poOverlay,
		Int                   iOverlayX,
		Int                   iOverlayY,
		UInt                  iOverlayScaledWidth,
		UInt                  iOverlayScaledHeight,
		BitmapOverlayStrategy eOverlayStrategy,
		BitmapScalingStrategy eScalingStrategy
	);

}





#endif // RLGAMECANVAS_BITMAP_CPP