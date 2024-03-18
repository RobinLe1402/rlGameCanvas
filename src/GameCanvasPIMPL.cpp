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

		bool operator!=(const Resolution &o1, const Resolution &o2)
		{
			return memcmp(&o1, &o2, sizeof(Resolution)) != 0;
		}

		bool operator!=(const Config &o1, const Config &o2)
		{
			return memcmp(&o1, &o2, sizeof(Config)) != 0;
		}

		DWORD GenWindowStyle(UInt iMaximization, bool bMaxBtnAvailable)
		{
			DWORD dwStyle = 0;

			switch (iMaximization)
			{
			case RL_GAMECANVAS_MAX_FULLSCREEN:
				dwStyle = WS_POPUP;
				break;

			case RL_GAMECANVAS_MAX_MAXIMIZE:
				dwStyle = (WS_OVERLAPPEDWINDOW & ~WS_SIZEBOX) | WS_MAXIMIZE;
				break;

			case RL_GAMECANVAS_MAX_NONE:
				dwStyle = WS_OVERLAPPEDWINDOW & ~WS_SIZEBOX;
				if (!bMaxBtnAvailable)
					dwStyle &= ~WS_MAXIMIZEBOX;
				break;

			default:
				throw std::exception{};
			}

			return dwStyle;
		}

		struct WindowConfig
		{
			DWORD      dwStyle;
			Rect       rcWindow;
			Resolution oClientSize;
		};

		void GetFullscreenCoordAndSize(RECT &rcWindow, Resolution &oClientSize)
		{
			const HMONITOR hMonitor = MonitorFromPoint({}, MONITOR_DEFAULTTOPRIMARY);
			MONITORINFO mi{ sizeof(mi) };
			if (!GetMonitorInfoW(hMonitor, &mi))
				ThrowWithLastError("Monitor info could not be retreived.");

			rcWindow = mi.rcMonitor;
			oClientSize.x = rcWindow.right  - rcWindow.left;
			oClientSize.y = rcWindow.bottom - rcWindow.top;
			++rcWindow.right; // to avoid implicit fullscreen mode
		}

		void GetRestoredCoordAndSize(DWORD dwStyle, HWND hWndOnDestMonitor,
			const Resolution &oRes, UInt &iPixelSize, RECT &rcWindow_Necessary)
		{
			if (iPixelSize == 0)
			{
				// todo: automatically determine pixel size
				iPixelSize = 1;
			}


			HMONITOR hMonitor;

			if (hWndOnDestMonitor != NULL)
				hMonitor = MonitorFromWindow(hWndOnDestMonitor, MONITOR_DEFAULTTONEAREST);
			else
				hMonitor = MonitorFromPoint({}, MONITOR_DEFAULTTOPRIMARY);

			MONITORINFO mi{ sizeof(mi) };
			if (GetMonitorInfoW(hMonitor, &mi))
			{
				const int iWorkWidth  = mi.rcWork.right  - mi.rcWork.left;
				const int iWorkHeight = mi.rcWork.bottom - mi.rcWork.top;

				RECT rcBorder{};
				AdjustWindowRect(&rcBorder, dwStyle, FALSE); // TODO: error handling

				const int iWinWidth =
					oRes.x * iPixelSize + (rcBorder.right - rcBorder.left);
				const int iWinHeight =
					oRes.y * iPixelSize + (rcBorder.bottom - rcBorder.top);

				rcWindow_Necessary.left   = mi.rcWork.left + (iWorkWidth  / 2 - iWinWidth  / 2);
				rcWindow_Necessary.top    = mi.rcWork.top  + (iWorkHeight / 2 - iWinHeight / 2);
				rcWindow_Necessary.right  = rcWindow_Necessary.left + iWinWidth;
				rcWindow_Necessary.bottom = rcWindow_Necessary.top  + iWinHeight;
			}
		}

		Resolution GetActualClientSize(HWND hWnd)
		{
			const DWORD dwStyle = (DWORD)GetWindowLongW(hWnd, GWL_STYLE);

			RECT rcBorder = {};
			AdjustWindowRect(&rcBorder, dwStyle, FALSE); // TODO: error handling?

			RECT rcWindow = {};
			GetWindowRect(hWnd, &rcWindow); // TODO: error handling?

			Resolution result =
			{
				.x = UInt((rcWindow.right  - rcBorder.right)  - (rcWindow.left - rcBorder.left)),
				.y = UInt((rcWindow.bottom - rcBorder.bottom) - (rcWindow.top  - rcBorder.top ))
			};
			return result;
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
		m_oConfig(config), m_oHandle(oHandle), m_iPixelSize(config.oInitialConfig.iPixelSize)
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

		if (config.hIconBig && !config.hIconSmall)
			m_oConfig.hIconSmall = config.hIconBig;
		else if (config.hIconSmall && !config.hIconBig)
			m_oConfig.hIconBig = config.hIconSmall;


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

		const DWORD dwStyle = GenWindowStyle(m_oConfig.oInitialConfig.iMaximization,
			m_oConfig.iMaximizeBtnAction != RL_GAMECANVAS_MAX_NONE);
		switch (m_oConfig.oInitialConfig.iMaximization)
		{
		case RL_GAMECANVAS_MAX_FULLSCREEN:
		{
			RECT rcWindow = {};
			GetFullscreenCoordAndSize(rcWindow, m_oClientSize);
			iWinX      = rcWindow.left;
			iWinY      = rcWindow.top;
			iWinWidth  = rcWindow.right  - rcWindow.left;
			iWinHeight = rcWindow.bottom - rcWindow.top;
			break;
		}

		case RL_GAMECANVAS_MAX_MAXIMIZE:
			// size is not calculated, but read after creation
			break;

		case RL_GAMECANVAS_MAX_NONE:
		{
			RECT rcWindow = {};
			GetRestoredCoordAndSize(dwStyle, GetForegroundWindow(),
				m_oConfig.oInitialConfig.oResolution, m_iPixelSize, rcWindow);
			iWinX      = rcWindow.left;
			iWinY      = rcWindow.top;
			iWinWidth  = rcWindow.right  - rcWindow.left;
			iWinHeight = rcWindow.bottom - rcWindow.top;
		}
			break;

		default:
			throw std::exception{ "Unknown initial maximization state." };
		}


		

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

		if (m_oConfig.hIconSmall != NULL)
			SendMessage(m_hWnd, WM_SETICON, ICON_SMALL,
				reinterpret_cast<LPARAM>(m_oConfig.hIconSmall));

		if (m_oConfig.hIconBig != NULL)
			SendMessage(m_hWnd, WM_SETICON, ICON_BIG,
				reinterpret_cast<LPARAM>(m_oConfig.hIconBig));

		// get actual client area size
		if (m_oConfig.oInitialConfig.iMaximization != RL_GAMECANVAS_MAX_FULLSCREEN)
			m_oClientSize = GetActualClientSize(m_hWnd);



		// set up OpenGL

		try
		{
			PIXELFORMATDESCRIPTOR pfd =
			{
				sizeof(PIXELFORMATDESCRIPTOR), 1,
				PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
				PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				PFD_MAIN_PLANE, 0, 0, 0, 0
			};
			int pf = ChoosePixelFormat(m_hDC, &pfd);
			if (!SetPixelFormat(m_hDC, pf, &pfd))
				ThrowWithLastError("Call to SetPixelFormat failed.");

			m_hOpenGL = wglCreateContext(m_hDC);
			if (m_hOpenGL == NULL)
				ThrowWithLastError("Call to SetPixelFormat failed.");
			try
			{
				if (!wglMakeCurrent(m_hDC, m_hOpenGL))
					ThrowWithLastError("Call to wglMakeCurrent failed.");

				glViewport(
					0,               // x
					0,               // y
					m_oClientSize.x, // width
					m_oClientSize.y  // height
				);

				glMatrixMode(GL_PROJECTION);

				const Pixel pxBG = m_oConfig.oInitialConfig.pxBackgroundColor;
				glClearColor(
					pxBG.rgba.r / 255.0f,
					pxBG.rgba.g / 255.0f,
					pxBG.rgba.b / 255.0f,
					1.0f
				);

				glEnable(GL_TEXTURE_2D);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
			catch (...)
			{
				wglMakeCurrent(NULL, NULL);
				wglDeleteContext(m_hOpenGL);
				throw;
			}
		}
		catch (...)
		{
			DestroyWindow(m_hWnd);
			throw;
		}
		



		m_upOpenGL = std::make_unique<OpenGL>();
		m_bFBO     = m_upOpenGL->glGenFramebuffers;
#ifndef NDEBUG
		printf("> OpenGL Version String: \"%s\"\n", m_upOpenGL->versionStr().c_str());
		printf("> OpenGL framebuffers available: %s\n", m_bFBO ? "Yes" : "No");
#endif // NDEBUG

		if (m_bFBO)
		{
			auto &gl = *m_upOpenGL;
			const auto &cfg = m_oConfig.oInitialConfig;

			gl.glGenFramebuffers(1, &m_iIntScaledBufferFBO);
			glGenTextures(1, &m_iIntScaledBufferTexture);

			glBindTexture(GL_TEXTURE_2D, m_iIntScaledBufferTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		m_oConfig.fnCreateData(&m_pBuffer_Updating);
		m_oConfig.fnCreateData(&m_pBuffer_Drawing);

		const size_t iLayerCount = (size_t)1 + m_oConfig.iExtraLayerCount;
		m_oLayersForCallback      = std::make_unique<Layer[]>(iLayerCount);
		m_oLayersForCallback_Copy = std::make_unique<Layer[]>(iLayerCount);


		setResolution(m_oConfig.oInitialConfig.oResolution, true);
	}

	GameCanvas::PIMPL::~PIMPL() { quit(); }

	bool GameCanvas::PIMPL::run()
	{
		m_oMainThreadID = std::this_thread::get_id();

		m_bRunning = true;
		doUpdate(false, true); // first call to update() prepares the very first frame
		ShowWindow(m_hWnd, SW_SHOW);
		renderFrame(); // draw very first frame immediately
		SetForegroundWindow(m_hWnd);

		wglMakeCurrent(NULL, NULL); // give up control over OpenGL

		// TODO: do in WM_CREATE or WM_SHOW instead?
		m_eGraphicsThreadState = GraphicsThreadState::Running;
		m_oGraphicsThread = std::thread(&GameCanvas::PIMPL::graphicsThreadProc, this);

		MSG msg{};
		while (true)
		{
			while (PeekMessageW(&msg, m_hWnd, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);

				if (!m_bRunning)
					goto lbClose;

				if (m_bMinimized)
				{
					std::unique_lock lock(m_muxAppState);
					m_bMinimized_Waiting = true;
					m_cvAppState.wait(lock);
					while (m_bMinimized)
					{
						if (GetMessageW(&msg, m_hWnd, 0, 0) == 0)
							break; // while
						TranslateMessage(&msg);
						DispatchMessageW(&msg);
					}
					m_bMinimized         = false;
					m_bMinimized_Waiting = false;
					m_cvAppState.notify_one(); // continue graphics thread

					if (!m_bRunning)
						goto lbClose;
				}
			}

			if (!doUpdate(true, false))
				resumeGraphicsThread();
		}
	lbClose:
				
		m_oMainThreadID = {};

		// TODO: might cause problems when closing the window. maybe move to WM_DESTROY.

		if (m_oGraphicsThread.joinable())
			m_oGraphicsThread.join();

		wglMakeCurrent(m_hDC, m_hOpenGL);


		// destroy OpenGL

		if (m_bFBO)
		{
			m_upOpenGL->glDeleteFramebuffers(1, &m_iIntScaledBufferFBO);
			glDeleteTextures(1, &m_iIntScaledBufferTexture);

			m_iIntScaledBufferFBO     = 0;
			m_iIntScaledBufferTexture = 0;
		}

		m_upOpenGL.release();
		m_oLayers.destroy();
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(m_hOpenGL);



		return true;
	}

	void GameCanvas::PIMPL::quit()
	{
		if (m_eGraphicsThreadState != GraphicsThreadState::Running)
			return;

		std::unique_lock lock(m_muxAppState);
		m_bStopRequested = true;
	}

	LRESULT GameCanvas::PIMPL::localWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (!m_bMinimized && m_oConfig.fnOnWinMsg)
			m_oConfig.fnOnWinMsg(m_oHandle, uMsg, wParam, lParam);

		switch (uMsg)
		{
		case WM_CREATE:
			m_hWnd = hWnd;
			m_hDC  = GetDC(m_hWnd);
			sendMessage(RL_GAMECANVAS_MSG_CREATE, 0, 0);
			break;

		case WM_SYSCOMMAND:
			if (wParam == SC_MAXIMIZE)
			{
				if (m_oConfig.iMaximizeBtnAction == RL_GAMECANVAS_MAX_FULLSCREEN)
				{
					m_oConfig.oInitialConfig.iMaximization = RL_GAMECANVAS_MAX_FULLSCREEN;
					m_bMaxFullscreen = true;

					RECT rcWindow = {};
					Resolution oClientSize = {};
					GetFullscreenCoordAndSize(rcWindow, oClientSize);

					m_bIgnoreResize = true;
					ShowWindow(m_hWnd, SW_HIDE);
					SetWindowLongW(m_hWnd, GWL_STYLE, GenWindowStyle(RL_GAMECANVAS_MAX_FULLSCREEN, false));
					m_bIgnoreResize = false;
					SetWindowPos(
							m_hWnd,                          // hWnd
							NULL,                            // hWndInsertAfter
							rcWindow.left,                   // X
							rcWindow.top,                    // Y
							rcWindow.right  - rcWindow.left, // cx
							rcWindow.bottom - rcWindow.top,  // cy
							SWP_NOZORDER | SWP_SHOWWINDOW    // uFlags
					);

					return 0;
				}
			}
			break;

		case WM_SIZE:
		{
			if (m_eGraphicsThreadState == GraphicsThreadState::NotStarted || m_bIgnoreResize)
				break;

			const UInt iPrevMax = m_oConfig.oInitialConfig.iMaximization;

			switch (wParam)
			{
			case SIZE_MAXIMIZED:
				m_oConfig.oInitialConfig.iMaximization = RL_GAMECANVAS_MAX_MAXIMIZE;
				break;

			case SIZE_MINIMIZED:
				if (m_bMinimized)
					return 0;
				m_bMinimized = true;
				sendMessage(RL_GAMECANVAS_MSG_MINIMIZE, 1, 0);
				return 0;

			case SIZE_RESTORED:
				if (m_bMaxFullscreen)
				{
					m_bMaxFullscreen = false;
					break;
				}
				if (m_oConfig.oInitialConfig.iMaximization != RL_GAMECANVAS_MAX_NONE)
				{
					m_oConfig.oInitialConfig.iMaximization = RL_GAMECANVAS_MAX_NONE;

					const DWORD dwStyle = GetWindowLongW(m_hWnd, GWL_STYLE);
					RECT rcWindow;
					UInt iPixelSize = m_oConfig.oInitialConfig.iPixelSize;
					GetRestoredCoordAndSize(dwStyle, m_hWnd, m_oConfig.oInitialConfig.oResolution,
						iPixelSize, rcWindow);
					if (
						LOWORD(lParam) != iPixelSize * m_oConfig.oInitialConfig.oResolution.x ||
						HIWORD(lParam) != iPixelSize * m_oConfig.oInitialConfig.oResolution.y
					)
					{
						SetWindowPos(
							m_hWnd,                          // hWnd
							NULL,                            // hWndInsertAfter
							rcWindow.left,                   // X
							rcWindow.top,                    // Y
							rcWindow.right  - rcWindow.left, // cx
							rcWindow.bottom - rcWindow.top,  // cy
							SWP_NOZORDER                     // uFlags
						);
						return 0;
					}
				}
				else
					m_oConfig.oInitialConfig.iMaximization = RL_GAMECANVAS_MAX_NONE;
				break;

			default:
				return 0;
			}
			if (m_bMinimized && wParam != SIZE_MINIMIZED)
			{
				m_bMinimized = false;
				sendMessage(RL_GAMECANVAS_MSG_MINIMIZE, 0, 0);
			}
			else
				handleResize(LOWORD(lParam), HIWORD(lParam), iPrevMax,
					m_oConfig.oInitialConfig.iMaximization);
			return 0;
		}

		case WM_KILLFOCUS:
			sendMessage(RL_GAMECANVAS_MSG_LOSEFOCUS, 0, 0);
			break;

		case WM_SETFOCUS:
			sendMessage(RL_GAMECANVAS_MSG_GAINFOCUS, 0, 0);
			break;

		case WM_CLOSE:
		{
			std::unique_lock lock(m_muxAppState);
			m_bRunning = false;
			m_cvNextFrame.notify_one();
			if (m_eGraphicsThreadState == GraphicsThreadState::Running)
				m_cvAppState.wait(lock);
		}
			DestroyWindow(m_hWnd);
			return 0;

		case WM_DESTROY:
			sendMessage(RL_GAMECANVAS_MSG_DESTROY, 0, 0);

			PostQuitMessage(0);
			return 0;

		case WM_QUIT:
			return 0;
		}

		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}

	void GameCanvas::PIMPL::graphicsThreadProc()
	{
		wglMakeCurrent(m_hDC, m_hOpenGL);

		while (true)
		{
			// check if still running
			{
				std::unique_lock lock(m_muxAppState);
				if (!m_bRunning)
					break; // while
				if (m_bMinimized_Waiting)
				{
					// notify logic thread that minimization state was entered
					m_cvAppState.notify_one();
					// wait for logic thread to wake up graphics thread
					m_cvAppState.wait(lock);
				}
			}

			renderFrame();
		}

		std::unique_lock lock(m_muxAppState);
		m_eGraphicsThreadState = GraphicsThreadState::Stopped;
		wglMakeCurrent(NULL, NULL);
		m_cvAppState.notify_one(); // notify main thread that the graphics thread is now terminating
	}

	void GameCanvas::PIMPL::renderFrame()
	{
		// sync up with logic thread
		if (std::this_thread::get_id() == m_oGraphicsThread.get_id())
		{
			bool bOpenGLUnderControl = true;

			std::unique_lock lock(m_muxNextFrame);
			m_eGraphicsThreadState = GraphicsThreadState::Waiting;
			if (m_bOpenGLRequest)
			{
				bOpenGLUnderControl = false;
				wglMakeCurrent(NULL, NULL);
			}
			m_cvNextFrame.notify_one();
			m_cvNextFrame.wait(lock);
			
			std::unique_lock lock_state(m_muxAppState);
			if (!m_bRunning) // app is being shut down
				return; // cancel current frame rendering
			lock_state.unlock();

			if (bOpenGLUnderControl)
			{
				bool bNewOpenGLRequest = m_bOpenGLRequest;
				while (m_bOpenGLRequest)
				{
					if (bNewOpenGLRequest)
					{
						bOpenGLUnderControl = false;
						wglMakeCurrent(NULL, NULL);
						bNewOpenGLRequest = false;
					}
					m_cvNextFrame.notify_one();
					m_cvNextFrame.wait(lock);
					lock_state.lock();
					if (!m_bRunning)
						return;
				}
			}

			if (!bOpenGLUnderControl)
			{
				// regain control over OpenGL
				if (!wglMakeCurrent(m_hDC, m_hOpenGL))
					fprintf(stderr, "Graphics thread couldn't regain control over OpenGL!\n");
				// TODO: further error handling?
			}
			

			m_eGraphicsThreadState = GraphicsThreadState::Running;
		}

		// update the canvas
		memcpy_s(
			m_oLayersForCallback_Copy.get(), m_iLayersForCallback_Size,
			m_oLayersForCallback     .get(), m_iLayersForCallback_Size
		);
		m_oConfig.fnDraw(m_oHandle, m_oConfig.oInitialConfig.oResolution,
			m_oLayersForCallback_Copy.get(), (UInt)m_oLayers.layerCount(), m_pBuffer_Drawing);



		m_oLayers.uploadAll();

		// use FBO
		if (m_bFBO)
		{
			auto &gl = *m_upOpenGL;

			// set up rendering to texture
			gl.glBindFramebuffer(GL_FRAMEBUFFER, m_iIntScaledBufferFBO);
			glBindTexture(GL_TEXTURE_2D, m_iIntScaledBufferTexture);
			gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
				m_iIntScaledBufferTexture, 0);

			// check FBO completeness
			const auto iFramebbufferStatus = gl.glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (iFramebbufferStatus != GL_FRAMEBUFFER_COMPLETE) {
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
					glTexCoord2f(0.0, 0.0);  glVertex3f(-1.0f, 1.0f, 0.0f);
					glTexCoord2f(1.0, 1.0);  glVertex3f(1.0f, -1.0f, 0.0f);


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

	// This function is supposed to be called from another thread than the graphics thread.
	// It waits until the graphics thread is waiting.
	// For optimization purposes, no check is done if the graphics thread has already given up
	// control over OpenGL.
	void GameCanvas::PIMPL::waitForGraphicsThread(bool bRequestOpenGL)
	{
		std::unique_lock lock(m_muxNextFrame);
		m_bOpenGLRequest = bRequestOpenGL;

		switch (m_eGraphicsThreadState)
		{
		case GraphicsThreadState::Waiting:
			if (bRequestOpenGL)
			{
				m_cvNextFrame.notify_one(); // allow graphics thread to give up control over OpenGL
				m_cvNextFrame.wait(lock);   // wait for the above to finish
			}
			break;

		case GraphicsThreadState::Running:
			m_cvNextFrame.wait(lock); // wait for the graphics thread to finish drawing
			break;

		default:
			throw std::exception{};
		}

		if (bRequestOpenGL)
			wglMakeCurrent(m_hDC, m_hOpenGL); // todo: error handling?
	}

	// This function shall only be called after calling waitForGraphicsThread.
	void GameCanvas::PIMPL::resumeGraphicsThread()
	{
		m_oConfig.fnCopyData(m_pBuffer_Updating, m_pBuffer_Drawing);

		if (m_bOpenGLRequest)
		{
			wglMakeCurrent(NULL, NULL);
			m_bOpenGLRequest = false;
		}
		m_cvNextFrame.notify_one();
	}

	bool GameCanvas::PIMPL::doUpdate(bool bConfigChangable, bool bRepaint)
	{
		auto &oConfig = m_oConfig.oInitialConfig;
		Config oConfig_Copy = oConfig;

		UInt iFlags = 0;
		if (!bConfigChangable)
			iFlags |= RL_GAMECANVAS_UPD_READONLYCONFIG;
		if (bRepaint)
			iFlags |= RL_GAMECANVAS_UPD_REPAINT;

		m_tp2 = std::chrono::system_clock::now();
		if (m_tp1 == decltype(m_tp2){}) // not initialized?
			m_tp1 = m_tp2;

		m_bRunningUpdate = true;
		m_oConfig.fnUpdate(m_oHandle, m_pBuffer_Updating,
			std::chrono::duration_cast<std::chrono::duration<double>>(m_tp2 - m_tp1).count(),
			&oConfig_Copy, iFlags);
		m_bRunningUpdate = false;
		m_tp1 = m_tp2;

		if (
			m_oConfig.iMaximizeBtnAction != RL_GAMECANVAS_MAX_MAXIMIZE &&
			oConfig_Copy.iMaximization   == RL_GAMECANVAS_MAX_MAXIMIZE
			)
			oConfig_Copy = oConfig; // illegal change --> ignore request
		else
		{
			if (oConfig_Copy.oResolution.x == 0)
				oConfig_Copy.oResolution.x = oConfig.oResolution.x;
			if (oConfig_Copy.oResolution.y == 0)
				oConfig_Copy.oResolution.y = oConfig.oResolution.y;
			oConfig_Copy.pxBackgroundColor =
				RLGAMECANVAS_MAKEPIXELOPAQUE(oConfig_Copy.pxBackgroundColor);
		}

		bool bRendered = false;
		if (bConfigChangable && oConfig_Copy != oConfig)
		{
			waitForGraphicsThread(true);

			m_iPixelSize = oConfig_Copy.iPixelSize;

			if (oConfig_Copy.iMaximization != oConfig.iMaximization)
				setMaximization(oConfig_Copy.iMaximization, oConfig_Copy.oResolution, m_iPixelSize);

			if (oConfig_Copy.pxBackgroundColor != oConfig.pxBackgroundColor)
			{
				if (m_bFBO)
					m_upOpenGL->glBindFramebuffer(GL_FRAMEBUFFER, m_iIntScaledBufferFBO);

				const Pixel px = oConfig_Copy.pxBackgroundColor;
				glClearColor(
					px.rgba.r / 255.0f,
					px.rgba.g / 255.0f,
					px.rgba.b / 255.0f,
					1.0f
				);
			}

			const bool bNewRes = oConfig_Copy.oResolution != oConfig.oResolution;
			setResolution(oConfig_Copy.oResolution, bNewRes);

			oConfig = oConfig_Copy;

			doUpdate(false, bNewRes);
			renderFrame();
			bRendered = true;
		}

		return bRendered;
	}

	void GameCanvas::PIMPL::handleResize(unsigned iClientWidth, unsigned iClientHeight,
		UInt iPreviousMaximization, UInt iNewMaximization)
	{
#ifndef NDEBUG
		printf("> Resize: %u x %u pixels\n", iClientWidth, iClientHeight);
#endif // NDEBUG

		waitForGraphicsThread(true);

		const Resolution oOldClientSize = m_oClientSize;
		Resolution oNewRes = m_oConfig.oInitialConfig.oResolution;

		bool bResize = false;
		if (m_oConfig.fnOnMsg)
		{
			ResizeInputParams rip =
			{
				.oOldClientSize = oOldClientSize,
				.oNewClientSize = { .x = iClientWidth, .y = iClientHeight },
				.iOldMaximization = iPreviousMaximization,
				.iNewMaximization = iNewMaximization
			};
			const Resolution oOldRes = m_oConfig.oInitialConfig.oResolution;
			ResizeOutputParams rop =
			{
				.oResolution = oOldRes
			};

			m_oConfig.fnOnMsg(
				m_oHandle,                        // canvas
				RL_GAMECANVAS_MSG_RESIZE,         // iMsg
				reinterpret_cast<MsgParam>(&rip), // iParam1
				reinterpret_cast<MsgParam>(&rop)  // iParam2
			);

			if (rop.oResolution.x == 0)
				rop.oResolution.x = oOldRes.x;
			if (rop.oResolution.y == 0)
				rop.oResolution.y = oOldRes.y;

			bResize = rop.oResolution != oOldRes;
			if (bResize && iNewMaximization == RL_GAMECANVAS_MAX_NONE)
			{
				RECT rcWindow;
				m_iPixelSize = m_oConfig.oInitialConfig.iPixelSize;
				GetRestoredCoordAndSize(GetWindowLong(m_hWnd, GWL_STYLE), m_hWnd, rop.oResolution,
					m_iPixelSize, rcWindow);

				const int iWidth  = rcWindow.right  - rcWindow.left;
				const int iHeight = rcWindow.bottom - rcWindow.top;

				m_bIgnoreResize = true;
				SetWindowPos(
					m_hWnd,        // hWnd
					NULL,          // hWndInsertAfter
					rcWindow.left, // X
					rcWindow.top,  // Y
					iWidth,        // cx
					iHeight,       // cy
					SWP_NOZORDER   // uFlags
				);
				m_bIgnoreResize = false;

				iClientWidth  = m_iPixelSize * rop.oResolution.x;
				iClientHeight = m_iPixelSize * rop.oResolution.y;
			}
			oNewRes = rop.oResolution;
		}

		m_oClientSize.x = iClientWidth;
		m_oClientSize.y = iClientHeight;

		if (!m_bFBO)
			glViewport(0, 0, iClientWidth, iClientHeight);

		

		setResolution(oNewRes, bResize);
		doUpdate(false, true);
		m_oConfig.fnCopyData(m_pBuffer_Updating, m_pBuffer_Drawing);
		renderFrame();
	}

	// WARNING:
	// This function should only be called if...
	// * the calling thread has control over OpenGL
	// * there is currently no rendering in progress
	// * m_oClientSize is up to date
	// WHAT THIS FUNCTION DOES
	// * recalculate m_iPixelSize and m_oDrawRect
	// WHAT THIS FUNCTION DOESN'T DO
	// * draw to the screen
	void GameCanvas::PIMPL::setResolution(const Resolution &oNewRes, bool bResize)
	{
		if (bResize)
		{
			m_oConfig.oInitialConfig.oResolution = oNewRes;

			if (!m_oLayers.create(
				1 + m_oConfig.iExtraLayerCount, // iLayerCount
				oNewRes.x,                      // iWidth
				oNewRes.y                       // iHeight
			))
			{
				// TODO: error handling?
				fprintf(stderr, "Error resizing the canvas!\n");
				return;
			}

			for (size_t i = 0; i < m_oLayers.layerCount(); ++i)
			{
				auto &oLayer    = m_oLayersForCallback[i];

				oLayer.pData      = reinterpret_cast<rlGameCanvas_Pixel*>(m_oLayers.scanline(i, 0));
				oLayer.oSize      = oNewRes;
				oLayer.oScreenPos = {}; // TODO: make screen position changable per layer
				oLayer.bVisible   = 1; // TODO: make visibility changable per layer
			}
			m_iLayersForCallback_Size = sizeof(Layer) * m_oLayers.layerCount();
		}


		// calculate drawing rectangle

		const double dRatio = (double)oNewRes.x / oNewRes.y;

		UInt iDisplayWidth  = 0;
		UInt iDisplayHeight = 0;

		// calculate the biggest possible size that keeps the aspect ratio
		{
			iDisplayHeight = m_oClientSize.y;
			iDisplayWidth  = UInt(iDisplayHeight * dRatio);

			if (iDisplayWidth > m_oClientSize.x)
			{
				iDisplayWidth  = m_oClientSize.x;
				iDisplayHeight = UInt(iDisplayWidth / dRatio);

				if (iDisplayHeight > m_oClientSize.y)
					iDisplayHeight = m_oClientSize.y;
			}
		}

		// client area smaller than canvas size --> downscale
		if (m_oClientSize.x < oNewRes.x || m_oClientSize.y < oNewRes.y)
		{
			m_iPixelSize = 1;
			// keep the pre-calculated size
		}

		// canvas fits into client area at least once
		else
		{
			m_iPixelSize = std::min(m_oClientSize.x / oNewRes.x, m_oClientSize.y / oNewRes.y);
			if (m_oConfig.bOversample &&
				m_oClientSize.x > oNewRes.x * m_iPixelSize &&
				m_oClientSize.y > oNewRes.y * m_iPixelSize)
			{
				++m_iPixelSize;
				// keep the pre-calculated size
			}
			else
			{
				iDisplayWidth  = oNewRes.x * m_iPixelSize;
				iDisplayHeight = oNewRes.y * m_iPixelSize;
			}
		}

		// calculate the actual drawing rectangle
		m_oDrawRect.iLeft   = (m_oClientSize.x  - iDisplayWidth) / 2;
		m_oDrawRect.iRight  = m_oDrawRect.iLeft + iDisplayWidth;
		m_oDrawRect.iTop    = (m_oClientSize.y - iDisplayHeight) / 2;
		m_oDrawRect.iBottom = m_oDrawRect.iTop + iDisplayHeight;


		glBindTexture(GL_TEXTURE_2D, m_iIntScaledBufferTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
			m_oConfig.oInitialConfig.oResolution.x * m_iPixelSize,
			m_oConfig.oInitialConfig.oResolution.y * m_iPixelSize,
			0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	}

	// WARNING:
	// This function should only be called if...
	// * the calling thread has control over OpenGL
	// * there is currently no rendering in progress
	// WHAT THIS FUNCTION DOES
	// * change the window style, size and position
	// * update m_oClientSize
	// WHAT THIS FUNCTION DOESN'T DO
	// * draw to the screen
	// * recalculate the rendering size
	void GameCanvas::PIMPL::setMaximization(UInt iMaximization, Resolution oNewRes,
		UInt &iPixelSize)
	{
		int iWinX      = CW_USEDEFAULT;
		int iWinY      = CW_USEDEFAULT;
		int iWinWidth  = CW_USEDEFAULT;
		int iWinHeight = CW_USEDEFAULT;

		m_bIgnoreResize = true;

		const DWORD dwStyle =
			GenWindowStyle(iMaximization, m_oConfig.iMaximizeBtnAction != RL_GAMECANVAS_MAX_NONE);

		if (iMaximization != RL_GAMECANVAS_MAX_MAXIMIZE)
			ShowWindow(m_hWnd, SW_HIDE);
		SetWindowLongW(m_hWnd, GWL_STYLE, dwStyle); // TODO: error handling?

		switch (iMaximization)
		{
		case RL_GAMECANVAS_MAX_FULLSCREEN:
		{
			RECT rcWindow = {};
			GetFullscreenCoordAndSize(rcWindow, m_oClientSize);
			SetWindowPos(
				m_hWnd,                          // hWnd
				NULL,                            // hWndInsertAfter
				rcWindow.left,                   // X
				rcWindow.top,                    // Y
				rcWindow.right  - rcWindow.left, // cx
				rcWindow.bottom - rcWindow.top,  // cy
				SWP_SHOWWINDOW | SWP_NOZORDER    // uFlags
			);
			break;
		}
			
		case RL_GAMECANVAS_MAX_MAXIMIZE:
			ShowWindow(m_hWnd, SW_MAXIMIZE);
			break;

		case RL_GAMECANVAS_MAX_NONE:
		{
			RECT rcWindow = {};
			GetRestoredCoordAndSize(dwStyle, m_hWnd, oNewRes, iPixelSize, rcWindow);

			SetWindowPos(
				m_hWnd,                          // hWnd
				NULL,                            // hWndInsertAfter
				rcWindow.left,                   // X
				rcWindow.top,                    // Y
				rcWindow.right  - rcWindow.left, // cx
				rcWindow.bottom - rcWindow.top,  // cy
				SWP_SHOWWINDOW | SWP_NOZORDER    // uFlags
			);
			break;
		}
		}

		if (iMaximization != RL_GAMECANVAS_MAX_FULLSCREEN)
			m_oClientSize = GetActualClientSize(m_hWnd);

		m_bIgnoreResize = false;
	}

	void GameCanvas::PIMPL::sendMessage(
			rlGameCanvas_UInt     iMsg,
			rlGameCanvas_MsgParam iParam1,
			rlGameCanvas_MsgParam iParam2
	)
	{
		if (m_oConfig.fnOnMsg)
			m_oConfig.fnOnMsg(m_oHandle, iMsg, iParam1, iParam2);
	}

}