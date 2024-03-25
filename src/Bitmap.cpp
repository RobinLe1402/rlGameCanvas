#include <rlGameCanvas++/Bitmap.hpp>

#include <algorithm> // std::min

namespace rlGameCanvasLib
{

	bool ApplyBitmapOverlay(
		Bitmap               *poBase,
		const Bitmap         *poOverlay,
		Int                   iOverlayX,
		Int                   iOverlayY,
		BitmapOverlayStrategy eOverlayStrategy
	)
	{
		if (poBase == nullptr || poOverlay == nullptr)
			return false;

		// todo: fix disappearing around border



		// check if visible at all

		if (
			(iOverlayX < 0 && UInt(-iOverlayX) >  poOverlay->size.x) ||
			(iOverlayX > 0 && UInt( iOverlayX) >= poBase   ->size.x) ||
			(iOverlayY < 0 && UInt(-iOverlayY) >  poOverlay->size.y) ||
			(iOverlayY > 0 && UInt( iOverlayY) >= poBase   ->size.y)
		)
			return true;

		const UInt iVisibleLeft = (iOverlayX < 0) ? UInt(-iOverlayX) : 0;
		const UInt iVisibleTop  = (iOverlayY < 0) ? UInt(-iOverlayY) : 0;

		const UInt iVisibleX = UInt(iOverlayX + Int(iVisibleLeft));
		const UInt iVisibleY = UInt(iOverlayY + Int(iVisibleTop));

		UInt iVisibleWidth =
			std::min(poOverlay->size.x - iVisibleLeft, poBase->size.x - iOverlayX);
		UInt iVisibleHeight =
			std::min(poOverlay->size.y - iVisibleTop,  poBase->size.y - iOverlayY);

		switch (eOverlayStrategy)
		{
		case BitmapOverlayStrategy::Replace:
		{
			const size_t iRowDataSize = iVisibleWidth * sizeof(Pixel);
			size_t iOffsetBase    = (size_t)iVisibleY   * poBase   ->size.x + iVisibleX;
			size_t iOffsetOverlay = (size_t)iVisibleTop * poOverlay->size.x + iVisibleLeft;

			for (size_t iY = iVisibleTop; iY < poOverlay->size.y; ++iY)
			{
				if (UInt(iOverlayY + (Int)iY) >= poBase->size.y)
					break;

				memcpy_s(
					poBase->ppxData    + iOffsetBase,    iRowDataSize,
					poOverlay->ppxData + iOffsetOverlay, iRowDataSize
				);

				iOffsetBase    += poBase->size.x;
				iOffsetOverlay += poOverlay->size.x;
			}
			break;
		}

		case BitmapOverlayStrategy::Blend:
		{
			const size_t iIgnoredPixels_Overlay = (size_t)poOverlay->size.x - iVisibleWidth;
			const size_t iIgnoredPixels_Base    = (size_t)poBase   ->size.x - iVisibleWidth;


			const Pixel *pSrc = reinterpret_cast<Pixel *>(poOverlay->ppxData) +
				(iVisibleTop * poOverlay->size.x + iVisibleLeft);
			Pixel *pDest = reinterpret_cast<Pixel *>(poBase->ppxData +
				(iVisibleY   * poBase   ->size.x + iVisibleX));

			for (size_t iY = iVisibleTop; iY < poOverlay->size.y; ++iY)
			{
				if (UInt(iOverlayY + (Int)iY) >= poBase->size.y)
					break;

				for (size_t iX = iVisibleLeft; iX < poOverlay->size.x; ++iX, ++pSrc, ++pDest)
				{
					if (UInt(iOverlayX + (Int)iX) >= poBase->size.x)
						break;

					switch (pSrc->rgba.a)
					{
					case 0: // transparent --> do nothing
						break;

					case 255: // fully opaque --> override
						*pDest = *pSrc;
						break;

					default: // partially transparent --> mix
					{
						const double dVisibility_Top    = pSrc->rgba.a / 255.0;
						const double dVisibility_Bottom = 1.0 - dVisibility_Top;

						pDest->rgba.a =
							uint8_t(std::min(255.0, (double)pDest->rgba.a + pSrc->rgba.a));
						pDest->rgba.r = uint8_t(std::min(255.0,
							dVisibility_Top * pDest->rgba.r + dVisibility_Bottom * pSrc->rgba.r));
						pDest->rgba.g = uint8_t(std::min(255.0,
							dVisibility_Top * pDest->rgba.g + dVisibility_Bottom * pSrc->rgba.g));
						pDest->rgba.b = uint8_t(std::min(255.0,
							dVisibility_Top * pDest->rgba.b + dVisibility_Bottom * pSrc->rgba.b));

						break;
					}
					}
				}

				pSrc  += iIgnoredPixels_Overlay;
				pDest += iIgnoredPixels_Base;
			}
			break;
		}

		default:
			return false;
		}


		return true;
	}

}
