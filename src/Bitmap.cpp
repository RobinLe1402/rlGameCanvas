#include <rlGameCanvas++/Bitmap.hpp>

#include <algorithm> // std::min
#include <cmath>     // std::round
#include <memory>    // std::unique_ptr



namespace rlGameCanvasLib
{

	namespace
	{

		Pixel PixelLerp(const Pixel &px1, const Pixel &px2, double dOffset)
		{
			Pixel pxResult;

			pxResult.rgba.a = (uint8_t)std::lerp(px1.rgba.a, px2.rgba.a, dOffset);
			pxResult.rgba.r = (uint8_t)std::lerp(px1.rgba.r, px2.rgba.r, dOffset);
			pxResult.rgba.g = (uint8_t)std::lerp(px1.rgba.g, px2.rgba.g, dOffset);
			pxResult.rgba.b = (uint8_t)std::lerp(px1.rgba.b, px2.rgba.b, dOffset);

			return pxResult;
		}

	}



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

	bool ApplyBitmapOverlay_Scaled(
		Bitmap               *poBase,
		const Bitmap         *poOverlay,
		Int                   iOverlayX,
		Int                   iOverlayY,
		UInt                  iOverlayScaledWidth,
		UInt                  iOverlayScaledHeight,
		BitmapOverlayStrategy eOverlayStrategy,
		BitmapScalingStrategy eScalingStrategy
	)
	{
		if (iOverlayScaledWidth == 0 || iOverlayScaledHeight == 0)
			return false;


		// create temporary bitmap
		auto up_pxScaled = std::make_unique<Pixel[]>(
			(size_t)iOverlayScaledWidth * iOverlayScaledHeight
		);
		Bitmap bmpTemp =
		{
			.ppxData = reinterpret_cast<rlGameCanvas_Pixel *>(up_pxScaled.get()),
			.size =
			{
				.x = iOverlayScaledWidth,
				.y = iOverlayScaledHeight
			}
		};

		const auto &src = poOverlay->ppxData;



		// scale to temporary bitmap
		const double dHalfPixelWidth  = 1.0 / (2.0 * poOverlay->size.x);
		const double dHalfPixelHeight = 1.0 / (2.0 * poOverlay->size.y);
		switch (eScalingStrategy)
		{
		case BitmapScalingStrategy::NearestNeighbor:
		{
			auto up_oSamplePixels = std::make_unique<UInt[]>(iOverlayScaledWidth);
			for (size_t iX = 0; iX < iOverlayScaledWidth; ++iX)
			{
				const double dX = (double)iX / iOverlayScaledWidth;
				up_oSamplePixels[iX] =
					(UInt)std::min<double>(
						poOverlay->size.x - 1,
						std::round(dX * poOverlay->size.x + dHalfPixelWidth)
					);
			}

			Pixel *pDest         = up_pxScaled.get();
			for (size_t iY = 0; iY < iOverlayScaledHeight; ++iY)
			{
				const double dY       = (double)iY / iOverlayScaledHeight;
				const UInt   iSampleY =
					(UInt)std::min<double>(
						poOverlay->size.y - 1,
						std::round(dY * poOverlay->size.y + dHalfPixelHeight)
					);
				const size_t iSampleOffset = (size_t)iSampleY * poOverlay->size.x;

				for (size_t iX = 0; iX < iOverlayScaledWidth; ++iX, ++pDest)
				{
					*pDest = src[iSampleOffset + up_oSamplePixels[iX]];
				}
			}
			break;
		}


		case BitmapScalingStrategy::Bilinear:
		{
			const double dScaleX = (double)poOverlay->size.x / iOverlayScaledWidth;
			const double dScaleY = (double)poOverlay->size.y / iOverlayScaledHeight;

			struct HSamplingInfo
			{
				UInt   iIndexLeft, iIndexRight;
				// positive values that sum up to 0.5
				double dOffsetX;
			};

			auto up_oSamplePixels = std::make_unique<HSamplingInfo[]>(iOverlayScaledWidth);
			{
				auto pDest = up_oSamplePixels.get();
				for (size_t iX = 0; iX < iOverlayScaledWidth; ++iX, ++pDest)
				{
					const double dXAbs = dScaleX * iX;

					pDest->iIndexLeft  = UInt(dXAbs);
					pDest->iIndexRight =
						UInt(std::min<double>(poOverlay->size.x - 1, pDest->iIndexLeft + 1));

					pDest->dOffsetX = dXAbs - pDest->iIndexLeft;
				}
			}

			Pixel *pDest = up_pxScaled.get();
			for (size_t iY = 0; iY < iOverlayScaledHeight; ++iY)
			{
				const double dYAbs = dScaleY * iY;

				const UInt iIndexTop    = UInt(dYAbs);
				const UInt iIndexBottom = UInt(std::min<double>(
					poOverlay->size.y - 1,
					iIndexTop + 1
				));

				const double dOffsetY = dYAbs - iIndexTop;

				for (size_t iX = 0; iX < iOverlayScaledWidth; ++iX, ++pDest)
				{
					const auto &si = up_oSamplePixels[iX];

					const Pixel pxSample[4] =
					{
						src[iIndexTop    * poOverlay->size.x + si.iIndexLeft ],
						src[iIndexTop    * poOverlay->size.x + si.iIndexRight],
						src[iIndexBottom * poOverlay->size.x + si.iIndexLeft ],
						src[iIndexBottom * poOverlay->size.x + si.iIndexRight]
					};

					const Pixel pxTop    = PixelLerp(pxSample[0], pxSample[1], si.dOffsetX);
					const Pixel pxBottom = PixelLerp(pxSample[2], pxSample[3], si.dOffsetX);

					*pDest = PixelLerp(pxTop, pxBottom, dOffsetY);
				}
			}
			break;
		}
			
		default:
			return false;
		}



		return ApplyBitmapOverlay(poBase, &bmpTemp, iOverlayX, iOverlayY, eOverlayStrategy);
	}

}
