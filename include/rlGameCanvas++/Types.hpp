#ifndef RLGAMECANVAS_TYPES_CPP
#define RLGAMECANVAS_TYPES_CPP





#include "../rlGameCanvas/Types.h"



#if __cplusplus < 201811L
#define RLGAMECANVAS_NO_CHAR8
#endif

namespace rlGameCanvasLib
{

	// Bool doesn't need a definition in C++ - the built-in bool type can be used
	using UInt         = rlGameCanvas_UInt;
	using Int          = rlGameCanvas_Int;
	using U8Char       = rlGameCanvas_U8Char;
	using PixelInt     = rlGameCanvas_Pixel; // only used for the interface.

	using Resolution = rlGameCanvas_Resolution;

	using Bitmap = rlGameCanvas_Bitmap;

	using CreateStateCallback  = rlGameCanvas_CreateStateCallback;
	using DestroyStateCallback = rlGameCanvas_DestroyStateCallback;
	using CopyStateCallback    = rlGameCanvas_CopyStateCallback;

	using Config = rlGameCanvas_Config;

	using State = rlGameCanvas_State;

	using LayerMetadata = rlGameCanvas_LayerMetadata;
	using LayerData     = rlGameCanvas_LayerData;

	using Mode = rlGameCanvas_Mode;

	using UpdateStateCallback = rlGameCanvas_UpdateStateCallback;
	using DrawStateCallback   = rlGameCanvas_DrawStateCallback;

	using MsgParam       = rlGameCanvas_MsgParam;
	using MsgCallback    = rlGameCanvas_MsgCallback;
	using WinMsgCallback = rlGameCanvas_WinMsgCallback;

	using StartupConfig = rlGameCanvas_StartupConfig;

}

#include "Pixel.hpp"





#endif // RLGAMECANVAS_TYPES_CPP