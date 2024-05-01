#include <rlGameCanvas/Core.h>
#include <rlGameCanvas/Bitmap.h>
#include <rlGameCanvas/Definitions.h>
#include <rlGameCanvas/Pixel.h>

#include "TestBitmaps.h"

#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>





#define WIDTH  256
#define HEIGHT 240
#define HEIGHT_DEBUG (HEIGHT + 10)

#define MODE_COUNT 2
#define MODEID_DEFAULT 0
#define MODEID_DEBUG   1

#define LAYER_COUNT 5
#define LAYERID_BG     0
#define LAYERID_VLINES 1
#define LAYERID_HLINES 2
#define LAYERID_LOGO   3
#define LAYERID_CURSOR 4

#define LOGO_ANIM_SECONDS 2

bool bPaused = false;
typedef struct
{
	unsigned                iAnimFrame;
	double                  dFrameTime;

	double                  dLogoAnimTime;
	bool                    bLogoAnimFinishing;
	bool                    bLogoAnimFinished;
	double                  dLogoAnimState;

	bool                    bMouseOnCanvas;
	rlGameCanvas_Resolution oMousePos;
} GraphicsData;

#define RESIZETEST 1

#define YFACTOR (0.6)
#define FRAMESPERSEC (30)
#define SECSPERFRAME (1.0 / FRAMESPERSEC)
#define DIST_START (10)
#define DIST_ADD_PER_LINE (0.7)
#define FRAMECOUNT ((HEIGHT / 2) - (unsigned)((HEIGHT / 2) * YFACTOR - 1))

bool bAnimationToggled      = false;
bool bFullscreenToggled     = false;
bool bRestrictCursorToggled = false;
bool bHideCursorToggled     = false;
bool bModeToggled           = false;
bool bCloseRequested        = false;

bool bMouseCursorOnCanvas = false;
rlGameCanvas_Resolution oMouseCursorPos;





void __stdcall Update(
	rlGameCanvas              canvas,
	const rlGameCanvas_State *pcoReadonlyState,
	void                     *pvState,
	double                    dSecsSinceLastCall,
	rlGameCanvas_Config      *poConfig
)
{
	GraphicsData *pDataT = pvState;

	pDataT->dFrameTime += dSecsSinceLastCall;
	if (!pDataT->bLogoAnimFinishing)
	{
		pDataT->dLogoAnimTime += dSecsSinceLastCall;
		pDataT->dLogoAnimState = pDataT->dLogoAnimTime / LOGO_ANIM_SECONDS;

		if (pDataT->dLogoAnimState + 0.01 >= 1.0)
			pDataT->bLogoAnimFinishing = true;
	}
	else
		pDataT->bLogoAnimFinished = true;

	const unsigned iPassedFrames = (unsigned)(pDataT->dFrameTime / SECSPERFRAME);
	pDataT->dFrameTime -= iPassedFrames * SECSPERFRAME;
	pDataT->iAnimFrame += iPassedFrames;

	pDataT->iAnimFrame %= FRAMECOUNT;

	if (bAnimationToggled)
	{
		pDataT->bLogoAnimFinishing = false;
		pDataT->bLogoAnimFinished  = false;
		pDataT->dLogoAnimTime      = 0.0;
		pDataT->dLogoAnimState     = 0.0;

		bAnimationToggled = false;
	}
	if (bFullscreenToggled)
	{
		poConfig->iFlags  ^= RL_GAMECANVAS_CFG_FULLSCREEN;
		bFullscreenToggled = false;
	}
	if (bRestrictCursorToggled)
	{
		poConfig->iFlags      ^= RL_GAMECANVAS_CFG_RESTRICT_CURSOR;
		bRestrictCursorToggled = false;
	}
	if (bHideCursorToggled)
	{
		poConfig->iFlags  ^= RL_GAMECANVAS_CFG_HIDE_CURSOR;
		bHideCursorToggled = false;
	}
	if (bModeToggled)
	{
		poConfig->iMode = ++poConfig->iMode % MODE_COUNT;
		bModeToggled    = false;
	}
	if (bCloseRequested)
		rlGameCanvas_Quit(canvas);

	pDataT->bMouseOnCanvas = pcoReadonlyState->iFlags & RL_GAMECANVAS_STA_MOUSE_ON_CANVAS;
	pDataT->oMousePos      = pcoReadonlyState->oMousePos;
}

