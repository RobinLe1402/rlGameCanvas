#include <rlGameCanvas/Core.h>
#include <rlGameCanvas/Definitions.h>
#include <rlGameCanvas/Pixel.h>

#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>

#define WIDTH  256
#define HEIGHT 240

bool bPaused = false;
typedef struct
{
	unsigned iAnimFrame;
	double   dFrameTime;
	bool     bInitialize;
} GraphicsData;

#define YFACTOR (0.6)
#define FRAMESPERSEC (30)
#define SECSPERFRAME (1.0 / FRAMESPERSEC)
#define DIST_START (10)
#define DIST_ADD_PER_LINE (0.7)
#define FRAMECOUNT ((HEIGHT / 2) - (unsigned)((HEIGHT / 2) * YFACTOR - 1))

bool bFullscreenToggled = false;
rlGameCanvas_UInt iPrevMaximization;

void __stdcall Update(
	rlGameCanvas              canvas,
	rlGameCanvas_GraphicsData pData,
	double                    dSecsSinceLastCall,
	rlGameCanvas_Config      *pConfig,
	rlGameCanvas_UInt         iFlags
)
{
	GraphicsData *pDataT = pData;

	pDataT->dFrameTime += dSecsSinceLastCall;

	const unsigned iPassedFrames = (unsigned)(pDataT->dFrameTime / SECSPERFRAME);
	pDataT->dFrameTime -= iPassedFrames * SECSPERFRAME;
	pDataT->iAnimFrame += iPassedFrames;

	pDataT->iAnimFrame %= FRAMECOUNT;

	pDataT->bInitialize = iFlags & RL_GAMECANVAS_UPD_REPAINT;

	if (bFullscreenToggled && (iFlags & RL_GAMECANVAS_UPD_READONLYCONFIG) == 0)
	{
		if (pConfig->iMaximization == RL_GAMECANVAS_MAX_FULLSCREEN)
			pConfig->iMaximization = iPrevMaximization;
		else
		{
			iPrevMaximization = pConfig->iMaximization;
			pConfig->iMaximization = RL_GAMECANVAS_MAX_FULLSCREEN;
		}
		bFullscreenToggled = false;
	}
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

	case RL_GAMECANVAS_MSG_RESIZE:
	{
		printf("RESIZE received\n");
		const rlGameCanvas_ResizeInputParams *rip =
			(const rlGameCanvas_ResizeInputParams*)(iParam1);
		rlGameCanvas_ResizeOutputParams* rop =
			(rlGameCanvas_ResizeOutputParams*)(iParam2);

		printf("  Old client size: [%ux%u], new client size: [%ux%u]\n",
			rip->oOldClientSize.x, rip->oOldClientSize.y,
			rip->oNewClientSize.x, rip->oNewClientSize.y
		);

		
		/*
		// test code for resizing
		if (rip->iNewMaximization != RL_GAMECANVAS_MAX_NONE &&
			rip->oOldClientSize.x < 1000 && rip->oNewClientSize.x >= 1000)
			rop->oResolution.x = 2 * WIDTH;
		else if (rip->iNewMaximization == RL_GAMECANVAS_MAX_NONE)
			rop->oResolution.x = WIDTH;
		*/
		

		break;
	}
		
	case RL_GAMECANVAS_MSG_MINIMIZE:
		printf("MINIMIZE received\n");
		bPaused = iParam1;
		printf("  %s minimization\n", bPaused ? "Entered" : "Exited");
		break;
	}
}

void __stdcall WinMsg(
	rlGameCanvas canvas,
	UINT         uMsg,
	WPARAM       wParam,
	LPARAM       lParam
)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (wParam == 'F')
			bFullscreenToggled = !bFullscreenToggled;
		break;
	}
}

void __stdcall Draw(
	rlGameCanvas canvas,
	rlGameCanvas_Resolution oCanvasSize,
	rlGameCanvas_Layer* pLayers,
	rlGameCanvas_UInt iLayers,
	const rlGameCanvas_GraphicsData pData)
{
	if (bPaused)
		return;

	const unsigned iWidth  = oCanvasSize.x;
	const unsigned iHeight = oCanvasSize.y;

	const GraphicsData* pDataT = pData;

	const rlGameCanvas_Pixel px = RLGAMECANVAS_MAKEPIXEL_RGB(255, 0, 255);
	if (pDataT->bInitialize)
	{
		const unsigned iOffset = HEIGHT / 2;
		const unsigned iMaxXOffset = WIDTH / 2;

		double dXOffsetOld = 0;

		const unsigned iLineCenterOffset = WIDTH / 2;
		unsigned iLineOffset = iOffset * iWidth + iLineCenterOffset;

		for (unsigned iY = 0; iOffset + iY < HEIGHT; ++iY)
		{
			pLayers[0].pData[iLineOffset] = px; // center line

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

					pLayers[0].pData[iLineOffset + iOffset] = px;
					pLayers[0].pData[iLineOffset - iOffset] = px;
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
	memset(pLayers[1].pData, 0, sizeof(rlGameCanvas_Pixel) * iWidth * iHeight);

	double dOffset = (HEIGHT / 2) - FRAMECOUNT + pDataT->iAnimFrame;
	double dOldOffset = dOffset + 2;
	while (1)
	{
		const unsigned iY = HEIGHT / 2 + (unsigned)dOffset - 1;

		if ((unsigned)dOldOffset != (unsigned)dOffset + 1)
		{
			for (unsigned x = 0; x < WIDTH; ++x)
			{
				pLayers[1].pData[iY * iWidth + x] = px;
			}
		}


		if ((unsigned)dOffset == 0)
			break;

		dOldOffset = dOffset;
		dOffset *= YFACTOR;
	}
}

void __stdcall CreateData(void** pData)
{
	GraphicsData *pDataT = *pData = malloc(sizeof(GraphicsData));

	if (pDataT)
	{
		pDataT->iAnimFrame = 0;
		pDataT->dFrameTime = 0.0;
	}
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

	sc.szWindowCaption    = szTitle;
	sc.hIconBig           = LoadIconW(GetModuleHandleW(NULL), L"ROBINLE_ICON");
	sc.hIconSmall         = NULL;
	sc.iMaximizeBtnAction = RL_GAMECANVAS_MAX_MAXIMIZE;
	sc.fnOnMsg            = CanvasMsg;
	sc.fnOnWinMsg         = WinMsg;
	sc.fnUpdate           = Update;
	sc.fnDraw             = Draw;
	sc.fnCreateData       = CreateData;
	sc.fnDestroyData      = DestroyData;
	sc.fnCopyData         = CopyData;
	sc.iExtraLayerCount   = 1;
	sc.bOversample        = 1;
	sc.oInitialConfig.oResolution.x     = WIDTH;
	sc.oInitialConfig.oResolution.y     = HEIGHT;
	sc.oInitialConfig.iPixelSize        = 2;
	sc.oInitialConfig.iMaximization     = RL_GAMECANVAS_MAX_NONE;
	sc.oInitialConfig.pxBackgroundColor = rlGameCanvas_Color_Black;

	rlGameCanvas canvas = rlGameCanvas_Create(&sc);
	if (!canvas)
	{
		fprintf(stderr, "rlGameCanvas_Create failed.\n");
		return 1;
	}
	else
		printf("rlGameCanvas_Create succeeded.\n");

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
