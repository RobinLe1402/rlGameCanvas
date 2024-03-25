/***************************************************************************************************
  rlGameCanvas BITMAP API
  =======================

  This file contains additional function definitions for handling bitmaps within the rlGameCanvas
  dynamic library.

  (c) 2024 RobinLe
***************************************************************************************************/
#ifndef RLGAMECANVAS_BITMAP_C
#define RLGAMECANVAS_BITMAP_C





#include "ExportSpecs.h"
#include "Types.h"



/*
	BMP = Bitmap

	RL_GAMECANVAS_BMP_OVERLAY_REPLACE
		Replace the pixels of the base bitmap with the pixels of the overlay.
		Fastest overlay strategy.
	RL_GAMECANVAS_BMP_OVERLAY_BLEND
		Mix the pixels of the bitmap and the pixels of the overlay, considering the alpha value.
		Slower overlay strategy.
*/
#define RL_GAMECANVAS_BMP_OVERLAY_REPLACE 1
#define RL_GAMECANVAS_BMP_OVERLAY_BLEND   2



/// <summary>
/// Apply a bitmap overlay onto another bitmap.
/// </summary>
/// <param name="poBase">The "bottom" bitmap the overlay should be applied to.</param>
/// <param name="poOverlay">The "top" bitmap that acts as an overlay.</param>
/// <param name="iOverlayX">The x position of the overlay.</param>
/// <param name="iOverlayY">The y position of the overlay.</param>
/// <param name="iOverlayStrategy">One of the <c>RL_GAMECANVAS_BMP_OVERLAY_[...] values.</param>
/// <returns>Was the overlay successfully applied?</returns>
RLGAMECANVAS_API rlGameCanvas_Bool RLGAMECANVAS_LIB rlGameCanvas_ApplyBitmapOverlay(
	rlGameCanvas_Bitmap       *poBase,
	const rlGameCanvas_Bitmap *poOverlay,
	rlGameCanvas_Int           iOverlayX,
	rlGameCanvas_Int           iOverlayY,
	rlGameCanvas_UInt          iOverlayStrategy
);





/*
	BMP = Bitmap

	RL_GAMECANVAS_BMP_SCALE_NEAREST_NEIGHBOR
		Use nearest neighbor scaling.
		Fastest scaling strategy.
	RL_GAMECANVAS_BMP_SCALE_BILINEAR
		Use bilinear scaling.
		Slower scaling strategy.
*/
#define RL_GAMECANVAS_BMP_SCALE_NEAREST_NEIGHBOR 1
#define RL_GAMECANVAS_BMP_SCALE_BILINEAR         2



/// <summary>
/// Apply a bitmap overlay onto another bitmap with additional scaling.
/// </summary>
/// <param name="poBase">The "bottom" bitmap the overlay should be applied to.</param>
/// <param name="poOverlay">The "top" bitmap that acts as an overlay.</param>
/// <param name="iOverlayX">The x position of the overlay.</param>
/// <param name="iOverlayY">The y position of the overlay.</param>
/// <param name="iOverlayScaledWidth">The width the overlay should be scaled to.</param>
/// <param name="iOverlayScaledHeight">The height the overlay should be scaled to.</param>
/// <param name="iOverlayStrategy">One of the <c>RL_GAMECANVAS_BMP_OVERLAY_[...] values.</param>
/// <param name="iScalingStrategy">One of the <c>RL_GAMECANVAS_BMP_SCALE_[...] values.</param>
/// <returns>Was the overlay successfully applied?</returns>
RLGAMECANVAS_API rlGameCanvas_Bool RLGAMECANVAS_LIB rlGameCanvas_ApplyBitmapOverlay_Scaled(
	rlGameCanvas_Bitmap       *poBase,
	const rlGameCanvas_Bitmap *poOverlay,
	rlGameCanvas_Int           iOverlayX,
	rlGameCanvas_Int           iOverlayY,
	rlGameCanvas_UInt          iOverlayScaledWidth,
	rlGameCanvas_UInt          iOverlayScaledHeight,
	rlGameCanvas_UInt          iOverlayStrategy,
	rlGameCanvas_UInt          iScalingStrategy
);





#endif // RLGAMECANVAS_BITMAP_C