void __stdcall CanvasMsg(
	rlGameCanvas          canvas,
	rlGameCanvas_UInt     iMsg,
	rlGameCanvas_MsgParam iParam1,
	rlGameCanvas_MsgParam iParam2
)
{
	switch (iMsg)
	{
	case RL_GAMECANVAS_MSG_CREATE:
		printf("CREATE received\n");
		break;

	case RL_GAMECANVAS_MSG_DESTROY:
		printf("DESTROY received\n");
		break;

	case RL_GAMECANVAS_MSG_LOSEFOCUS:
		printf("LOSEFOCUS received\n");
		break;

	case RL_GAMECANVAS_MSG_GAINFOCUS:
		printf("GAINFOCUS received\n");
		break;
		
	case RL_GAMECANVAS_MSG_MINIMIZE:
		printf("MINIMIZE received\n");
		bPaused = iParam1;
		printf("  %s minimization\n", bPaused ? "Entered" : "Exited");
		break;
	}
}

void __stdcall WinMsg(
	rlGameCanvas canvas,
	HWND         hWnd,
	UINT         uMsg,
	WPARAM       wParam,
	LPARAM       lParam
)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'A':
			bAnimationToggled = true;
			break;

		case 'C':
			bHideCursorToggled = true;
			break;

		case 'F':
			bFullscreenToggled = true;
			break;

		case 'M':
			bModeToggled = true;
			break;

		case 'R':
			bRestrictCursorToggled = true;
			break;

		case 'X':
			bCloseRequested = true;
			break;
		}
		break;
	}
}

