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

#define YFACTOR (0.6)
#define FRAMESPERSEC (30)
#define SECSPERFRAME (1.0 / FRAMESPERSEC)
#define DIST_START (10)
#define DIST_ADD_PER_LINE (0.7)
#define FRAMECOUNT ((HEIGHT / 2) - (unsigned)((HEIGHT / 2) * YFACTOR - 1))

void Update(
	rlGameCanvas              canvas,
	rlGameCanvas_GraphicsData pData,
	double                    dSecsSinceLastCall
)
{
	GraphicsData *pDataT = pData;

	pDataT->dFrameTime += dSecsSinceLastCall;

	const unsigned iPassedFrames = (unsigned)(pDataT->dFrameTime / SECSPERFRAME);
	pDataT->dFrameTime -= iPassedFrames * SECSPERFRAME;
	pDataT->iAnimFrame += iPassedFrames;

	pDataT->iAnimFrame %= FRAMECOUNT;
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

	static int bInit = 0;

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

	//Sleep(10);

	const rlGameCanvas_Pixel pxRed = RLGAMECANVAS_MAKEPIXEL_RGB(255, 0, 0);

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
		pLayers[0].pData[(size_t)(HEIGHT - 1) * WIDTH + x] = pxRed;
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
