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
		bool updateConfig(const Config &oConfig, UInt iFlags);
		// =========================================================================================


	private: // methods

		LRESULT localWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		void graphicsThreadProc();

		void drawFrame();
		void doUpdate();
		void handleResize(unsigned iClientWidth, unsigned iClientHeight);
		void setResolution(const Resolution &oNewRes, bool bResize);

		void sendMessage(
			rlGameCanvas_UInt     iMsg,
			rlGameCanvas_MsgParam iParam1,
			rlGameCanvas_MsgParam iParam2
		);


	private: // variables

		rlGameCanvas  m_oHandle;
		std::u8string m_sWindowCaption;
		bool          m_bMinimized = false;
		bool          m_bMinimized_Waiting = false;

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

		std::mutex              m_mux; // general-purpose.
		std::condition_variable m_cv;  // general-purpose.
		bool m_bRunning       = false; // is the game logic running?
		bool m_bStopRequested = false; // user/game requested a stop.

		std::mutex              m_muxOpenGL;
		std::condition_variable m_cvOpenGL;
		bool                    m_bOpenGLRequest = false;

		std::mutex   m_muxBuffer;
		GraphicsData m_pBuffer_Updating = nullptr; // for access in update callback
		GraphicsData m_pBuffer_Shared   = nullptr; // ready to be drawn
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