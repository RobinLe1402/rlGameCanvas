#include "private/GameCanvasPIMPL.hpp"
#include "private/Windows.hpp"
#include <rlGameCanvas/Definitions.h>

namespace rlGameCanvasLib
{
	namespace
	{

		[[noreturn]]
		void ThrowWithLastError(const char *szMessage, DWORD dwLastError = GetLastError())
		{
			std::string sFullMessage = szMessage;
			sFullMessage += '\n' + WindowsLastErrorString(dwLastError);

			throw std::exception{ sFullMessage.c_str() };
		}

	}


	std::map<HWND, GameCanvas::PIMPL*> GameCanvas::PIMPL::s_oInstances;
	const HINSTANCE GameCanvas::PIMPL::s_hInstance = GetModuleHandle(NULL);

	LRESULT CALLBACK GameCanvas::PIMPL::StaticWndProc(HWND hWnd, UINT uMsg, WPARAM wParam,
		LPARAM lParam)
	{
		if (hWnd == NULL)
		{
			// no handle specified --> broadcast to all currently known windows
			for (auto &o : s_oInstances)
			{
				o.second->localWndProc(hWnd, uMsg, wParam, lParam);
			}
			return 0;
		}
		else
		{
			auto it = s_oInstances.find(hWnd);
			if (it == s_oInstances.end())
			{
				if (uMsg != WM_CREATE)
					return DefWindowProcW(hWnd, uMsg, wParam, lParam);;

				const auto &cs = *reinterpret_cast<const CREATESTRUCT *>(lParam);
				s_oInstances.insert(
					{ hWnd, reinterpret_cast<GameCanvas::PIMPL *>(cs.lpCreateParams) }
				);

				it = s_oInstances.find(hWnd);
			}

			return it->second->localWndProc(hWnd, uMsg, wParam, lParam);
		}
	}

	bool GameCanvas::PIMPL::RegisterWindowClass()
	{
		static bool bRegistered = false;

		if (bRegistered)
			return true;

		WNDCLASSEXW wc = { sizeof(wc) };
		wc.hInstance = s_hInstance;
		wc.lpszClassName = s_szCLASSNAME;
		wc.lpfnWndProc   = StaticWndProc;
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;

		bRegistered = RegisterClassExW(&wc);
		return bRegistered;
	}


	GameCanvas::PIMPL::PIMPL(const StartupConfig &config, rlGameCanvas oHandle) :
		m_oConfig(config), m_oHandle(oHandle)
	{
		// check if the configuration is valid
		if (config.fnUpdate      == nullptr ||
			config.fnDraw        == nullptr ||
			config.fnCreateData  == nullptr ||
			config.fnDestroyData == nullptr ||
			config.fnCopyData    == nullptr ||
			config.iMaximizeBtnAction           > 2 ||
			config.oInitialConfig.iMaximization > 2 ||
			config.oInitialConfig.iWidth  == 0 ||
			config.oInitialConfig.iHeight == 0)
			throw std::exception{ "Invalid startup configuration." };


		// initialize the window caption string
		if (config.szWindowCaption == nullptr) // default text
			m_sWindowCaption = u8"rlGameCanvas";
		else // custom text
		{
			m_sWindowCaption = config.szWindowCaption;

			// pointer might be invalid after constructor finishes, so it's safer to just set it to
			// nullptr and to use m_sWindowCaption instead.
			m_oConfig.szWindowCaption = nullptr;
		}

		// try to register the window class
		if (!RegisterWindowClass())
			ThrowWithLastError("Failed to register the window class.");

		// try to create the window
		if (CreateWindowExW(
				NULL,                                                   // dwExStyle
				s_szCLASSNAME,                                          // lpClassName
				UTF8toWindowsUnicode(m_sWindowCaption.c_str()).c_str(), // lpWindowName
				WS_OVERLAPPEDWINDOW,                                    // dwStyle // TODO
				CW_USEDEFAULT,                                          // X       // TODO
				CW_USEDEFAULT,                                          // Y       // TODO
				CW_USEDEFAULT,                                          // nWidth  // TODO
				CW_USEDEFAULT,                                          // nHeight // TODO
				NULL,                                                   // hWndParent
				NULL,                                                   // hMenu
				s_hInstance,                                            // hInstance
				this                                                    // lpParam
			) == NULL)
			ThrowWithLastError("Failed to create the window.");


		std::unique_lock lock(m_mux);
		m_oGraphicsThread = std::thread(&GameCanvas::PIMPL::graphicsThreadProc, this);
		m_cv.wait(lock); // wait for the graphics thread to finish the attempt to setup OpenGL.

		if (m_hOpenGL == NULL) // OpenGL could not be initialized --> error, join thread now.
		{
			if (m_oGraphicsThread.joinable())
				m_oGraphicsThread.join();


			lock.unlock();

			if (m_hWnd != NULL) // to remove warning C6387: "might be "0""
				DestroyWindow(m_hWnd);
			
			throw std::exception{ "Failed to initialize OpenGL." };
		}

		m_oConfig.fnCreateData(&m_pBuffers_Live[0]);
		m_oConfig.fnCreateData(&m_pBuffers_Live[1]);
		m_oConfig.fnCreateData(&m_pBuffer_Drawing);
	}

