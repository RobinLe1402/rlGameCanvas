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
	rlGameCanvas_Bool         bConfigChangable
)
{
	GraphicsData *pDataT = pData;

	pDataT->dFrameTime += dSecsSinceLastCall;

	const unsigned iPassedFrames = (unsigned)(pDataT->dFrameTime / SECSPERFRAME);
	pDataT->dFrameTime -= iPassedFrames * SECSPERFRAME;
	pDataT->iAnimFrame += iPassedFrames;

	pDataT->iAnimFrame %= FRAMECOUNT;

	if (bFullscreenToggled && bConfigChangable)
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

		printf("  Old canvas size: [%ux%u], new canvas size: [%ux%u]\n",
			rip->oOldRes.x, rip->oOldRes.y, rip->oNewRes.x, rip->oNewRes.y);

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
	rlGameCanvas_Layer* pLayers,
	rlGameCanvas_UInt iLayers,
	const rlGameCanvas_GraphicsData pData)
{
	if (bPaused)
		return;

	const GraphicsData* pDataT = pData;

	static bool bInit = false;

	const rlGameCanvas_Pixel px = RLGAMECANVAS_MAKEPIXEL_RGB(255, 0, 255);
	if (!bInit)
	{
		const unsigned iOffset = HEIGHT / 2;
		const unsigned iMaxXOffset = WIDTH / 2;

		double dXOffsetOld = 0;

		const unsigned iLineCenterOffset = WIDTH / 2;
		unsigned iLineOffset = iOffset * WIDTH + iLineCenterOffset;

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

			iLineOffset += WIDTH;
		}

		bInit = true;
	}

	memset(pLayers[1].pData, 0, sizeof(rlGameCanvas_Pixel) * WIDTH * HEIGHT);

	double dOffset = (HEIGHT / 2) - FRAMECOUNT + pDataT->iAnimFrame;
	double dOldOffset = dOffset + 2;
	while (1)
	{
		const unsigned iY = HEIGHT / 2 + (unsigned)dOffset - 1;

		if ((unsigned)dOldOffset != (unsigned)dOffset + 1)
		{
			for (unsigned x = 0; x < WIDTH; ++x)
			{
				pLayers[1].pData[iY * WIDTH + x] = px;
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
	sc.oInitialConfig.iMaximization     = RL_GAMECANVAS_MAX_MAXIMIZE;
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
	return 0;
}
