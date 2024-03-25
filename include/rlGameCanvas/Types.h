#ifndef RLGAMECANVAS_TYPES_C
#define RLGAMECANVAS_TYPES_C





#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

#include <stdint.h>



typedef unsigned rlGameCanvas_Bool;
typedef uint32_t rlGameCanvas_UInt;
typedef  int32_t rlGameCanvas_Int; // TODO: mark sizes > 0x7FFFFFFF invalid

#if defined(__cplusplus) && __cplusplus >= 201811L
typedef char8_t rlGameCanvas_U8Char;
#else
typedef char rlGameCanvas_U8Char;
#endif



/*
	A single pixel.
	Defined by the in-memory representation of RGBA.
	Since Windows is Little Endian, the values we see are ABGR:

	If we have R = 0x12, G = 0x34, B = 0x56 and A = 0x78, the value is...
	  +----+----+----+----+
	  | 12 | 34 | 56 | 78 | in-memory
	  +----+----+----+----+
	  | 78 | 56 | 34 | 12 | to the application
	  +----+----+----+----+
*/
typedef uint32_t rlGameCanvas_Pixel;



typedef struct
{
	rlGameCanvas_UInt x, y;
} rlGameCanvas_Resolution;



typedef struct
{
	rlGameCanvas_Pixel *ppxData;
	rlGameCanvas_Resolution size;
} rlGameCanvas_Bitmap;



typedef struct rlGameCanvas_OpaquePtrStruct
{
	int iUnused;
} *rlGameCanvas;



typedef void (__stdcall *rlGameCanvas_CreateStateCallback)(void **ppvData);
typedef void (__stdcall *rlGameCanvas_DestroyStateCallback)(void *pvData);
typedef void (__stdcall *rlGameCanvas_CopyStateCallback)(const void *pcvSrc, void *pvDest);



/*
	The current "logical" graphics configuration.
	Can be changed via the game settings, no manual resizing by the user.

	iMode
		The index of the canvas mode.
	iFlags
		Some setting flags.
		A combination of the RL_GAMECANVAS_CFG_[...] values.
*/
typedef struct
{
	rlGameCanvas_UInt iMode;
	rlGameCanvas_UInt iFlags;
} rlGameCanvas_Config;



/*
	The current non-changable, internal canvas state.

	oMousePos
		The current mouse position, in pixels.
		Only valid if RL_GAMECANVAS_STA_MOUSE_ON_CANVAS is set in iFlags.
	iFlags
		A combination of the RL_GAMECANVAS_STA_[...] values.
*/
typedef struct
{
	rlGameCanvas_Resolution oMousePos;
	rlGameCanvas_UInt       iFlags;
} rlGameCanvas_State;



/*
	Metadata defining a layer.

	oLayerSize
		The size, in pixels, of the layer.
		Must be at least as big as the screen.
	oScreenPos
		The coordinate of the top- and leftmost pixel visible on screen.
		The layer contents are repeated if out-of-bounds pixels would be visible.
	bVisible
		Should the layer be rendered to the screen?
*/
typedef struct
{
	rlGameCanvas_Resolution oLayerSize;
	rlGameCanvas_Resolution oScreenPos;
	rlGameCanvas_Bool       bHide;
} rlGameCanvas_LayerMetadata;

/*
	Layer data to be used in the Draw callback.

	bmp
		The layer bitmap.
		Changes to the size member variable will be ignored.
	poScreenPos
		The top-left position of the "camera".
	pbVisible
		Should the layer be rendered to the screen?
*/
typedef struct
{
	rlGameCanvas_Bitmap bmp;

	rlGameCanvas_Resolution   *poScreenPos;
	rlGameCanvas_Bool         *pbVisible;
} rlGameCanvas_LayerData;



/*
	Data used to define a canvas mode.

	oScreenSize
		The size, in pixels, of the "camera".
		x and y members cannot be zero.
	iLayerCount
		The count of elements in the array pointed to by pcoLayerMetadata.
		Must be > 0.
	pcoLayerMetadata
		Pointer to an array of layer definitions.
*/
typedef struct
{
	rlGameCanvas_Resolution           oScreenSize;
	rlGameCanvas_UInt                 iLayerCount;
	const rlGameCanvas_LayerMetadata *pcoLayerMetadata;
} rlGameCanvas_Mode;




/*
	A callback for Windows messages.

	canvas
		The canvas calling the callback.
	hWnd
		The handle of the window that received the message.
	uMsg
		The Windows message code.
	wParam
		Generic parameter #1.
	lParam
		Generic parameter #2.
*/
typedef void (__stdcall *rlGameCanvas_WinMsgCallback)(
	rlGameCanvas canvas,
	HWND         hWnd,
	UINT         uMsg,
	WPARAM       wParam,
	LPARAM       lParam
);