void __stdcall Draw(
	rlGameCanvas            canvas,
	const void             *pcvState,
	rlGameCanvas_UInt       iMode,
	rlGameCanvas_Resolution oScreenSize,
	rlGameCanvas_UInt       iLayers,
	rlGameCanvas_LayerData *poLayers,
	rlGameCanvas_Pixel     *ppxBackground,
	rlGameCanvas_UInt       iFlags
)
{
	if (bPaused)
		return;

	const unsigned iWidth  = oScreenSize.x;
	const unsigned iHeight = oScreenSize.y;

	const GraphicsData* pDataT = pcvState;

	const rlGameCanvas_Pixel px = RLGAMECANVAS_MAKEPIXEL_RGB(255, 0, 255);
	if (iFlags & RL_GAMECANVAS_DRW_NEWMODE)
	{
		const unsigned iOffset     = HEIGHT / 2;
		const unsigned iMaxXOffset = WIDTH  / 2;

		double dXOffsetOld = 0;

		const unsigned iLineCenterOffset = WIDTH / 2;
		unsigned iLineOffset = iOffset * iWidth + iLineCenterOffset;


		// draw a background pattern
		if (iMode == MODEID_DEBUG)
		{
			const rlGameCanvas_Pixel px = RLGAMECANVAS_MAKEPIXEL(255, 255, 255, 48);

			for (unsigned iY = 0; iY < poLayers[LAYERID_BG].bmp.size.y; ++iY)
			{
				unsigned iColorIndicator = iY % 2;

				for (unsigned iX = 0; iX < poLayers[LAYERID_BG].bmp.size.x; ++iX)
				{
					if ((iX + iColorIndicator) % 2 == 0)
						poLayers[LAYERID_BG].bmp.ppxData[
							iY * poLayers[LAYERID_BG].bmp.size.x + iX
						] = px;
				}
			}
		}

		for (unsigned iY = 0; iOffset + iY < HEIGHT; ++iY)
		{
			poLayers[LAYERID_VLINES].bmp.ppxData[iLineOffset] = px; // center line

			const double dXOffset = iY * DIST_ADD_PER_LINE;

			unsigned iLine = 1;
			while (1)
			{
				const double dXDist    = iLine * (DIST_START + dXOffset);
				const double dXDistOld = iLine * (DIST_START + dXOffsetOld);

				int bOutOfBounds = 0;
				for (unsigned iOffset = (unsigned)dXDistOld; iOffset <= dXDist; ++iOffset)
				{
					//if (iOffset == dXDistOld && dXDist != dXDistOld)
					//	continue; // avoid line repeats on same column

					if (iLineCenterOffset + iOffset >= WIDTH)
					{
						bOutOfBounds = 1;
						break;
					}

					poLayers[LAYERID_VLINES].bmp.ppxData[iLineOffset + iOffset] = px;
					poLayers[LAYERID_VLINES].bmp.ppxData[iLineOffset - iOffset] = px;
				}

				if (bOutOfBounds)
					break;

				++iLine;
			}

			dXOffsetOld = dXOffset;

			iLineOffset += iWidth;
		}
	}



	// clear the horizontal line layer
	memset(poLayers[LAYERID_HLINES].bmp.ppxData, 0, sizeof(rlGameCanvas_Pixel) * iWidth * iHeight);

	// draw the horizontal lines
	double dOffset = (HEIGHT / 2) - FRAMECOUNT + pDataT->iAnimFrame;
	double dOldOffset = dOffset + 2;
	while (1)
	{
		const unsigned iY = HEIGHT / 2 + (unsigned)dOffset - 1;

		if ((unsigned)dOldOffset != (unsigned)dOffset + 1)
		{
			for (unsigned x = 0; x < WIDTH; ++x)
			{
				poLayers[LAYERID_HLINES].bmp.ppxData[iY * iWidth + x] = px;
			}
		}


		if ((unsigned)dOffset == 0)
			break;

		dOldOffset = dOffset;
		dOffset *= YFACTOR;
	}



	// draw the logo
	if (!pDataT->bLogoAnimFinished)
		memset(poLayers[LAYERID_LOGO].bmp.ppxData, 0,
			sizeof(rlGameCanvas_Pixel) * iWidth * iHeight
		);
	if (!pDataT->bLogoAnimFinished && !pDataT->bLogoAnimFinishing)
	{
		const rlGameCanvas_UInt iLogoSize =
			(rlGameCanvas_UInt)(bmpLOGO_ROBINLE.size.x * pDataT->dLogoAnimState);

		const rlGameCanvas_UInt iX = ( WIDTH       - iLogoSize) / 2;
		const rlGameCanvas_UInt iY = ((HEIGHT / 2) - iLogoSize) / 2;

		rlGameCanvas_ApplyBitmapOverlay_Scaled(
			&poLayers[LAYERID_LOGO].bmp, &bmpLOGO_ROBINLE,
			iX, iY, iLogoSize, iLogoSize,
			RL_GAMECANVAS_BMP_OVERLAY_REPLACE, RL_GAMECANVAS_BMP_SCALE_BILINEAR
		);
	}
	else if ((!pDataT->bLogoAnimFinished && pDataT->bLogoAnimFinishing) ||
		iFlags & RL_GAMECANVAS_DRW_NEWMODE)
	{
		// todo: fix timing! fix thread synchronization!
		rlGameCanvas_ApplyBitmapOverlay(&poLayers[LAYERID_LOGO].bmp, &bmpLOGO_ROBINLE,
			(WIDTH - bmpLOGO_ROBINLE.size.x) / 2, ((HEIGHT / 2) - bmpLOGO_ROBINLE.size.y) / 2,
			RL_GAMECANVAS_BMP_OVERLAY_REPLACE
		);
	}



	// if applicable, draw a cursor indicator
	memset(poLayers[LAYERID_CURSOR].bmp.ppxData, 0, sizeof(rlGameCanvas_Pixel) * iWidth * iHeight);
	if (pDataT->bMouseOnCanvas)
	{
		const rlGameCanvas_UInt iX = pDataT->oMousePos.x;
		const rlGameCanvas_UInt iY = pDataT->oMousePos.y;
		
		rlGameCanvas_ApplyBitmapOverlay(
			&poLayers[LAYERID_CURSOR].bmp, &bmpCURSOR,
			(rlGameCanvas_Int)iX - 1, (rlGameCanvas_Int)iY - 1,
			RL_GAMECANVAS_BMP_OVERLAY_REPLACE
		);
	}
}

