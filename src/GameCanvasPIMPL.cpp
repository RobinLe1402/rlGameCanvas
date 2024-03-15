#include "private/GameCanvasPIMPL.hpp"
#include "private/Windows.hpp"
#include <rlGameCanvas/Definitions.h>

#include "private/OpenGL.hpp"

#include <gl/glext.h>

#pragma comment(lib, "Opengl32.lib")

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
			config.oInitialConfig.oResolution.x == 0 ||
			config.oInitialConfig.oResolution.y == 0 ||
			// initially maximized, but maximization is not available:
			(config.oInitialConfig.iMaximization == RL_GAMECANVAS_MAX_MAXIMIZE &&
				config.iMaximizeBtnAction != RL_GAMECANVAS_MAX_MAXIMIZE)
		)
			throw std::exception{ "Invalid startup configuration." };

		if (config.oInitialConfig.iPixelSize == 0)
			m_oConfig.oInitialConfig.iPixelSize = 1; // TODO: automatic pixel size


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



		int iWinX      = CW_USEDEFAULT;
		int iWinY      = CW_USEDEFAULT;
		int iWinWidth  = CW_USEDEFAULT;
		int iWinHeight = CW_USEDEFAULT;

		DWORD dwStyle;
		switch (m_oConfig.oInitialConfig.iMaximization)
		{
		case RL_GAMECANVAS_MAX_FULLSCREEN:
		{
			m_eWindowState = WindowState::Fullscreen;
			dwStyle        = WS_POPUP;

			iWinX = 0;
			iWinY = 0;

			const HMONITOR hMonitor = MonitorFromPoint({}, MONITOR_DEFAULTTOPRIMARY);
			MONITORINFO mi{ sizeof(mi) };
			if (!GetMonitorInfoW(hMonitor, &mi))
				ThrowWithLastError("Monitor info could not be retreived.");
			iWinWidth  = mi.rcMonitor.right - mi.rcMonitor.left + 1; // "+ 1" on purpose
			iWinHeight = mi.rcMonitor.bottom - mi.rcMonitor.top;
			break;
		}

		case RL_GAMECANVAS_MAX_MAXIMIZE:
			m_eWindowState = WindowState::Maximized;

			dwStyle        = (WS_OVERLAPPEDWINDOW & ~WS_SIZEBOX) | WS_MAXIMIZE;
			// todo: size calculation?
			break;

		case RL_GAMECANVAS_MAX_NONE:
			{
				m_eWindowState = WindowState::Restored;
				dwStyle        = WS_OVERLAPPEDWINDOW & ~WS_SIZEBOX;
				if (m_oConfig.iMaximizeBtnAction == RL_GAMECANVAS_MAX_NONE)
					dwStyle &= ~WS_MAXIMIZEBOX;

				HMONITOR hMonitor;

				const HWND hWndForeground = GetForegroundWindow();
				if (hWndForeground != NULL)
					hMonitor = MonitorFromWindow(hWndForeground, MONITOR_DEFAULTTONEAREST);
				else
					hMonitor = MonitorFromPoint({}, MONITOR_DEFAULTTOPRIMARY);

				MONITORINFO mi{ sizeof(mi) };
				if (GetMonitorInfoW(hMonitor, &mi))
				{
					const int iWorkWidth  = mi.rcWork.right  - mi.rcWork.left;
					const int iWorkHeight = mi.rcWork.bottom - mi.rcWork.top;

					RECT rcBorder{};
					AdjustWindowRect(&rcBorder, dwStyle, FALSE); // TODO: error handling

					iWinWidth =
						m_oConfig.oInitialConfig.oResolution.x *
						m_oConfig.oInitialConfig.iPixelSize + (rcBorder.right - rcBorder.left);
					iWinHeight =
						m_oConfig.oInitialConfig.oResolution.y *
						m_oConfig.oInitialConfig.iPixelSize + (rcBorder.bottom - rcBorder.top);

					iWinX = mi.rcWork.left + (iWorkWidth  / 2 - iWinWidth  / 2);
					iWinY = mi.rcWork.top  + (iWorkHeight / 2 - iWinHeight / 2);
				}
				break;
			}

		default:
			throw std::exception{ "Unknown initial maximization state." };
		}


		

		// windowed mode --> center on currently active monitor

		// try to create the window
		if (CreateWindowExW(
				NULL,                                                   // dwExStyle
				s_szCLASSNAME,                                          // lpClassName
				UTF8toWindowsUnicode(m_sWindowCaption.c_str()).c_str(), // lpWindowName
				dwStyle,                                                // dwStyle
				iWinX,                                                  // X
				iWinY,                                                  // Y
				iWinWidth,                                              // nWidth
				iWinHeight,                                             // nHeight
				NULL,                                                   // hWndParent
				NULL,                                                   // hMenu
				s_hInstance,                                            // hInstance
				this                                                    // lpParam
			) == NULL)
			ThrowWithLastError("Failed to create the window.");

		// for compiler: C687 - "could be zero"
		if (m_hWnd == NULL)
			throw std::exception{ "Failed to create the window: No handle." };

		// get client area size
		{
			RECT rcBorder{};
			AdjustWindowRect(&rcBorder, dwStyle, FALSE); // TODO: error handling?

			RECT rcWindow{};
			GetWindowRect(m_hWnd, &rcWindow); // TODO: error handling?

			m_oClientSize.x = (rcWindow.right  - rcBorder.right ) - (rcWindow.left - rcBorder.left);
			m_oClientSize.y = (rcWindow.bottom - rcBorder.bottom) - (rcWindow.top  - rcBorder.top );

			if (m_eWindowState == WindowState::Fullscreen)
				--m_oClientSize.x;



			// calculate drawing rectangle

			const auto  &res    = m_oConfig.oInitialConfig.oResolution;
			const double dRatio = (double)res.x / res.y;

			UInt iDisplayWidth  = 0;
			UInt iDisplayHeight = 0;

			// calculate the biggest possible size that keeps the aspect ratio
			{
				iDisplayWidth  = m_oClientSize.x;
				iDisplayHeight = UInt(iDisplayWidth * dRatio);

				if (iDisplayHeight > m_oClientSize.y)
				{
					iDisplayHeight = m_oClientSize.y;
					iDisplayWidth  = UInt(iDisplayHeight / dRatio);

					if (iDisplayWidth > m_oClientSize.x)
						iDisplayWidth = m_oClientSize.x;
				}
			}

			// client area smaller than canvas size --> downscale
			if (m_oClientSize.x < res.x || m_oClientSize.y < res.y)
			{
				m_iPixelSize = 1;
				// keep the pre-calculated size
			}

			// canvas fits into client area at least once
			else
			{
				m_iPixelSize = std::min(m_oClientSize.x / res.x, m_oClientSize.y / res.y);
				if (m_oConfig.bOversample &&
					m_oClientSize.x > res.x * m_iPixelSize &&
					m_oClientSize.y > res.y * m_iPixelSize)
				{
					++m_iPixelSize;
					// keep the pre-calculated size
				}
				else
				{
					iDisplayWidth  = res.x * m_iPixelSize;
					iDisplayHeight = res.y * m_iPixelSize;
				}
			}

			// calculate the actual drawing rectangle
			m_oDrawRect.iLeft   = (m_oClientSize.x - iDisplayWidth) / 2;
			m_oDrawRect.iRight  = m_oDrawRect.iLeft + iDisplayWidth;
			m_oDrawRect.iTop    = (m_oClientSize.y - iDisplayHeight) / 2;
			m_oDrawRect.iBottom = m_oDrawRect.iTop + iDisplayHeight;
		}


		std::unique_lock lock(m_mux);
		m_oGraphicsThread = std::thread(&GameCanvas::PIMPL::graphicsThreadProc, this);
		m_cv.wait(lock); // wait for the graphics thread to finish the attempt to setup OpenGL.

		if (m_hOpenGL == NULL) // OpenGL could not be initialized --> error, join thread now.
		{
			if (m_oGraphicsThread.joinable())
				m_oGraphicsThread.join();


			lock.unlock();
			DestroyWindow(m_hWnd);
			
			throw std::exception{ "Failed to initialize OpenGL." };
		}

		m_oConfig.fnCreateData(&m_pBuffer_Updating);
		m_oConfig.fnCreateData(&m_pBuffer_Shared);
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

			lock.lock();
			m_cv.wait(lock); // wait until graphics thread is ready for new data
			lock.unlock();

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
			m_hDC  = GetDC(m_hWnd);
			if (m_oConfig.fnOnMsg)
				m_oConfig.fnOnMsg(m_oHandle, RL_GAMECANVAS_MSG_CREATE, 0, 0);
			break;

		case WM_SIZE:
		{
			if (m_eGraphicsThreadState == GraphicsThreadState::NotStarted)
				break;

			// TODO: this could cause problems with fullscreen mode later.
			//       maybe reset m_eWindowState whenever entering/exiting fullscreen mode.
			switch (wParam)
			{
			case SIZE_MAXIMIZED:
				if (m_eWindowState == WindowState::Maximized)
					return 0;
				m_eWindowState = WindowState::Maximized;
				// TODO: handle maximization
				break;

			case SIZE_MINIMIZED:
				if (m_eWindowState == WindowState::Minimized)
					return 0;
				m_eWindowState = WindowState::Minimized;
				// TODO: handle minimization
				return 0;

			case SIZE_RESTORED:
				if (m_eWindowState == WindowState::Restored)
					return 0;
				m_eWindowState = WindowState::Restored;
				// TODO: handle window restoration
				break;

			default:
				return 0;
			}
			handleResize(LOWORD(lParam), HIWORD(lParam));
			break;
		}

		case WM_KILLFOCUS:
			if (m_oConfig.fnOnMsg)
				m_oConfig.fnOnMsg(m_oHandle, RL_GAMECANVAS_MSG_LOSEFOCUS, 0, 0);
			break;

		case WM_SETFOCUS:
			if (m_oConfig.fnOnMsg)
				m_oConfig.fnOnMsg(m_oHandle, RL_GAMECANVAS_MSG_GAINFOCUS, 0, 0);
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
		m_eGraphicsThreadState = GraphicsThreadState::Waiting;
		Pixel pxBG;
		{
			bool bSuccess = false;

			PIXELFORMATDESCRIPTOR pfd =
			{
				sizeof(PIXELFORMATDESCRIPTOR), 1,
				PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
				PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				PFD_MAIN_PLANE, 0, 0, 0, 0
			};
			int pf = ChoosePixelFormat(m_hDC, &pfd);
			if (!SetPixelFormat(m_hDC, pf, &pfd))
				goto lbEnd;

			m_hOpenGL = wglCreateContext(m_hDC);
			if (m_hOpenGL == NULL)
				goto lbEnd;
			if (!wglMakeCurrent(m_hDC, m_hOpenGL))
			{
				wglDeleteContext(m_hOpenGL);
				m_hOpenGL = 0;
				goto lbEnd;
			}

			glViewport(
				0,               // x
				0,               // y
				m_oClientSize.x, // width
				m_oClientSize.y  // height
			);

			glMatrixMode(GL_PROJECTION);

			pxBG = m_oConfig.oInitialConfig.pxBackgroundColor;
			glClearColor(
				pxBG.rgba.r / 255.0f,
				pxBG.rgba.g / 255.0f,
				pxBG.rgba.b / 255.0f,
				1.0f
			);

			glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			if (!m_oLayers.create(
				1 + m_oConfig.iExtraLayerCount,         // iLayerCount
				m_oConfig.oInitialConfig.oResolution.x, // iWidth
				m_oConfig.oInitialConfig.oResolution.y  // iHeight
			))
			{
				wglDeleteContext(m_hOpenGL);
				m_hOpenGL = 0;
				goto lbEnd;
			}

			bSuccess = true;

		lbEnd:
			if (!bSuccess)
			{
				m_eGraphicsThreadState = GraphicsThreadState::Stopped;
				m_cv.notify_one();
				return;
			}
		}

		m_upOpenGL = std::make_unique<OpenGL>();
