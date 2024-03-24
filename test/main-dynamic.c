#include <rlGameCanvas/Core.h>
#include <rlGameCanvas/Definitions.h>
#include <rlGameCanvas/Pixel.h>

#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>

#define WIDTH  256
#define HEIGHT 240
#define LAYER_COUNT 3

bool bPaused = false;
typedef struct
{
	unsigned                iAnimFrame;
	double                  dFrameTime;
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

bool bFullscreenToggled     = false;
bool bRestrictCursorToggled = false;
bool bHideCursorToggled     = false;

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

	const unsigned iPassedFrames = (unsigned)(pDataT->dFrameTime / SECSPERFRAME);
	pDataT->dFrameTime -= iPassedFrames * SECSPERFRAME;
	pDataT->iAnimFrame += iPassedFrames;

	pDataT->iAnimFrame %= FRAMECOUNT;

	if (bFullscreenToggled)
	{
		poConfig->iFlags  ^= RL_GAMECANVAS_CFG_FULLSCREEN;
		bFullscreenToggled = false;
	}
	if (bRestrictCursorToggled)
	{
		poConfig->iFlags ^= RL_GAMECANVAS_CFG_RESTRICT_CURSOR;
		bRestrictCursorToggled = false;
	}
	if (bHideCursorToggled)
	{
		poConfig->iFlags ^= RL_GAMECANVAS_CFG_HIDE_CURSOR;
		bHideCursorToggled = false;
	}

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
		case 'C':
			bHideCursorToggled = true;
			break;

		case 'F':
			bFullscreenToggled = true;
			break;

		case 'R':
			bRestrictCursorToggled = true;
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

		for (unsigned iY = 0; iOffset + iY < HEIGHT; ++iY)
		{
			poLayers[0].ppxData[iLineOffset] = px; // center line

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

					poLayers[0].ppxData[iLineOffset + iOffset] = px;
					poLayers[0].ppxData[iLineOffset - iOffset] = px;
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
	memset(poLayers[1].ppxData, 0, sizeof(rlGameCanvas_Pixel) * iWidth * iHeight);

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
				poLayers[1].ppxData[iY * iWidth + x] = px;
			}
		}


		if ((unsigned)dOffset == 0)
			break;

		dOldOffset = dOffset;
		dOffset *= YFACTOR;
	}



	// if applicable, draw a cursor indicator
	memset(poLayers[2].ppxData, 0, sizeof(rlGameCanvas_Pixel) * iWidth * iHeight);
	if (pDataT->bMouseOnCanvas)
	{
		const rlGameCanvas_UInt iX = pDataT->oMousePos.x;
		const rlGameCanvas_UInt iY = pDataT->oMousePos.y;
		const rlGameCanvas_Pixel px = rlGameCanvas_Color_White;

		if (iX > 0)
			poLayers[2].ppxData[iY * iWidth + iX - 1] = px;
		if (iX > 1)
			poLayers[2].ppxData[iY * iWidth + iX - 2] = px;
		if (iY > 0)
			poLayers[2].ppxData[(iY - 1) * iWidth + iX] = px;
		if (iY > 1)
			poLayers[2].ppxData[(iY - 2) * iWidth + iX] = px;
		if (iWidth - 1 - iX > 0)
			poLayers[2].ppxData[iY * iWidth + iX + 1] = px;
		if (iWidth - 1 - iX > 1)
			poLayers[2].ppxData[iY * iWidth + iX + 2] = px;
		if (iHeight - 1 - iY > 0)
			poLayers[2].ppxData[(iY + 1) * iWidth + iX] = px;
		if (iHeight - 1 - iY > 1)
			poLayers[2].ppxData[(iY + 2) * iWidth + iX] = px;
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

	const rlGameCanvas_LayerMetadata LAYERS[LAYER_COUNT] = { 0 };
	const rlGameCanvas_Mode MODE = { { WIDTH, HEIGHT }, LAYER_COUNT, LAYERS };

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
	sc.iFlags          = RL_GAMECANVAS_SUP_WINDOWED | RL_GAMECANVAS_SUP_FULLSCREEN_ON_MAXIMZE |
		RL_GAMECANVAS_SUP_RESTRICT_CURSOR;
	sc.iModeCount      = 1;
	sc.pcoModes        = &MODE;

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
		"[C]ursor     - Toggle mouse cursor visiblity.\n"
		"[R]estrict   - Toggle mouse cursor restriction.\n"
		"[F]ullscreen - Toggle fullscreen mode.\n"
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
