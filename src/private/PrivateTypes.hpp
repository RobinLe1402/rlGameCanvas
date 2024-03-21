#ifndef RLGAMECANVAS_GAMECANVAS_PRIVATETYPES
#define RLGAMECANVAS_GAMECANVAS_PRIVATETYPES





#include <vector>



namespace rlGameCanvasLib
{

	struct Rect
	{
		UInt iLeft;
		UInt iTop;
		UInt iRight;
		UInt iBottom;
	};

	struct Mode_CPP
	{
		rlGameCanvas_Resolution    oScreenSize = {};
		std::vector<LayerMetadata> oLayerMetadata;
	};

}





#endif // RLGAMECANVAS_GAMECANVAS_PRIVATETYPES