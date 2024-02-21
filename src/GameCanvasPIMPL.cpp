#include "private/GameCanvasPIMPL.hpp"

namespace rlGameCanvasLib
{

	std::map<HWND, GameCanvas::PIMPL*> GameCanvas::PIMPL::s_oInstances;

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


	GameCanvas::PIMPL::PIMPL(const StartupConfig &config) : m_oConfig(config)
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
			throw std::exception{ "Invalid startup configuration" };


		// initialize the window caption string
		if (config.szWindowCaption == nullptr) // default text
		{
#ifndef RLGAMECANVAS_NO_CHAR8
			m_sWindowCaption = u8"rlGameCanvas";
#else
			m_sWindowCaption = "rlGameCanvas";
#endif
		}
		else // custom text
		{
			m_sWindowCaption = config.szWindowCaption;

			// pointer might be invalid after constructor finishes, so it's safer to just set it to
			// nullptr and to use m_sWindowCaption instead.
			m_oConfig.szWindowCaption = nullptr;
		}


		std::unique_lock<std::mutex> lock(m_mux);
		m_oGraphicsThread = std::thread(&GameCanvas::PIMPL::graphicsThreadProc, this);
		m_cv.wait(lock); // wait for the graphics thread to finish the attempt to create the window.

		if (m_hWnd == NULL) // window was not created --> error, join thread now.
		{
			if (m_oGraphicsThread.joinable())
				m_oGraphicsThread.join();
			
			lock.unlock();
			throw std::exception{ "Failed to create window" };
		}
	}

	GameCanvas::PIMPL::~PIMPL() { quit(); }

	bool GameCanvas::PIMPL::run()
	{
		std::unique_lock<std::mutex> lock(m_mux);
		if (m_eGraphicsThreadState != GraphicsThreadState::Waiting)
			return false;

		m_oMainThreadID = std::this_thread::get_id();

		m_cv.notify_one(); // tell graphics thread to start working
		lock.unlock();

		logicThreadProc(); // run game logic
		
		m_oMainThreadID = {};

		if (m_oGraphicsThread.joinable())
			m_oGraphicsThread.join();

		return true;
	}

	void GameCanvas::PIMPL::quit()
	{
		if (m_eGraphicsThreadState != GraphicsThreadState::Running)
			return;

		std::unique_lock<std::mutex> lock(m_mux);
		m_bStopRequested = true;
	}

	bool GameCanvas::PIMPL::updateConfig(const Config &oConfig, UInt iFlags)
	{
		// only calls from the main thread currently running game logic are accepted.
		if (std::this_thread::get_id() != m_oMainThreadID)
			return false;

		std::unique_lock<std::mutex> lock(m_muxBetweenFrames);
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
		// todo

		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}

	void GameCanvas::PIMPL::graphicsThreadProc()
	{
		// todo: create window, initial graphics setup
		if (m_hWnd == NULL)
		{
			std::unique_lock<std::mutex> lock(m_mux);
			m_eGraphicsThreadState = GraphicsThreadState::Stopped;
			m_cv.notify_one(); // notify main thread that initialization is done
			return;
		}

		std::unique_lock<std::mutex> lock(m_mux);
		m_eGraphicsThreadState = GraphicsThreadState::Waiting;
		m_cv.notify_one();
		m_cv.wait(lock); // wait for run(), quit() or destructor
		lock.unlock();

		if (m_bRunning)
		{
			// todo: Windows message loop with peek + graphics processing
			// NOTE: graphics processing = lock mutex, copy data, unlock mutex, process copied data
		}

		m_eGraphicsThreadState = GraphicsThreadState::Stopped;
	}

	void GameCanvas::PIMPL::logicThreadProc()
	{
		// todo: core game loop framework, call to update callback (surrounded with lock)
	}

}