	GameCanvas::PIMPL::~PIMPL() { quit(); }

	bool GameCanvas::PIMPL::run()
	{
		std::unique_lock lock(m_mux);
		if (m_eGraphicsThreadState != GraphicsThreadState::Waiting)
			return false;

		m_oMainThreadID = std::this_thread::get_id();

		// TODO: force initialization of first frame before running graphics thread?

		m_bRunning = true;
		m_cv.notify_one(); // tell graphics thread to start working
		lock.unlock();

		ShowWindow(m_hWnd, SW_SHOW);
		SetForegroundWindow(m_hWnd);

		MSG msg{};
		while (true)
		{
			while (PeekMessageW(&msg, m_hWnd, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);

				if (!m_bRunning)
					goto lbClose;
			}

			doUpdate();
		}
	lbClose:
				
		m_oMainThreadID = {};

		if (m_oGraphicsThread.joinable())
			m_oGraphicsThread.join();

		return true;
	}

	void GameCanvas::PIMPL::quit()
	{
		if (m_eGraphicsThreadState != GraphicsThreadState::Running)
			return;

		std::unique_lock lock(m_mux);
		m_bStopRequested = true;
	}

	bool GameCanvas::PIMPL::updateConfig(const Config &oConfig, UInt iFlags)
	{
		// only calls from the main thread currently running game logic are accepted.
		if (std::this_thread::get_id() != m_oMainThreadID)
			return false;

		std::unique_lock lock(m_muxConfig);
		if (m_eGraphicsThreadState == GraphicsThreadState::Stopped)
			return false;

		m_bNewConfig = true;
		m_oNewConfig = oConfig;
		m_cv.notify_one(); // TODO: maybe split up into multiple CVs and/or Mutexes?
		m_cv.wait(lock); // wait for thread to apply new settings

		return !m_bNewConfig;
	}

	LRESULT GameCanvas::PIMPL::localWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		// TODO: handle more messages
		switch (uMsg)
		{
		case WM_CREATE:
			m_hWnd = hWnd;
			if (m_oConfig.fnOnMsg)
				m_oConfig.fnOnMsg(m_oHandle, RL_GAMECANVAS_MSG_CREATE, 0, 0);
			break;

		case WM_CLOSE:
		{
			std::unique_lock lock(m_mux);
			m_bRunning = false;
			if (m_eGraphicsThreadState != GraphicsThreadState::Stopped)
				m_cv.wait(lock);
		}
			DestroyWindow(m_hWnd);
			return 0;

		case WM_DESTROY:
			if (m_oConfig.fnOnMsg)
				m_oConfig.fnOnMsg(m_oHandle, RL_GAMECANVAS_MSG_DESTROY, 0, 0);

			PostQuitMessage(0);
			return 0;

		case WM_QUIT:
			return 0;
		}

		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}

	void GameCanvas::PIMPL::graphicsThreadProc()
	{
		// todo: setup OpenGL
		if (m_hOpenGL == NULL)
		{
			m_eGraphicsThreadState = GraphicsThreadState::Stopped;
			m_cv.notify_one();
			return;
		}

		std::unique_lock lock(m_mux);
		m_eGraphicsThreadState = GraphicsThreadState::Waiting;
		m_cv.notify_one();
		m_cv.wait(lock); // wait for run(), quit() or destructor
		lock.unlock();

		while (true)
		{
			lock.lock();
			if (!m_bRunning)
				break; // while --> unlock at end of function
			lock.unlock();

			// todo: graphics processing
			// NOTE: graphics processing = lock mutex, copy data, unlock mutex, process copied data
		}

		// todo: destroy OpenGL

		m_eGraphicsThreadState = GraphicsThreadState::Stopped;
	}

	void GameCanvas::PIMPL::doUpdate()
	{
		std::unique_lock lockBuf(m_muxBuffers[m_iCurrentLiveBuffer]);

		static std::chrono::system_clock::time_point tp2;
		tp2 = std::chrono::system_clock::now();
		static auto tp1 = tp2;

		m_oConfig.fnUpdate(m_oHandle, m_pBuffers_Live[m_iCurrentLiveBuffer],
			std::chrono::duration_cast<std::chrono::duration<double>>(tp2 - tp1).count());
		tp1 = tp2;

		// change buffer
		std::unique_lock lockBufChange(m_muxBufferChange);
		m_iCurrentLiveBuffer = ++m_iCurrentLiveBuffer % 2;
	}

}