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
			Waiting,    // waiting for a task
			Drawing,    // drawing a frame
			Asleep,     // waiting to be woken up (e.g. on minimization)
			Stopped     // graphics thread was stopped after working.
		};

		enum class GraphicsThreadTask
		{
			Draw,         // draw next frame (default)
			GiveUpOpenGL, // logic thread needs control over OpenGL
			Pause,        // for minimization
			Stop          // canvas is being shut down
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
		static void CALLBACK TimerProc(HWND hWnd, UINT uMsg, UINT idTimer, DWORD dwTime);
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

		const Mode_CPP &currentMode() { return m_oModes[m_iCurrentMode]; }

		// Prepare everything for the current mode.
		// Creates graphics data, maybe sets the window size and calculates the drawing area.
		void initializeCurrentMode();

		// (Re-)create the actual graphics data.
		// Requires m_iCurrentMode to be up to date.
		void createGraphicsData();

		void enterFullscreenMode();
		void exitFullscreenMode();
		void adjustWindowedSize();

		// Recalculate output rectangle and pixel size.
		// Requires m_iCurrentMode and m_oClientSize to be up to date.
		// Updates m_iPixelSize and m_oDrawRect.
		void calcRenderParams();

		// Apply the current cursor restriction setting.
		void applyCursorRestriction();

		// Apply the current cursor visibility setting.
		void applyCursor();

		// Get the draw rectangle, in screen coordinates.
		RECT getDrawRect();


		LRESULT localWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		void onRepaintTimer();

		void graphicsThreadProc();

		void renderFrame();
		void waitForGraphicsThread();
		void runGraphicsTask(GraphicsThreadTask eTask);

		void logicFrame(); // update + draw state (to be called from the logic thread

		void doUpdate();

		void updateConfig(const Config &cfg);


		void sendMessage(
			rlGameCanvas_UInt     iMsg,
			rlGameCanvas_MsgParam iParam1,
			rlGameCanvas_MsgParam iParam2
		);


	private: // variables

		const rlGameCanvas m_oHandle;

		HWND          m_hWnd    = NULL;
		HACCEL        m_hAccel  = NULL;
		HGLRC         m_hOpenGL = NULL;
		HDC           m_hDC     = NULL;
		HMONITOR      m_hMon    = NULL;

		Resolution m_oWindowedBorderSize = {};
		const Resolution m_oMinWinSize =
		{ /* x */ (UInt)GetSystemMetrics(SM_CXMIN), /* y */ (UInt)GetSystemMetrics(SM_CYMIN)};

		WINDOWPLACEMENT m_wndpl = {};

		std::unique_ptr<OpenGL> m_upOpenGL; // extended OpenGL interface

		bool   m_bFBO                    = false;
		GLuint m_iIntScaledBufferFBO     = 0;
		GLuint m_iIntScaledBufferTexture = 0;

		GraphicsThreadTask      m_eGraphicsThreadTask = GraphicsThreadTask::Draw;
		std::mutex              m_muxGraphicsThread;
		std::condition_variable m_cvGraphicsThread;
		std::mutex              m_muxGraphicsTask;
		std::condition_variable m_cvGraphicsTask;


		std::thread::id     m_oMainThreadID;
		std::thread         m_oGraphicsThread;
		GraphicsThreadState m_eGraphicsThreadState = GraphicsThreadState::NotStarted;

		std::condition_variable m_cvControlOverGL;
		bool m_bGraphicsThreadHasControlOverGL = false;


		Resolution m_oCursorPos          = {};
		bool m_bMouseCursorOutsideClient = true;
		bool m_bMouseOverCanvas          = false;

		bool m_bHasFocus          = false;
		bool m_bMinimized         = false;
		bool m_bFullscreenToggled = false;
		bool m_bMouseTracking     = false;

		bool m_bRunning      = false; // is the game logic running?
		bool m_bIgnoreResize = false;

		std::chrono::system_clock::time_point m_tp1, m_tp2;



		// configurable data: startup ==============================================================
		const std::string          m_sWindowCaption;
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
		const bool                 m_bPreferPixelPerfect;
		bool                       m_bRestrictCursor;
		// configurable data: runtime ==============================================================
		bool         m_bHideCursor;
		bool         m_bFullscreen;
		bool         m_bMaximized;
		UInt         m_iCurrentMode = 0;
		Pixel        m_pxBackground = rlGameCanvas_Color_Black;
		//==========================================================================================

		double m_dTimeSinceLastMouseMove = 0.0;
		bool   m_bHideCursorEx = false;

		bool m_bNewMode = true;



		Resolution m_oClientSize    = {};
		UInt       m_iPixelSize     = 1;
		UInt       m_iPixelSize_Win = 2;
		Rect       m_oDrawRect      = {};

		

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