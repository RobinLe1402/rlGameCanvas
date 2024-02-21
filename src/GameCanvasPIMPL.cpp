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
		if (config.fnDraw        == nullptr ||
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
		m_oGraphicsThread = std::thread(graphicsThreadProc, this);
		m_cv.wait(lock); // wait for the graphics thread to finish the attempt to create the window.

		if (m_hWnd == NULL) // window was not created --> error, join thread now.
		{
			if (m_oGraphicsThread.joinable())
				m_oGraphicsThread.join();
			
			throw std::exception{ "Failed to create window" };
		}
	}

	GameCanvas::PIMPL::~PIMPL() { quit(); }

	bool GameCanvas::PIMPL::run()
	{
		if (m_eGraphicsThreadState != GraphicsThreadState::Waiting)
			return false;

		// todo
		return false;
	}

	void GameCanvas::PIMPL::quit()
	{
		if (m_eGraphicsThreadState != GraphicsThreadState::Running)
			return;

		// todo
	}

	bool GameCanvas::PIMPL::updateConfig(const Config &oConfig, UInt iFlags)
	{
		if (m_eGraphicsThreadState == GraphicsThreadState::Stopped)
			return false;

		// todo
		return false;
	}

	LRESULT GameCanvas::PIMPL::localWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		// todo

		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}

	void GameCanvas::PIMPL::graphicsThreadProc()
	{
		// todo

		m_eGraphicsThreadState = GraphicsThreadState::Stopped;
	}

	void GameCanvas::PIMPL::logicThreadProc()
	{
		// todo

		if (m_oGraphicsThread.joinable())
			m_oGraphicsThread.join();
	}

}