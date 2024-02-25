#ifndef RLGAMECANVAS_GAMECANVAS_PIMPL
#define RLGAMECANVAS_GAMECANVAS_PIMPL





#include <rlGameCanvas++/GameCanvas.hpp>

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
			Waiting, // window was created, graphics thread is waiting to be activated.
			Running, // run() was called, graphics thread is working.
			Stopped  // graphics thread was stopped after working.
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


	private: // variables

		rlGameCanvas m_oHandle;

		StartupConfig m_oConfig;

		std::u8string m_sWindowCaption;

		HWND  m_hWnd    = NULL;
		HGLRC m_hOpenGL = NULL;

		std::thread::id     m_oMainThreadID;
		std::thread         m_oGraphicsThread;
		GraphicsThreadState m_eGraphicsThreadState = GraphicsThreadState::Waiting;
		/* initialized with Waiting because thread will be started within the constructor.        */

		std::mutex              m_mux; // general-purpose.
		std::condition_variable m_cv;  // general-purpose.
		bool m_bRunning       = false; // is the game logic running?
		bool m_bStopRequested = false; // user/game requested a stop.

		std::mutex m_muxConfig;
		bool       m_bNewConfig = false;
		Config     m_oNewConfig = {};

		std::mutex   m_muxBufferChange; // for when one buffer is done processing on either threads
		unsigned     m_iCurrentLiveBuffer = 0;
		std::mutex   m_muxBuffers[2]; 
		GraphicsData m_pBuffers_Live[2] = {};
		GraphicsData m_pBuffer_Drawing  = nullptr;
	};

}





#endif // RLGAMECANVAS_GAMECANVAS_PIMPL