#ifndef NDEBUG
		printf("> OpenGL Version String: \"%s\"\n", m_upOpenGL->versionStr().c_str());
		printf("> OpenGL framebuffers available: %s\n",
			m_upOpenGL->glGenFramebuffers != nullptr ? "Yes" : "No");
#endif // NDEBUG

		if (m_upOpenGL->glGenFramebuffers)
		{
			auto &gl = *m_upOpenGL;
			const auto &cfg = m_oConfig.oInitialConfig;

			gl.glGenFramebuffers(1, &m_iIntScaledBufferFBO);
			glGenTextures(1, &m_iIntScaledBufferTexture);

			glBindTexture(GL_TEXTURE_2D, m_iIntScaledBufferTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cfg.oResolution.x * m_iPixelSize,
				cfg.oResolution.y * m_iPixelSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		std::unique_lock lock(m_mux);
		m_eGraphicsThreadState = GraphicsThreadState::Waiting;
		m_cv.notify_one();
		m_cv.wait(lock); // wait for run(), quit() or destructor
		lock.unlock();

		auto oLayers     = std::make_unique<Layer[]>(size_t(1) + m_oConfig.iExtraLayerCount);
		auto oLayersCopy = std::make_unique<Layer[]>(size_t(1) + m_oConfig.iExtraLayerCount);

		for (size_t i = 0; i < m_oLayers.layerCount(); ++i)
		{
			oLayers[i].pData = reinterpret_cast<rlGameCanvas_Pixel *>(m_oLayers.scanline(i, 0));
		}
		const size_t iDataSize = sizeof(oLayers[0]) * m_oLayers.layerCount();

		while (true)
		{
			lock.lock();
			if (!m_bRunning)
				break; // while --> unlock at end of function
			lock.unlock();

			// copy live data
			{
				std::unique_lock lockBuf(m_muxBuffer);
				m_oConfig.fnCopyData(m_pBuffer_Shared, m_pBuffer_Drawing);
				lockBuf.unlock();
			}

			lock.lock();
			m_cv.notify_one(); // notify logic thread that the graphics thread is ready for new data
			lock.unlock();

			// update the canvas
			memcpy_s(oLayersCopy.get(), iDataSize, oLayers.get(), iDataSize);
			m_oConfig.fnDraw(m_oHandle, oLayersCopy.get(), (UInt)m_oLayers.layerCount(),
				m_pBuffer_Drawing);



			m_oLayers.uploadAll();

			// use FBO
			if (m_iIntScaledBufferFBO)
			{
				auto &gl = *m_upOpenGL;

				// set up rendering to texture
				gl.glBindFramebuffer(GL_FRAMEBUFFER, m_iIntScaledBufferFBO);
				glBindTexture(GL_TEXTURE_2D, m_iIntScaledBufferTexture);
				gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
					m_iIntScaledBufferTexture, 0);

				// check FBO completeness
				if (gl.glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
					// TODO: handle framebuffer error
					fprintf(stderr, "Framebuffer is not complete!\n");
				}

				// draw to texture
				glViewport(0, 0, m_oConfig.oInitialConfig.oResolution.x * m_iPixelSize,
					m_oConfig.oInitialConfig.oResolution.y * m_iPixelSize);
				glClear(GL_COLOR_BUFFER_BIT);
				for (size_t i = 0; i < m_oLayers.layerCount(); ++i)
				{
					glBindTexture(GL_TEXTURE_2D, m_oLayers.textureID(i));

					// draw texture (full width and height, but upside down)
					glBegin(GL_TRIANGLE_STRIP);
					{
						// first triangle
						//
						// SCREEN:  TEXTURE:
						//  1--3     2
						//  | /      |\
						//  |/       | \
						//  2        1--3

						glTexCoord2f(0.0, 1.0);  glVertex3f(-1.0f, -1.0f, 0.0f);
						glTexCoord2f(0.0, 0.0);  glVertex3f(-1.0f,  1.0f, 0.0f);
						glTexCoord2f(1.0, 1.0);  glVertex3f( 1.0f, -1.0f, 0.0f);


						// second triangle (sharing an edge with the first one)
						//
						// SCREEN:  TEXTURE:
						//     3     2--4
						//    /|      \ |
						//   / |       \|
						//  2--4        3

						glTexCoord2f(1.0, 0.0);  glVertex3f(1.0f, 1.0f, 0.0f);
					}
					glEnd();
				}

				// render to screen

				gl.glBindFramebuffer(GL_FRAMEBUFFER, 0); // bind default framebuffer (screen buffer)
				glViewport(
					0,               // x
					0,               // y
					m_oClientSize.x, // width
					m_oClientSize.y  // height
				);
				
				glPushMatrix();
				glLoadIdentity();
				glOrtho(0, m_oClientSize.x, m_oClientSize.y, 0, 0.0f, 1.0f);
				{
					glClear(GL_COLOR_BUFFER_BIT);
					glBindTexture(GL_TEXTURE_2D, m_iIntScaledBufferTexture);

					// draw texture
					glBegin(GL_TRIANGLE_STRIP);
					{
						// first triangle
						//
						// 2
						// |\
						// | \
						// 1--3

						// 1
						glTexCoord2f(0.0, 0.0);
						glVertex3i(m_oDrawRect.iLeft, m_oDrawRect.iBottom, 0);

						// 2
						glTexCoord2f(0.0, 1.0);
						glVertex3i(m_oDrawRect.iLeft, m_oDrawRect.iTop, 0);

						// 3
						glTexCoord2f(1.0, 0.0);
						glVertex3i(m_oDrawRect.iRight, m_oDrawRect.iBottom, 0);


						// second triangle (sharing an edge with the first one)
						//
						// 2--4
						//  \ |
						//   \|
						//    3

						// 4
						glTexCoord2f(1.0, 1.0);
						glVertex3i(m_oDrawRect.iRight, m_oDrawRect.iTop, 0);
					}
					glEnd();
				}
				glPopMatrix();
			}

			// draw directly
			else
			{
				glLoadIdentity();
				glOrtho(0, m_oClientSize.x, m_oClientSize.y, 0, 0.0f, 1.0f);

				glClear(GL_COLOR_BUFFER_BIT);
				for (size_t i = 0; i < m_oLayers.layerCount(); ++i)
				{
					glBindTexture(GL_TEXTURE_2D, m_oLayers.textureID(i));

					// draw texture (full width and height, but upside down)
					glBegin(GL_TRIANGLE_STRIP);
					{
						// first triangle
						//
						// SCREEN:  TEXTURE:
						//  1--3     2
						//  | /      |\
						//  |/       | \
						//  2        1--3

						// 1
						glTexCoord2f(0.0, 1.0);
						glVertex3i(m_oDrawRect.iLeft, m_oDrawRect.iBottom, 0);

						// 2
						glTexCoord2f(0.0, 0.0);
						glVertex3i(m_oDrawRect.iLeft, m_oDrawRect.iTop, 0);

						// 3
						glTexCoord2f(1.0, 1.0);
						glVertex3i(m_oDrawRect.iRight, m_oDrawRect.iBottom, 0);


						// second triangle (sharing an edge with the first one)
						//
						// SCREEN:  TEXTURE:
						//     3     2--4
						//    /|      \ |
						//   / |       \|
						//  2--4        3

						// 4
						glTexCoord2f(1.0, 0.0);
						glVertex3i(m_oDrawRect.iRight, m_oDrawRect.iTop, 0);
					}
					glEnd();
				}
			}



			SwapBuffers(m_hDC);
		}

		if (m_iIntScaledBufferFBO)
		{
			m_upOpenGL->glDeleteFramebuffers(1, &m_iIntScaledBufferFBO);
			glDeleteTextures(1, &m_iIntScaledBufferTexture);

			m_iIntScaledBufferFBO     = 0;
			m_iIntScaledBufferTexture = 0;
		}

		m_upOpenGL.release();
		m_oLayers.destroy();
		wglDeleteContext(m_hOpenGL);

		m_eGraphicsThreadState = GraphicsThreadState::Stopped;
		m_cv.notify_one();
	}

	void GameCanvas::PIMPL::doUpdate()
	{
		m_tp2 = std::chrono::system_clock::now();
		if (m_tp1 == decltype(m_tp2){}) // not initialized?
			m_tp1 = m_tp2;

		m_oConfig.fnUpdate(m_oHandle, m_pBuffer_Updating,
			std::chrono::duration_cast<std::chrono::duration<double>>(m_tp2 - m_tp1).count());
		m_tp1 = m_tp2;

		// copy to shared buffer
		std::unique_lock lockBuf(m_muxBuffer);
		m_oConfig.fnCopyData(m_pBuffer_Updating, m_pBuffer_Shared);
	}

	void GameCanvas::PIMPL::handleResize(unsigned iClientWidth, unsigned iClientHeight)
	{
#ifndef NDEBUG
		printf("> Resize: %u x %u pixels\n", iClientWidth, iClientHeight);
#endif // NDEBUG
	}

}