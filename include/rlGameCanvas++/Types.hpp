#ifndef RLGAMECANVAS_TYPES_CPP
#define RLGAMECANVAS_TYPES_CPP





#include "../rlGameCanvas/Types.h"



#if __cplusplus < 201811L
#define RLGAMECANVAS_NO_CHAR8
#endif

#include "Pixel.hpp"

namespace rlGameCanvasLib
{

	// Bool doesn't need a definition in C++ - the built-in bool type can be used
	using UInt         = rlGameCanvas_UInt;
	using GraphicsData = rlGameCanvas_GraphicsData;
	using U8Char       = rlGameCanvas_U8Char;

	using Layer = rlGameCanvas_Layer;

	using CreateData  = rlGameCanvas_CreateData;
	using DestroyData = rlGameCanvas_DestroyData;
	using CopyData    = rlGameCanvas_CopyData;

	using Resolution = rlGameCanvas_Resolution;

	using WinMsgCallback = rlGameCanvas_WinMsgCallback;
	using DrawCallback   = rlGameCanvas_DrawCallback;

	using ResizeParams = rlGameCanvas_ResizeParams;

	using MsgParam = rlGameCanvas_MsgParam;

	using MsgCallback = rlGameCanvas_MsgCallback;

	using Config = rlGameCanvas_Config;

	using StartupConfig = rlGameCanvas_StartupConfig;

}





#endif // RLGAMECANVAS_TYPES_CPP