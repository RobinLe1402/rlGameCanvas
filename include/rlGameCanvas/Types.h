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
typedef void*    rlGameCanvas_GraphicsData; // the data needed to draw the current frame.



typedef uint32_t rlGameCanvas_Pixel;



typedef struct
{
	rlGameCanvas_UInt   iWidth, iHeight; /* the dimensions, in pixel, of the layer.
	                                        modifications of these values will be ignored.          */
	rlGameCanvas_Pixel *pData;           /* the pixel data - left to right, top to bottom.
	                                        value at position (x,y) can be found at
	                                         index [y * width + x].                                 */
} rlGameCanvas_Layer;



typedef struct rlGameCanvas_OpaquePtrStruct
{
	int iUnused;
} *rlGameCanvas;



typedef void (*rlGameCanvas_CreateData )(rlGameCanvas_GraphicsData pData);
typedef void (*rlGameCanvas_DestroyData)(rlGameCanvas_GraphicsData pData);

typedef struct
{
	rlGameCanvas_UInt x, y;
} rlGameCanvas_Resolution;




typedef void (*rlGameCanvas_WinMsgCallback)(
	rlGameCanvas canvas, // the canvas that received the message.
	UINT         uMsg,   // the message code, see WM_[...].
	WPARAM       wParam, // generic parameter #1.
	LPARAM       lParam  // generic parameter #2.
);



typedef void (*rlGameCanvas_DrawCallback)(
	      rlGameCanvas               canvas,  // the canvas to be modified.
	      rlGameCanvas_Layer        *pLayers, // array of the layers to be updated.
	      rlGameCanvas_UInt          iLayers, // count of elements in the pLayers array.
	const rlGameCanvas_GraphicsData *pData    // the data to be used for updating the canvas.
);



typedef struct
{
	rlGameCanvas_Bool bFullscreenMode;    /* zero    = Fullscreen/maximized mode was enabled.
	                                         nonzero = Windowed mode was enabled.                   */
	rlGameCanvas_Resolution  oOldRes;     /* the old size, in pixels, of the client area.           */
	rlGameCanvas_Resolution  oNewRes;     /* the new size, in pixels, of the client area.           */
	rlGameCanvas_Resolution *pLayerSizes; /* the sizes, in pixels, of the layers.                   */
	rlGameCanvas_UInt        iLayerCount; /* the size of the pLayerSizes array.                     */
} rlGameCanvas_ResizeParams;



typedef uint64_t rlGameCanvas_MsgParam;

typedef void (*rlGameCanvas_MsgCallback)(
	rlGameCanvas          canvas,
	rlGameCanvas_UInt     iMsg,
	rlGameCanvas_MsgParam iParam1,
	rlGameCanvas_MsgParam iParam2
);



/*
	The current graphics configuration.
	Can be changed via the game settings, no manual resizing by the user.
*/
typedef struct
{
	rlGameCanvas_UInt iWidth, iHeight; // size, in pixels, of the canvas.
	rlGameCanvas_UInt iScaling;        // the factor for up-"scaling". 0 = maximum.
} rlGameCanvas_Config;



typedef struct
{
	char *szWindowCaption;                   /* the (UTF-8) text that should appear in the titlebar.
	                                            can be NULL.                                        */
	HICON hIconSmall, hIconBig;              /* the icons for the titlebar and taskbar.
	                                            can be NULL.                                        */
	rlGameCanvas_UInt iMaximizeBtnAction;    /* behaviour when user clicks the maximize button.
	                                            one of the RL_GAMECANVAS_MAX_[...] values.          */
	rlGameCanvas_UInt iInitialMaximization;  /* initial maximization state.
	                                            one of the RL_GAMECANVAS_MAX_[...] values.          */
	rlGameCanvas_Bool bHideMouseCursor;      /* should the mouse cursor (initially) be hidden on the
	                                             client area when the window has focus?             */
	rlGameCanvas_WinMsgCallback pWMCallback; /* callback function for window messages.
	                                            can be NULL.                                        */
	rlGameCanvas_DrawCallback pDrawCallback; /* callback function that updates the canvas.
	                                            cannot be NULL.                                     */
	rlGameCanvas_CreateData pCreateData;     /* function that creates an rlGameCanvas_GraphicsData
	                                             object.                                            */
	rlGameCanvas_DestroyData pDestroyData;   /* function that destroys an rlGameCanvas_GraphicsData
	                                             object.                                            */
	rlGameCanvas_Config oInitialConfig;      /* the initial graphics configuration.                 */
} rlGameCanvas_StartupConfig;





#endif RLGAMECANVAS_TYPES_C