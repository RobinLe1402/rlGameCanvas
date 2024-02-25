#include <rlGameCanvas/Core.h>
#include <rlGameCanvas/Definitions.h>

#include <malloc.h>
#include <stdio.h>

typedef struct
{
	int iUnused; // todo: use graphics data
} GraphicsData;

void Update(
	rlGameCanvas              canvas,
	rlGameCanvas_GraphicsData pData,
	double                    dSecsSinceLastCall
)
{
	// todo: update data
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
	const GraphicsData* pDataTyped = pData;

	// todo: draw something
	MessageBoxA(NULL, "Test", "LOL", MB_SYSTEMMODAL);
}

void CreateData(void** pData)
{
	*pData = malloc(sizeof(GraphicsData));
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
	sc.oInitialConfig.iHeight          = 240;
	sc.oInitialConfig.iWidth           = 256;
	sc.oInitialConfig.iScaling         = 2;
	sc.oInitialConfig.iMaximization    = RL_GAMECANVAS_MAX_NONE;

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
