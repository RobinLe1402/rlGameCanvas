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



/*
	MESSAGE:           RL_GAMECANVAS_MSG_CREATE
	TRIGGER:           Graphics engine is starting.
	                   Guaranteed to be called exactly one time, as the very first message.
	EXPECTED BEHAVIOR: Initialization of internal data.
	PARAMETER 1:       Not used.
	PARAMETER 2:       Not used.
*/
#define RL_GAMECANVAS_MSG_CREATE (0x00000001)

/*
	MESSAGE:           RL_GAMECANVAS_MSG_DESTROY
	TRIGGER:           Graphics engine is exiting.
	                   Guaranteed to be called exactly one time, as the very last message.
	EXPECTED BEHAVIOR: Cleanup of internal data.
	PARAMETER 1:       Not used.
	PARAMETER 2:       Not used.
*/
#define RL_GAMECANVAS_MSG_DESTROY (0x00000002)

/*
	MESSAGE:           RL_GAMECANVAS_MSG_LOSEFOCUS
	TRIGGER:           Graphics engine has lost focus.
	EXPECTED BEHAVIOR: Stop music/reduce volume, pause if necessary.
	PARAMETER 1:       Not used.
	PARAMETER 2:       Not used.
*/
#define RL_GAMECANVAS_MSG_LOSEFOCUS (0x00000003)

/*
	MESSAGE:           RL_GAMECANVAS_MSG_GAINFOCUS
	TRIGGER:           Graphics engine has gained focus.
	EXPECTED BEHAVIOR: Resume music/reset volume, possibly unpause.
	PARAMETER 1:       Not used.
	PARAMETER 2:       Not used.
*/
#define RL_GAMECANVAS_MSG_GAINFOCUS (0x00000004)

// TODO: replace "<TBA>"
/*
	MESSAGE:           RL_GAMECANVAS_MSG_RESIZE
	TRIGGER:           Client area size has changed (= fullscreen mode/maximization was toggled).
	EXPECTED BEHAVIOR: Possibly adjust the viewport and layer resolutions via the <TBA> function.
	PARAMETER 1:       [rlGameCanvas_ResizeParams*] Information about the resize. Changes are ignored.
	PARAMETER 2:       Not used.
*/
#define RL_GAMECANVAS_MSG_RESIZE (0x00000005)

/*
	MESSAGE:           RL_GAMECANVAS_MSG_MOUSEMOVE
	TRIGGER:           The mouse cursor has moved to a new position on the client area.
	                   Only fired when the window has focus.
	EXPECTED BEHAVIOR: Possibly move mouse cursor/change button highlighting.
	PARAMETER 1:       [rlGameCanvas_Resolution*] The coordinates of the pixel the mouse cursor is now
	                   on.
	PARAMETER 2:       Not used.
*/
#define RL_GAMECANVAS_MSG_MOUSEMOVE (0x00000006)

/*
	MESSAGE:           RL_GAMECANVAS_MSG_MOUSELEAVE
	TRIGGER:           a) The window has focus and the mouse cursor has moved to a new position
	                      outside the client area.
					   b) The window has lost focus. Sent after RL_GAMECANVAS_MSG_LOSEFOCUS.
	EXPECTED BEHAVIOR: Possibly move mouse cursor/change button highlighting.
	PARAMETER 1:       Not used.
	PARAMETER 2:       Not used.
*/
#define RL_GAMECANVAS_MSG_MOUSELEAVE (0x00000007)



typedef uint64_t rlGameCanvas_MsgParam;

typedef void (*rlGameCanvas_MsgCallback)(
	rlGameCanvas          canvas,
	rlGameCanvas_UInt     iMsg,
	rlGameCanvas_MsgParam iParam1,
	rlGameCanvas_MsgParam iParam2
);



/*
	MAX = Maximization
	Enum defining the behavior on attempted maximization via the maximize button by the user.
*/
#define RL_GAMECANVAS_MAX_NONE       0 /* disable maximization altogether.                          */
#define RL_GAMECANVAS_MAX_MAXIMIZE   1 /* regular maximization.
                                          behaves like a smaller fullscreen mode.                   */
#define RL_GAMECANVAS_MAX_FULLSCREEN 2 /* switch to fullscreen mode instead of maximizing.          */



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
	char *szWindowCaption;                   /* the text that should appear in the titlebar.
	                                            can be NULL.                                        */
	HICON hIconSmall, hIconBig;              /* the icons for the titlebar and taskbar.
	                                            can be NULL.                                        */
	rlGameCanvas_UInt iMaximization;         /* maximization behaviour.
	                                            one of the RL_GAMECANVAS_MAX_[...] values.          */
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