void __stdcall CreateData(void** pData)
{
	GraphicsData *pDataT = *pData = malloc(sizeof(GraphicsData));

	if (pDataT)
		memset(pDataT, 0, sizeof(GraphicsData));
}

void __stdcall DestroyData(void* pData)
{
	free(pData);
}

void __stdcall CopyData(const void* pSrc, void* pDest)
{
	memcpy_s(pDest, sizeof(GraphicsData), pSrc, sizeof(GraphicsData));
}

int main(int argc, char* argv[])
{
	rlGameCanvas_StartupConfig sc = { 0 };

	const char szTitle[] = "rlGameCanvas test in C";

	rlGameCanvas_LayerMetadata LAYERS[LAYER_COUNT] = { 0 };
	LAYERS[LAYERID_BG].oLayerSize.x = WIDTH;
	LAYERS[LAYERID_BG].oLayerSize.y = HEIGHT_DEBUG;

	rlGameCanvas_Mode oModes[MODE_COUNT] = { 0 };

	oModes[MODEID_DEFAULT].oScreenSize.x    = WIDTH;
	oModes[MODEID_DEFAULT].oScreenSize.y    = HEIGHT;
	oModes[MODEID_DEFAULT].iLayerCount      = LAYER_COUNT;
	oModes[MODEID_DEFAULT].pcoLayerMetadata = LAYERS;

	oModes[MODEID_DEBUG] = oModes[MODEID_DEFAULT];
	oModes[MODEID_DEBUG].oScreenSize.y = HEIGHT_DEBUG;


	sc.szWindowCaption = szTitle;
	sc.hIconBig        = LoadIconW(GetModuleHandleW(NULL), L"ROBINLE_ICON");
	sc.hIconSmall      = NULL;
	sc.fnUpdateState   = Update;
	sc.fnDrawState     = Draw;
	sc.fnCreateState   = CreateData;
	sc.fnDestroyState  = DestroyData;
	sc.fnCopyState     = CopyData;
	sc.fnOnMsg         = CanvasMsg;
	sc.fnOnWinMsg      = WinMsg;
	sc.iFlags          = RL_GAMECANVAS_SUP_HIDE_CURSOR | RL_GAMECANVAS_SUP_PREFER_PIXELPERFECT;
	sc.iModeCount      = MODE_COUNT;
	sc.pcoModes        = oModes;

	rlGameCanvas canvas = rlGameCanvas_Create(&sc);
	if (!canvas)
	{
		fprintf(stderr, "rlGameCanvas_Create failed.\n");
		return 1;
	}
	else
		printf("rlGameCanvas_Create succeeded.\n");

	printf(
		"==================================================\n"
		"CONTROLS:\n"
		"[A]nimation  - Restart the logo animation.\n"
		"[C]ursor     - Toggle mouse cursor visiblity.\n"
		"[F]ullscreen - Toggle fullscreen.\n"
		"[M]ode       - Toggle debug mode.\n"
		"[R]estrict   - Toggle mouse cursor restriction.\n"
		"E[x]it       - Close the demo.\n"
		"==================================================\n"
	);

	printf("Calling rlGameCanvas_Run...\n");
	if (!rlGameCanvas_Run(canvas))
	{
		fprintf(stderr, "rlGameCanvas_Run failed.\n");
		rlGameCanvas_Destroy(canvas);
		return 1;
	}
	else
		printf("rlGameCanvas_Run succeeded.\n");

	printf("Calling rlGameCanvas_Destroy...\n");
	rlGameCanvas_Destroy(canvas);

	DestroyIcon(sc.hIconBig);
	return 0;
}
