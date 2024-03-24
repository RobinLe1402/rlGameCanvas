#ifndef RLGAMECANVAS_CORE_DEFINITIONS_C
#define RLGAMECANVAS_CORE_DEFINITIONS_C





/*
	MESSAGE:           RL_GAMECANVAS_MSG_CREATE
	TRIGGER:           Graphics engine is starting.
	                   Guaranteed to be called exactly one time, as the very first message.
	EXPECTED BEHAVIOR: Initialization of internal data.
	PARAMETER 1:       [rlGameCanvas_UInt*] A pointer to the index of the mode to use at launch.
	                                        The default value is 0.
	                                        If an invalid index is passed, mode 0 will be used.
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
	MESSAGE:           RL_GAMECANVAS_MSG_MINIMIZE
	TRIGGER:           Window is minimized/restored from minimization.
	EXPECTED BEHAVIOR: Mute audio altogether/continue regular execution.
	PARAMETER 1:       [rlGameCanvas_Bool] Nonzero: The window was minimized.
	                                       Zero:    The window was restored from minimization.
	PARAMETER 2:       Not used.
*/
#define RL_GAMECANVAS_MSG_MINIMIZE (0x00000005)










/*
	SUP = StartUp

	RL_GAMECANVAS_SUP_WINDOWED
		Launch in windowed mode.
		Don't combine with RL_GAMECANVAS_SUP_MAXIMIZED or RL_GAMECANVAS_SUP_FULLSCREEN.
	RL_GAMECANVAS_SUP_MAXIMIZED
		Launch in maximized mode.
		Don't combine with RL_GAMECANVAS_SUP_FULLSCREEN_ON_MAXIMZE, RL_GAMECANVAS_SUP_WINDOWED or
		RL_GAMECANVAS_SUP_FULLSCREEN.
	RL_GAMECANVAS_SUP_FULLSCREEN
		Launch in fullscreen mode.
		Don't combine with RL_GAMECANVAS_SUP_WINDOWED or RL_GAMECANVAS_SUP_MAXIMIZED.
	RL_GAMECANVAS_SUP_FULLSCREEN_ON_MAXIMZE
		When the user tries to maximize the window, switch to fullscreen mode instead.
		If this flag is set, RL_GAMECANVAS_SUP_MAXIMIZED cannot be set, as maximization is disabled
		altogether.
	RL_GAMECANVAS_SUP_RESTRICT_CURSOR
		If this flag is set, the mouse cursor position will be restricted to the canvas while the
		window has focus.
	RL_GAMECANVAS_SUP_DONT_OVERSAMPLE
		If this flag is not set, the screen will allways be upscaled to fill the maximum area while
		retaining the original aspect ratio.
		If this flag is set, the screen will only be upscaled in multiples of the original size.
	RL_GAMECANVAS_SUP_HIDECURSOR
		If this flag is set, the cursor will initially be hidden.
		This can be changed later in the Update callback.
*/
#define RL_GAMECANVAS_SUP_WINDOWED              (0x00000001)
#define RL_GAMECANVAS_SUP_MAXIMIZED             (0x00000002)
#define RL_GAMECANVAS_SUP_FULLSCREEN            (0x00000004)
// (0x00000008) is reserved for future use.
#define RL_GAMECANVAS_SUP_FULLSCREEN_ON_MAXIMZE (0x00000010)
#define RL_GAMECANVAS_SUP_DONT_OVERSAMPLE       (0x00000020)
#define RL_GAMECANVAS_SUP_RESTRICT_CURSOR       (0x00000040)
#define RL_GAMECANVAS_SUP_HIDE_CURSOR           (0x00000080)










/*
	CFG = Config

	RL_GAMECANVAS_CFG_FULLSCREEN
		If this flag is set, the canvas enters fullscreen mode.
		If it's cleared, the canvas exists fullscreen mode.
	RL_GAMECANVAS_CFG_RESTRICT_CURSOR
		If this flag is set, the mouse cursor position will be restricted to the canvas area.
	RL_GAMECANVAS_CFG_HIDECURSOR
		If this flag is set, the mouse cursor will be hidden over the client area when the window
		has focus.
*/
#define RL_GAMECANVAS_CFG_FULLSCREEN      (0x00000001)
#define RL_GAMECANVAS_CFG_RESTRICT_CURSOR (0x00000002)
#define RL_GAMECANVAS_CFG_HIDE_CURSOR     (0x00000004)










/*
	STA = State

	RL_GAMECANVAS_STA_MOUSE_ON_CANVAS
		If this flag is set, the mouse cursor is on the client area.
		Otherwise, it isn't.
*/
#define RL_GAMECANVAS_STA_MOUSE_ON_CANVAS (0x00000001)










/*
	DRW = Draw

	RL_GAMECANVAS_DRW_NEWMODE
		If this flag is set, a mode switch occured.
		This means that...
		* all layers are now empty and must be refilled from scratch
		* the screen positions of all layers were reset to (0,0)
		
		This happens...
		* at the very first call to the Draw callback.
		* whenever a mode change occurs.
*/
#define RL_GAMECANVAS_DRW_NEWMODE (0x00000001)





#endif // RLGAMECANVAS_CORE_DEFINITIONS_C