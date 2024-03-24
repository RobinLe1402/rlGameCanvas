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
	using U8Char       = rlGameCanvas_U8Char;

	using CreateStateCallback  = rlGameCanvas_CreateStateCallback;
	using DestroyStateCallback = rlGameCanvas_DestroyStateCallback;
	using CopyStateCallback    = rlGameCanvas_CopyStateCallback;

	using Resolution = rlGameCanvas_Resolution;

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





#endif // RLGAMECANVAS_TYPES_CPP