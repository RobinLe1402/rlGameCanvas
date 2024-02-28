#include <rlGameCanvas/Core.h>
#include <rlGameCanvas/Definitions.h>
#include <rlGameCanvas/Pixel.h>

#include <malloc.h>
#include <stdio.h>

#define WIDTH  256
#define HEIGHT 240

typedef struct
{
	unsigned iAnimFrame;
	double   dFrameTime;
} GraphicsData;

const double dSecsPerFrame = 1.0 / 15;
const unsigned iFrameCount = 2;

void Update(
	rlGameCanvas              canvas,
	rlGameCanvas_GraphicsData pData,
	double                    dSecsSinceLastCall
)
{
	GraphicsData *pDataT = pData;

	pDataT->dFrameTime += dSecsSinceLastCall;

	const unsigned iPassedFrames = pDataT->dFrameTime / dSecsPerFrame;
	pDataT->dFrameTime -= iPassedFrames * dSecsPerFrame;
	pDataT->iAnimFrame += iPassedFrames;

	pDataT->iAnimFrame %= iFrameCount;
}

void CanvasMsg(
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
	}
}

void Draw(
	rlGameCanvas canvas,
	rlGameCanvas_Layer* pLayers,
	rlGameCanvas_UInt iLayers,
	const rlGameCanvas_GraphicsData pData)
{
	const GraphicsData* pDataT = pData;

	//Sleep(10);

	const rlGameCanvas_Pixel pxRed = RLGAMECANVAS_MAKEPIXEL_RGB(255, 0, 0);

	rlGameCanvas_Pixel pxOverlay;
	if (pDataT->iAnimFrame == 0)
		pxOverlay = rlGameCanvas_Color_Blank;
	else
		pxOverlay = rlGameCanvas_Color_Black;
	pLayers[1].pData[0] = pxOverlay;

	for (size_t x = 1; x < WIDTH - 1; ++x)
	{
		pLayers[0].pData[x] = pxRed;
	}
	for (size_t y = 0; y < HEIGHT; ++y)
	{
		pLayers[0].pData[ y      * WIDTH    ] = pxRed;
		pLayers[0].pData[(y + 1) * WIDTH - 1] = pxRed;
	}
	for (size_t x = 1; x < WIDTH - 1; ++x)
	{
		pLayers[0].pData[(HEIGHT - 1) * WIDTH + x] = RLGAMECANVAS_MAKEPIXEL_RGB(200, 0, 255);
	}

	// todo: draw something
}

void CreateData(void** pData)
{
	GraphicsData *pDataT = *pData = malloc(sizeof(GraphicsData));

	if (pDataT)
	{
		pDataT->iAnimFrame = 0;
		pDataT->dFrameTime = 0.0;
	}
}

void DestroyData(void* pData)
{
	free(pData);
}

void CopyData(const void* pSrc, void* pDest)
{
	memcpy_s(pDest, sizeof(GraphicsData), pSrc, sizeof(GraphicsData));
}

int main(int argc, char* argv[])
{
	rlGameCanvas_StartupConfig sc = { 0 };

	const char szTitle[] = "rlGameCanvas test in C";

	sc.szWindowCaption    = szTitle;
	sc.iMaximizeBtnAction = RL_GAMECANVAS_MAX_NONE;
	sc.fnOnMsg            = CanvasMsg;
	sc.fnOnWinMsg         = 0;
	sc.fnUpdate           = Update;
	sc.fnDraw             = Draw;
	sc.fnCreateData       = CreateData;
	sc.fnDestroyData      = DestroyData;
	sc.fnCopyData         = CopyData;
	sc.iExtraLayerCount   = 1;
	sc.oInitialConfig.oResolution.x     = WIDTH;
	sc.oInitialConfig.oResolution.y     = HEIGHT;
	sc.oInitialConfig.oPixelSize.x      = 2;
	sc.oInitialConfig.iMaximization     = RL_GAMECANVAS_MAX_NONE;
	sc.oInitialConfig.pxBackgroundColor = rlGameCanvas_Color_White;

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
