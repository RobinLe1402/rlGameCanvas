#ifndef RLGAMECANVAS_CORE_DEFINITIONS_C
#define RLGAMECANVAS_CORE_DEFINITIONS_C





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

/*
	MESSAGE:           RL_GAMECANVAS_MSG_RESIZE
	TRIGGER:           Canvas size has changed.
	EXPECTED BEHAVIOR: Respect the new size in all newly written data buffers.
	PARAMETER 1:       [rlGameCanvas_ResizeInputParams*] Information about the resize.
	                   Changes are ignored.
	PARAMETER 2:       [rlGameCanvas_ResizeOutputParams*] Select settings that can be changed right
	                    now.
*/
#define RL_GAMECANVAS_MSG_RESIZE (0x00000005)

/*
	MESSAGE:           RL_GAMECANVAS_MSG_MINIMIZE
	TRIGGER:           Window is minimized/restored from minimization.
	EXPECTED BEHAVIOR: Mute audio altogether/continue regular execution.
	PARAMETER 1:       [rlGameCanvas_Bool] Nonzero: The window was minimized.
	                                       Zero:    The window was restored from minimization.
	PARAMETER 2:       Not used.
*/
#define RL_GAMECANVAS_MSG_MINIMIZE (0x00000006)

// TODO: make use of mouse movements?
///*
//	MESSAGE:           RL_GAMECANVAS_MSG_MOUSEMOVE
//	TRIGGER:           The mouse cursor has moved to a new position on the client area.
//	                   Only fired when the window has focus.
//	EXPECTED BEHAVIOR: Possibly move mouse cursor/change button highlighting.
//	PARAMETER 1:       [rlGameCanvas_Resolution*] The coordinates of the pixel the mouse cursor is
//	                   now on.
//	PARAMETER 2:       Not used.
//*/
//#define RL_GAMECANVAS_MSG_MOUSEMOVE (0x00000007)
//
///*
//	MESSAGE:           RL_GAMECANVAS_MSG_MOUSELEAVE
//	TRIGGER:           a) The window has focus and the mouse cursor has moved to a new position
//	                      outside the client area.
//	                   b) The window has lost focus. Sent after RL_GAMECANVAS_MSG_LOSEFOCUS.
//	EXPECTED BEHAVIOR: Possibly move mouse cursor/change button highlighting.
//	PARAMETER 1:       Not used.
//	PARAMETER 2:       Not used.
//*/
//#define RL_GAMECANVAS_MSG_MOUSELEAVE (0x00000008)










/*
	MAX = Maximization

	Enum defining...
	1) the behavior on attempted maximization via the maximize button by the user.
	2) the initial state of the canvas.
*/
#define RL_GAMECANVAS_MAX_NONE       0 /* 1) disable maximize button.
                                          2) start in windowed mode.                              */
#define RL_GAMECANVAS_MAX_MAXIMIZE   1 /* 1) regular maximization.
                                          2) start in maximized mode.
										  behaves like a smaller fullscreen mode.                 */
#define RL_GAMECANVAS_MAX_FULLSCREEN 2 /* 1) switch to fullscreen mode instead of maximizing.
                                          2) start in fullscreen mode.                            */










/*
	UPD = Update
	Flags for the update callack.


	RL_GAMECANVAS_UPD_READONLYCONFIG
		Changes to the struct pointed to by pConfig will be ignored.
		This flag is set when the current call to the Update callback is already the result of a
		change in configuration/setup, in order to prevent endless loops.

	RL_GAMECANVAS_UPD_REPAINT
		The layer data has been cleared and must be redrawn from scratch.
		Unlike if this flag is not set, the resulting data pointer is guaranteed to be used in the
		very next drawn screen.
		This flag is set on the very first call to the Update callback as well as after the canvas'
		resolution has been changed.
*/
#define RL_GAMECANVAS_UPD_READONLYCONFIG (0x00000001)
#define RL_GAMECANVAS_UPD_REPAINT        (0x00000002)





#endif // RLGAMECANVAS_CORE_DEFINITIONS_C