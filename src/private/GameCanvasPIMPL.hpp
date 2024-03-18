#ifndef RLGAMECANVAS_GAMECANVAS_PIMPL
#define RLGAMECANVAS_GAMECANVAS_PIMPL





#include <rlGameCanvas++/GameCanvas.hpp>

#include "MultiLayerBitmap.hpp"
#include "OpenGL.hpp"

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

		struct Rect
		{
			UInt iLeft;
			UInt iTop;
			UInt iRight;
			UInt iBottom;
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

		PIMPL(const StartupConfig &config, rlGameCanvas oHandle);
		~PIMPL();

		// interface methods =======================================================================
		bool run();
		void quit();
		// =========================================================================================


	private: // methods

		LRESULT localWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		void graphicsThreadProc();

		void renderFrame();
		void waitForGraphicsThread(bool bRequestOpenGL);
		void resumeGraphicsThread(); // copy data, resume graphics thread

		bool doUpdate(bool bConfigChangable, bool bRepaint); // return value: was frame rendered?
		void handleResize(unsigned iClientWidth, unsigned iClientHeight,
			UInt iPreviousMaximization, UInt iNewMaximization);
		void setResolution(const Resolution &oNewRes, bool bResize);
		void setMaximization(UInt iMaximization, Resolution oRes, UInt &iPixelSize);

		void sendMessage(
			rlGameCanvas_UInt     iMsg,
			rlGameCanvas_MsgParam iParam1,
			rlGameCanvas_MsgParam iParam2
		);


	private: // variables

		rlGameCanvas  m_oHandle;
		std::u8string m_sWindowCaption;
		bool          m_bRunningUpdate     = false; // is the fnUpdate callback currently being run?
		bool          m_bMinimized         = false;
		bool          m_bMinimized_Waiting = false;
		bool          m_bIgnoreResize      = false;
		bool          m_bMaxFullscreen     = false;

		Resolution m_oClientSize = {};
		UInt       m_iPixelSize  = 0;
		Rect       m_oDrawRect   = {};

		std::chrono::system_clock::time_point m_tp1, m_tp2;

		StartupConfig m_oConfig;

		HWND  m_hWnd    = NULL;
		HGLRC m_hOpenGL = NULL;
		HDC   m_hDC     = NULL;

		std::unique_ptr<OpenGL> m_upOpenGL; // extended OpenGL interface

		std::thread::id     m_oMainThreadID;
		std::thread         m_oGraphicsThread;
		GraphicsThreadState m_eGraphicsThreadState = GraphicsThreadState::NotStarted;

		// for minimization and closing
		std::mutex              m_muxAppState;
		std::condition_variable m_cvAppState;

		bool m_bRunning       = false; // is the game logic running?
		bool m_bStopRequested = false; // user/game requested a stop.

		std::mutex              m_muxNextFrame;
		std::condition_variable m_cvNextFrame;
		bool                    m_bOpenGLRequest = false;

		GraphicsData m_pBuffer_Updating = nullptr; // for access in update callback
		GraphicsData m_pBuffer_Drawing  = nullptr; // for access in draw callback

		MultiLayerBitmap m_oLayers;
		std::unique_ptr<Layer[]> m_oLayersForCallback;
		std::unique_ptr<Layer[]> m_oLayersForCallback_Copy;
		size_t m_iLayersForCallback_Size;

		bool   m_bFBO                    = false;
		GLuint m_iIntScaledBufferFBO     = 0;
		GLuint m_iIntScaledBufferTexture = 0;
	};

}





#endif // RLGAMECANVAS_GAMECANVAS_PIMPL