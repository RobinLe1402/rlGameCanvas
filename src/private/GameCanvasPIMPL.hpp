#ifndef RLGAMECANVAS_GAMECANVAS_PIMPL
#define RLGAMECANVAS_GAMECANVAS_PIMPL





#include <rlGameCanvas++/GameCanvas.hpp>

#include "MultiLayerBitmap.hpp"
#include "OpenGL.hpp"

#include <gl/GL.h>
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
			Waiting,    // window was created, graphics thread is waiting to be activated.
			Running,    // run() was called, graphics thread is working.
			Stopped     // graphics thread was stopped after working.
		};

		enum class WindowState
		{
			Unknown,
			Restored,
			Maximized,
			Fullscreen,
			Minimized
		};

		struct Rect
		{
			UInt iLeft;
			UInt iTop;
			UInt iRight;
			UInt iBottom;
		};

		struct RectF
		{
			float fLeft;
			float fTop;
			float fRight;
			float fBottom;
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
		bool updateConfig(const Config &oConfig, UInt iFlags);
		// =========================================================================================


	private: // methods

		LRESULT localWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		void graphicsThreadProc();

		void doUpdate();
		void handleResize(unsigned iClientWidth, unsigned iClientHeight);


	private: // variables

		rlGameCanvas  m_oHandle;
		std::u8string m_sWindowCaption;
		WindowState   m_eWindowState = WindowState::Unknown;

		Resolution m_oClientSize = {};
		UInt       m_iPixelSize  = 0;
		Rect       m_oDrawRect   = {};
		RectF      m_oDrawRectF  = {};

		StartupConfig m_oConfig;

		HWND  m_hWnd    = NULL;
		HGLRC m_hOpenGL = NULL;
		HDC   m_hDC     = NULL;

		std::unique_ptr<OpenGL> m_upOpenGL; // extended OpenGL interface

		std::thread::id     m_oMainThreadID;
		std::thread         m_oGraphicsThread;
		GraphicsThreadState m_eGraphicsThreadState = GraphicsThreadState::NotStarted;

		std::mutex              m_mux; // general-purpose.
		std::condition_variable m_cv;  // general-purpose.
		bool m_bRunning       = false; // is the game logic running?
		bool m_bStopRequested = false; // user/game requested a stop.

		std::mutex m_muxConfig; // todo: use!
		bool       m_bNewConfig = false;
		Config     m_oNewConfig = {};

		std::mutex   m_muxBuffer;
		GraphicsData m_pBuffer_Updating = nullptr; // for access in update callback
		GraphicsData m_pBuffer_Shared   = nullptr; // ready to be drawn
		GraphicsData m_pBuffer_Drawing  = nullptr; // for access in draw callback

		MultiLayerBitmap m_oLayers;

		GLuint m_iIntScaledBufferFBO     = 0;
		GLuint m_iIntScaledBufferTexture = 0;
	};

}





#endif // RLGAMECANVAS_GAMECANVAS_PIMPL