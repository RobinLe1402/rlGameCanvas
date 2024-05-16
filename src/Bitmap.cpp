#include <rlGameCanvas++/Bitmap.hpp>
#include "private/PrivateTypes.hpp" // Rect

#include <algorithm> // std::min
#include <cmath>     // std::round
#include <memory>    // std::unique_ptr



namespace rlGameCanvasLib
{

	namespace
	{

		// as a C++17 replacement for C++20's std::lerp
		constexpr double lerp(double a, double b, double t) noexcept
		{
			return a * (1.0 - t) + (b * t);
		}

		Pixel PixelLerp(const Pixel &px1, const Pixel &px2, double dOffset)
		{
			Pixel pxResult;

			pxResult.rgba.a = (uint8_t)lerp(px1.rgba.a, px2.rgba.a, dOffset);
			pxResult.rgba.r = (uint8_t)lerp(px1.rgba.r, px2.rgba.r, dOffset);
			pxResult.rgba.g = (uint8_t)lerp(px1.rgba.g, px2.rgba.g, dOffset);
			pxResult.rgba.b = (uint8_t)lerp(px1.rgba.b, px2.rgba.b, dOffset);

			return pxResult;
		}


		bool DeFactoCoords(
			const Resolution &resDest,
			Int iOverlayX, Int iOverlayY, const Resolution &resOverlay,
			UInt &iStartX, UInt &iStartY,       Resolution &resVisible,
			Rect &rectVisible
		)
		{
			// check if too far right and/or down
			if (
				(iOverlayX > 0 && UInt(iOverlayX) >= resDest.x) ||
				(iOverlayY > 0 && UInt(iOverlayY) >= resDest.y)
			)
				return false;


			iStartX = (iOverlayX < 0) ? UInt(-iOverlayX) : 0;
			iStartY = (iOverlayY < 0) ? UInt(-iOverlayY) : 0;

			if (iStartX >= resOverlay.x || iStartY >= resOverlay.y)
				return false; // too far left and/or up


			resVisible =
			{
				/* x */ resOverlay.x - iStartX,
				/* y */ resOverlay.y - iStartY
			};

			const UInt iRightmost  = UInt(iOverlayX + Int(resOverlay.x));
			const UInt iBottommost = UInt(iOverlayY + Int(resOverlay.y));

			if (iRightmost >= resDest.x)
				resVisible.x -= iRightmost  - resDest.x;
			if (iBottommost >= resDest.y)
				resVisible.y -= iBottommost - resDest.y;

			rectVisible.iLeft   = UInt(iOverlayX + Int(iStartX));
			rectVisible.iTop    = UInt(iOverlayY + Int(iStartY));
			rectVisible.iRight  = rectVisible.iLeft + resVisible.x;
			rectVisible.iBottom = rectVisible.iTop  + resVisible.y;

			return true;
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

		UInt       iStartX, iStartY;
		Resolution resVisible;
		Rect       rectVisible;
		if (!DeFactoCoords(poBase->size, iOverlayX, iOverlayY, poOverlay->size,
			iStartX, iStartY, resVisible, rectVisible)
		)
			return true;



		switch (eOverlayStrategy)
		{
		case BitmapOverlayStrategy::Replace:
		{
			const size_t iRowDataSize = resVisible.x * sizeof(Pixel);
			size_t iOffsetBase    = (size_t)rectVisible.iTop * poBase->size.x + rectVisible.iLeft;
			size_t iOffsetOverlay = (size_t)iStartY       * poOverlay->size.x + iStartX;

			for (size_t iY = iStartY; iY < poOverlay->size.y; ++iY)
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
			const size_t iIgnoredPixels_Overlay = (size_t)poOverlay->size.x - resVisible.x;
			const size_t iIgnoredPixels_Base    = (size_t)poBase   ->size.x - resVisible.x;


			const Pixel *pSrc = reinterpret_cast<Pixel *>(poOverlay->ppxData) +
				(iStartY * poOverlay->size.x + iStartX);
			Pixel *pDest = reinterpret_cast<Pixel *>(poBase->ppxData +
				(rectVisible.iTop * poBase->size.x + rectVisible.iLeft));

			for (size_t iY = iStartY; iY < poOverlay->size.y; ++iY)
			{
				if (UInt(iOverlayY + (Int)iY) >= poBase->size.y)
					break;

				for (size_t iX = iStartX; iX < poOverlay->size.x; ++iX, ++pSrc, ++pDest)
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

		// same size --> draw directly
		if (iOverlayScaledWidth == poOverlay->size.x && iOverlayScaledHeight == poOverlay->size.y)
			return ApplyBitmapOverlay(poBase, poOverlay, iOverlayX, iOverlayY, eOverlayStrategy);

		const Resolution resScaled =
		{
			/* x */ iOverlayScaledWidth,
			/* y */ iOverlayScaledHeight
		};
		UInt       iStartX, iStartY;
		Resolution resVisible;
		Rect       rectVisible;
		if (!DeFactoCoords(
			poBase->size, iOverlayX, iOverlayY, resScaled,
			iStartX, iStartY, resVisible, rectVisible
		))
			return true;


		// create temporary bitmap
		auto up_pxScaled = std::make_unique<Pixel[]>(
			(size_t)resVisible.x * resVisible.y
		);
		Bitmap bmpTemp =
		{
			/* ppxData */ reinterpret_cast<rlGameCanvas_Pixel *>(up_pxScaled.get()),
			/* size */
			{
				/* x */ resVisible.x,
				/* y */ resVisible.y
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
			auto up_oSamplePixels = std::make_unique<UInt[]>(resVisible.x);
			for (size_t iX = 0; iX < resVisible.x; ++iX)
			{
				size_t iAbsX = iStartX + iX;
				const double dX = (double)iAbsX / iOverlayScaledWidth;
				up_oSamplePixels[iX] =
					(UInt)std::min<double>(
						poOverlay->size.x - 1,
						std::round(dX * poOverlay->size.x + dHalfPixelWidth)
					);
			}

			Pixel *pDest = up_pxScaled.get();
			for (size_t iY = iStartY; iY < resVisible.y; ++iY)
			{
				const double dY       = (double)iY / iOverlayScaledHeight;
				const UInt   iSampleY =
					(UInt)std::min<double>(
						poOverlay->size.y - 1,
						std::round(dY * poOverlay->size.y + dHalfPixelHeight)
					);
				const size_t iSampleOffset = (size_t)iSampleY * poOverlay->size.x;

				for (size_t iX = 0; iX < resVisible.x; ++iX, ++pDest)
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

			auto up_oSamplePixels = std::make_unique<HSamplingInfo[]>(resVisible.x);
			{
				auto pDest = up_oSamplePixels.get();
				for (size_t iX = 0; iX < resVisible.x; ++iX, ++pDest)
				{
					const double dXAbs = dScaleX * (iStartX + iX);

					pDest->iIndexLeft  = UInt(dXAbs);
					pDest->iIndexRight =
						UInt(std::min<double>(poOverlay->size.x - 1, pDest->iIndexLeft + 1));

					pDest->dOffsetX = dXAbs - pDest->iIndexLeft;
				}
			}

			Pixel *pDest = up_pxScaled.get();
			for (size_t iY = 0; iY < resVisible.y; ++iY)
			{
				const double dYAbs = dScaleY * (iStartY + iY);

				const UInt iIndexTop    = UInt(dYAbs);
				const UInt iIndexBottom = UInt(std::min<double>(
					poOverlay->size.y - 1,
					iIndexTop + 1
				));

				const double dOffsetY = dYAbs - iIndexTop;

				for (size_t iX = 0; iX < resVisible.x; ++iX, ++pDest)
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



		return ApplyBitmapOverlay(poBase, &bmpTemp, rectVisible.iLeft, rectVisible.iTop,
			eOverlayStrategy);
	}

}