/*
	A callback for drawing to the bitmap layers.

	canvas
		The canvas calling the callback.
	pcvState
		The current state of the game.
	iMode
		The index of the current canvas mode.
	oScreenSize
		The size, in pixels, of the screen.
	iLayers
		The count of elements in the poLayers array.
	poLayers
		Array of the layers to be updated.
	ppxBackground
		Pointer to the background color.
		On entry, this is the previous background color.
		On exit, this should be the color to be used in the background for this frame.
	iFlags
		Flags for the drawing routine.
		A combination of the RL_GAMECANVAS_DRW_[...] values.
*/
typedef void(__stdcall *rlGameCanvas_DrawStateCallback)(
	rlGameCanvas            canvas,
	const void             *pcvState,
	rlGameCanvas_UInt       iMode,
	rlGameCanvas_Resolution oScreenSize,
	rlGameCanvas_UInt       iLayers,
	rlGameCanvas_LayerData *poLayers,
	rlGameCanvas_Pixel     *ppxBackground,
	rlGameCanvas_UInt       iFlags
);



/*
	A callback for updating the game state.

	canvas
		The canvas calling the callback.
	pvState
		The previous state of the game; to be updated.
	dSecsSinceLastCall
		The time that passed since the last call to this callback, in seconds.
		0 on the very first call.
	poConfig
		A pointer to select canvas settings.
		On entry, this is the configuration used on the previous frame.
		On exit, this is the configuration to be used for the next frame.
*/
typedef void (__stdcall *rlGameCanvas_UpdateStateCallback)(
	rlGameCanvas              canvas,
	const rlGameCanvas_State *pcoReadonlyState,
	void                     *pvState,
	double                    dSecsSinceLastCall,
	rlGameCanvas_Config      *poConfig
);



typedef uint64_t rlGameCanvas_MsgParam;

/*
	A callback for canvas events.

	canvas
		The canvas calling the callback.
	iMsg
		The ID of the message.
		One of the RL_GAMECANVAS_MSG_[...] values.
	iParam1
		Generic parameter 1.
		Value depends on the message.
	iParam2
		Generic parameter 2.
		Value depends on the message.
*/
typedef void (__stdcall *rlGameCanvas_MsgCallback)(
	rlGameCanvas          canvas,
	rlGameCanvas_UInt     iMsg,
	rlGameCanvas_MsgParam iParam1,
	rlGameCanvas_MsgParam iParam2
);



/*
	The startup configuration of a game canvas.

	szWindowCaption
		The (UTF-8) text that should appear in the titlebar.
		Can be NULL, in which case the caption will show a default text.
	hIconSmall
		The icon to be shown in the titlebar of the window.
		Can be NULL. If hIconBig is set, it will be used. Otherwise, a default icon will be used.
	hIconBig
		The icon to be shown in the taskbar.
		Can be NULL. If hIconSmall is set, it will be used. Otherwise, a default icon will be used.
	fnUpdateState
		Callback function for updating the graphics data (= the code for the main game loop).
		Cannot be NULL.
	fnDrawState
		Callback function that updates the canvas.
		Cannot be NULL.
	fnCreateState
		Function that creates an rlGameCanvas_GameState object.
		Cannot be NULL.
	fnDestroyState
		Function that destroys an rlGameCanvas_GameState object.
		Cannot be NULL.
	fnCopyState
		Function that copies the contents of one rlGameCanvas_GameState object into another one.
		Cannot be NULL.
	fnOnMsg
		Callback function for custom messages.
		Can be NULL.
	fnOnWinMsg
		Callback function for window messages.
		Can be NULL.
	iFlags
		Flags for the canvas.
		A combination of the RL_GAMECANVAS_SUP_[...] values.
	iModeCount
		Size of the array pointed to by poModes.
	pcoModes
		Pointer to an array of mode definitions.
*/
typedef struct
{
	const rlGameCanvas_U8Char        *szWindowCaption;
	HICON                             hIconSmall;
	HICON                             hIconBig;
	rlGameCanvas_UpdateStateCallback  fnUpdateState;
	rlGameCanvas_DrawStateCallback    fnDrawState;
	rlGameCanvas_CreateStateCallback  fnCreateState;
	rlGameCanvas_DestroyStateCallback fnDestroyState;
	rlGameCanvas_CopyStateCallback    fnCopyState;
	rlGameCanvas_MsgCallback          fnOnMsg;
	rlGameCanvas_WinMsgCallback       fnOnWinMsg;
	rlGameCanvas_UInt                 iFlags;
	rlGameCanvas_UInt                 iModeCount;
	const rlGameCanvas_Mode          *pcoModes;
} rlGameCanvas_StartupConfig;





#endif RLGAMECANVAS_TYPES_C