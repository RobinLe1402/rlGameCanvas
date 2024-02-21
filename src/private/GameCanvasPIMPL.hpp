#ifndef RLGAMECANVAS_GAMECANVAS_PIMPL
#define RLGAMECANVAS_GAMECANVAS_PIMPL





#include <rlGameCanvas++/GameCanvas.hpp>

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


	private: // static variables

		static std::map<HWND, GameCanvas::PIMPL*> s_oInstances;


	public: // methods

		PIMPL(const StartupConfig &config);
		~PIMPL();

		// interface methods =======================================================================
		bool run();
		void quit();
		bool updateConfig(const Config &oConfig, UInt iFlags);
		// =========================================================================================


	private: // methods

		LRESULT localWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		void graphicsThreadProc();
		void logicThreadProc();


	private: // variables

		StartupConfig m_oConfig;

#ifndef RLGAMECANVAS_NO_CHAR8
		std::u8string m_sWindowCaption;
#else
		std::string m_sWindowCaption;
#endif

		std::thread::id     m_oMainThreadID;
		std::thread         m_oGraphicsThread;
		GraphicsThreadState m_eGraphicsThreadState = GraphicsThreadState::Waiting;
		/* initialized with Waiting because thread will be started within the constructor.        */

		std::mutex m_mux;
		std::mutex m_muxBetweenFrames;
		std::condition_variable m_cv;
		bool m_bRunning       = false;
		bool m_bStopRequested = false;

		bool   m_bNewConfig = false;
		Config m_oNewConfig = {};
		
		HWND m_hWnd = NULL;
		GraphicsData m_pGraphicsData_Live    = nullptr;
		GraphicsData m_pGraphicsData_Drawing = nullptr;
	};

}





#endif // RLGAMECANVAS_GAMECANVAS_PIMPL