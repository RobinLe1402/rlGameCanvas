#ifndef RLGAMECANVAS_GAMECANVAS_PIMPL
#define RLGAMECANVAS_GAMECANVAS_PIMPL





#include <rlGameCanvas++/GameCanvas.hpp>

#include "GraphicsData.hpp"
#include "OpenGL.hpp"
#include "PrivateTypes.hpp"

#include <gl/GL.h>

#include <chrono>
#include <map>
#include <mutex>
#include <thread>



namespace rlGameCanvasLib
{
	namespace
	{

		enum class GraphicsThreadState
		{
			NotStarted, // graphics thread hasn't been created yet
			Running,    // run() was called, graphics thread is working.
			Waiting,    // finished previous frame, waiting for next one
			Stopped     // graphics thread was stopped after working.
		};

		enum class Maximization
		{
			Unknown, // for startup
			Windowed,
			Maximized,
			Fullscreen
		};

		struct LayerSettings
		{
			rlGameCanvas_Bool bVisible;
			Resolution        oScreenPos;
		};

	}


	class GameCanvas::PIMPL final
	{
	private: // static methods

		static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static bool RegisterWindowClass();


	private: // static variables

		static std::map<HWND, GameCanvas::PIMPL*> s_oInstances;
		static const HINSTANCE s_hInstance;
		static constexpr wchar_t s_szCLASSNAME[] = L"rlGameCanvas";


	public: // methods

		PIMPL(rlGameCanvas oHandle, const StartupConfig &config);
		~PIMPL();

		// interface methods =======================================================================
		bool run();
		void quit();
		// =========================================================================================


	private: // methods

		// Prepare everything for the current mode.
		// Creates graphics data, maybe sets the window size and calculates the drawing area.
		void initializeCurrentMode();

		// (Re-)create the actual graphics data.
		// Requires m_iCurrentMode to be up to date.
		void createGraphicsData();

		// Set window size and style without processing any of the resize-related messages.
		// Requires m_iCurrentMode and m_eMaximization to be up to date.
		// Updates m_oClientSize.
		void setWindowSize(HWND hWndOnTargetMonitor);

		// Recalculate output rectangle and pixel size.
		// Requires m_iCurrentMode and m_oClientSize to be up to date.
		// Updates m_iPixelSize and m_oDrawRect.
		void calcRenderParams();


		LRESULT localWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		void graphicsThreadProc();

		void renderFrame();
		void waitForGraphicsThread();
		void resumeGraphicsThread(); // copy data, resume graphics thread

		void setFullscreenOnMaximize();

		void doUpdate();

		void updateConfig(const Config &cfg, bool bForceUpdateAll = false);

		void handleResize(unsigned iClientWidth, unsigned iClientHeight);


		void sendMessage(
			rlGameCanvas_UInt     iMsg,
			rlGameCanvas_MsgParam iParam1,
			rlGameCanvas_MsgParam iParam2
		);


	private: // variables

		const rlGameCanvas m_oHandle;

		const HCURSOR m_hCursor = LoadCursorW(NULL, IDC_ARROW);
		HWND          m_hWnd    = NULL;
		HACCEL        m_hAccel  = NULL;
		HGLRC         m_hOpenGL = NULL;
		HDC           m_hDC     = NULL;

		std::unique_ptr<OpenGL> m_upOpenGL; // extended OpenGL interface

		bool   m_bFBO                    = false;
		GLuint m_iIntScaledBufferFBO     = 0;
		GLuint m_iIntScaledBufferTexture = 0;

		std::mutex              m_muxNextFrame;
		std::condition_variable m_cvNextFrame;


		std::thread::id     m_oMainThreadID;
		std::thread         m_oGraphicsThread;
		GraphicsThreadState m_eGraphicsThreadState = GraphicsThreadState::NotStarted;

		// for minimization and closing
		std::mutex              m_muxAppState;
		std::condition_variable m_cvAppState;


		Resolution m_oPrevCursorPos = {};
		bool m_bMouseOverCanvas     = false;

		bool m_bHasFocus            = false;
		bool m_bMinimized           = false;
		bool m_bMinimized_Waiting   = false;
		bool m_bRestoreHandled      = false;
		bool m_bFullscreenToggled   = false;

		bool m_bRunning       = false; // is the game logic running?
		bool m_bStopRequested = false; // user/game requested a stop.
		bool m_bRunningUpdate = false; // is the fnUpdate callback currently being run?
		bool m_bIgnoreResize  = false;
		bool m_bMaxFullscreen = false;

		std::chrono::system_clock::time_point m_tp1, m_tp2;



		// configurable data: startup ==============================================================
		const std::u8string        m_sWindowCaption;
		HICON                      m_hIconSmall;
		HICON                      m_hIconBig;
		const UpdateStateCallback  m_fnUpdateState;
		const DrawStateCallback    m_fnDrawState;
		const CreateStateCallback  m_fnCreateState;
		const CopyStateCallback    m_fnCopyState;
		const DestroyStateCallback m_fnDestroyState;
		const MsgCallback          m_fnOnMsg;
		const WinMsgCallback       m_fnOnWinMsg;
		std::vector<Mode_CPP>      m_oModes;
		const bool                 m_bFullscreenOnMaximize;
		const bool                 m_bDontOversample;
		// configurable data: runtime ==============================================================
		bool         m_bHideMouseCursor;
		Maximization m_eMaximization;
		UInt         m_iCurrentMode = 0;
		Pixel        m_pxBackground = rlGameCanvas_Color_Black;
		//==========================================================================================

		bool m_bNewMode = true;
		Maximization m_eNonFullscreenMaximization;



		Resolution m_oClientSize = {};
		UInt       m_iPixelSize  = 1;
		Rect       m_oDrawRect   = {};

		UInt m_iPixelSize_Restored = 1;

		

		void *m_pvState_Updating = nullptr; // for access in update callback
		void *m_pvState_Drawing  = nullptr; // for access in draw callback

		GraphicsData m_oGraphicsData;
		std::unique_ptr<LayerData[]> m_oLayersForCallback;
		std::unique_ptr<LayerData[]> m_oLayersForCallback_Copy;
		size_t m_iLayersForCallback_Size;
		std::unique_ptr<LayerSettings[]> m_oLayerSettings;


		bool m_bGraphicsThread_NewViewport = true;
		bool m_bGraphicsThread_NewFBOSize  = true;

	};

}





#endif // RLGAMECANVAS_GAMECANVAS_PIMPL