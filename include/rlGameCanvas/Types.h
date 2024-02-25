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
	// TODO: add different layer sizes?
	rlGameCanvas_Pixel *pData; /* the pixel data - left to right, top to bottom.
	                              value at position (x,y) can be found at index [y * width + x]. */
} rlGameCanvas_Layer;



typedef struct rlGameCanvas_OpaquePtrStruct
{
	int iUnused;
} *rlGameCanvas;



typedef void (__stdcall *rlGameCanvas_CreateData )(rlGameCanvas_GraphicsData *ppData);
typedef void (__stdcall *rlGameCanvas_DestroyData)(rlGameCanvas_GraphicsData  pData);
typedef void (__stdcall *rlGameCanvas_CopyData)(rlGameCanvas_GraphicsData     pSrc,
	rlGameCanvas_GraphicsData pDest);

typedef struct
{
	rlGameCanvas_UInt x, y;
} rlGameCanvas_Resolution;




typedef void (__stdcall *rlGameCanvas_WinMsgCallback)(
	rlGameCanvas canvas, // the canvas that received the message.
	UINT         uMsg,   // the message code, see WM_[...].
	WPARAM       wParam, // generic parameter #1.
	LPARAM       lParam  // generic parameter #2.
);



typedef void (__stdcall *rlGameCanvas_DrawCallback)(
		  rlGameCanvas              canvas,  // the canvas to be modified.
	      rlGameCanvas_Layer       *pLayers, // array of the layers to be updated.
	      rlGameCanvas_UInt         iLayers, // count of elements in the pLayers array.
	const rlGameCanvas_GraphicsData pData    // the data to be used for updating the canvas.
);



typedef void (__stdcall *rlGameCanvas_UpdateCallback)(
	rlGameCanvas              canvas,            /* the canvas to be updated.                     */
	rlGameCanvas_GraphicsData pData,             /* the data to be written.                       */
	double                    dSecsSinceLastCall /* number of seconds since last call.
	                                                0 on the first call.                          */
);



typedef struct
{
	rlGameCanvas_Resolution oOldRes; /* the old size, in pixels, of the canvas.                   */
	rlGameCanvas_Resolution oNewRes; /* the new size, in pixels, of the canvas.                   */
} rlGameCanvas_ResizeParams;



typedef uint64_t rlGameCanvas_MsgParam;

typedef void (__stdcall *rlGameCanvas_MsgCallback)(
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
	rlGameCanvas_UInt iWidth, iHeight;  /* size, in pixels, of the canvas.                        */
	rlGameCanvas_UInt iScaling;         /* the factor for up-"scaling". 0 = maximum.              */
	rlGameCanvas_UInt iMaximization;    /* maximization state.
	                                       one of the RL_GAMECANVAS_MAX_[...] values.             */

	// TODO: offer option to hide mouse cursor over client area?
	//rlGameCanvas_Bool bHideMouseCursor; /* should the mouse cursor be hidden on the client area when
	//                                        the window has focus?                                 */
} rlGameCanvas_Config;



typedef struct
{
	const rlGameCanvas_U8Char *szWindowCaption; /* the (UTF-8) text that should appear in the
	                                                titlebar.
	                                               can be NULL.                                   */
	HICON hIconSmall, hIconBig;                 /* the icons for the titlebar and taskbar.
	                                               can be NULL.                                   */
	rlGameCanvas_UInt iMaximizeBtnAction;       /* behaviour when user clicks the maximize button.
	                                               one of the RL_GAMECANVAS_MAX_[...] values.     */
	rlGameCanvas_UpdateCallback fnUpdate;       /* callback function for updating the graphics data
	                                                (= the code for the main game loop).
	                                               cannot be NULL.                                */
	rlGameCanvas_MsgCallback fnOnMsg;           /* callback function for custom messages.
	                                               can be NULL.                                   */
	rlGameCanvas_WinMsgCallback fnOnWinMsg;     /* callback function for window messages.
	                                               can be NULL.                                   */
	rlGameCanvas_DrawCallback fnDraw;           /* callback function that updates the canvas.
	                                               cannot be NULL.                                */
	rlGameCanvas_CreateData fnCreateData;       /* function that creates an
	                                                rlGameCanvas_GraphicsData object.             */
	rlGameCanvas_DestroyData fnDestroyData;     /* function that destroys an
	                                                rlGameCanvas_GraphicsData object.
	                                               cannot be NULL.                                */
	rlGameCanvas_CopyData fnCopyData;           /* function that copies the contents of one
	                                                rlGameCanvas_GraphicsData object into another
	                                                one.
	                                               cannot be NULL.                                */
	rlGameCanvas_Config oInitialConfig;         /* the part of the configuration that can be changed
	                                                at runtime.                                   */
} rlGameCanvas_StartupConfig;





#endif RLGAMECANVAS_TYPES_C