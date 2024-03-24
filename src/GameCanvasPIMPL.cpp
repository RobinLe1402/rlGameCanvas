#include <rlGameCanvas/Definitions.h>

#include "private/GameCanvasPIMPL.hpp"
#include "private/OpenGL.hpp"
#include "private/PrivateTypes.hpp"
#include "private/Windows.hpp"

#include <gl/glext.h>
#include <windowsx.h>

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

		Maximization StartupFlagsToMaximizationEnum(UInt iStartupFlags)
		{
			iStartupFlags &= 0x0000000F;

			switch (iStartupFlags)
			{
			case RL_GAMECANVAS_SUP_WINDOWED:
				return Maximization::Windowed;
			case RL_GAMECANVAS_SUP_MAXIMIZED:
				return Maximization::Maximized;
			case RL_GAMECANVAS_SUP_FULLSCREEN:
				return Maximization::Fullscreen;

			default:
				return Maximization::Unknown;
			}
		}



		constexpr DWORD GenWindowStyle(Maximization eMaximization)
		{
			switch (eMaximization)
			{
			case Maximization::Fullscreen:
				return WS_POPUP;

			case Maximization::Maximized:
				return (WS_OVERLAPPEDWINDOW & ~WS_SIZEBOX) | WS_MAXIMIZE;

			case Maximization::Windowed:
				return WS_OVERLAPPEDWINDOW & ~WS_SIZEBOX;

			default:
				throw std::exception{};
			}
		}

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

		void GetRestoredCoordAndSize(HWND hWndOnDestMonitor,
			const Resolution &oRes, UInt iPixelSize, RECT &rcWindow_Necessary)
		{
			constexpr DWORD dwStyle = GenWindowStyle(Maximization::Windowed);


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
		wc.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;

		bRegistered = RegisterClassExW(&wc);
		return bRegistered;
	}


	GameCanvas::PIMPL::PIMPL(rlGameCanvas oHandle, const StartupConfig &config) :
		m_oHandle              (oHandle),
		m_sWindowCaption       (config.szWindowCaption ? config.szWindowCaption : u8"rlGameCanvas"),
		m_hIconSmall           (config.hIconSmall),
		m_hIconBig             (config.hIconBig),
		m_fnUpdateState        (config.fnUpdateState),
		m_fnDrawState          (config.fnDrawState),
		m_fnCreateState        (config.fnCreateState),
		m_fnCopyState          (config.fnCopyState),
		m_fnDestroyState       (config.fnDestroyState),
		m_fnOnMsg              (config.fnOnMsg),
		m_fnOnWinMsg           (config.fnOnWinMsg),
		m_oModes               (config.iModeCount), // set values later
		m_bFullscreenOnMaximize(config.iFlags & RL_GAMECANVAS_SUP_FULLSCREEN_ON_MAXIMZE),
		m_bDontOversample      (config.iFlags & RL_GAMECANVAS_SUP_DONT_OVERSAMPLE      ),
		m_bHideMouseCursor     (config.iFlags & RL_GAMECANVAS_SUP_HIDECURSOR           ),
		m_eMaximization        (StartupFlagsToMaximizationEnum(config.iFlags)),
		m_eNonFullscreenMaximization(
			(m_eMaximization != Maximization::Fullscreen) ? m_eMaximization :
			(m_bFullscreenOnMaximize ? Maximization::Windowed : Maximization::Maximized)
		)
	{
		// check if the configuration is valid
		bool bValidConfig =
			m_eMaximization != Maximization::Unknown &&
			(!m_bFullscreenOnMaximize || m_eMaximization != Maximization::Maximized) &&
			m_fnCreateState  != nullptr &&
			m_fnCopyState    != nullptr &&
			m_fnDestroyState != nullptr &&
			m_fnUpdateState  != nullptr &&
			m_fnDrawState    != nullptr &&
			!m_oModes.empty();

		if (bValidConfig)
		{
			for (size_t iMode = 0; iMode < m_oModes.size(); ++iMode)
			{
				auto &input  = config.pcoModes[iMode];
				auto &output = m_oModes[iMode];
				
				bValidConfig =
					input.iLayerCount   > 0 &&
					input.oScreenSize.x > 0 && input.oScreenSize.y > 0;
				if (!bValidConfig)
					break;

				output.oScreenSize = input.oScreenSize;
				output.oLayerMetadata.reserve(input.iLayerCount);
				for (size_t iLayer = 0; iLayer < input.iLayerCount; ++iLayer)
				{
					output.oLayerMetadata.push_back(input.pcoLayerMetadata[iLayer]);
					
					auto &oLayerSize = output.oLayerMetadata.back().oLayerSize;
					
					if (oLayerSize.x == 0)
						oLayerSize.x = output.oScreenSize.x;
					if (oLayerSize.y == 0)
						oLayerSize.y = output.oScreenSize.y;
				}
			}
		}

		if (!bValidConfig)
			throw std::exception{ "Invalid startup configuration." };



		if (m_hIconBig && !m_hIconSmall)
			m_hIconSmall = m_hIconBig;
		else if (m_hIconSmall && !m_hIconBig)
			m_hIconBig = m_hIconSmall;



		// try to register the window class
		if (!RegisterWindowClass())
			ThrowWithLastError("Failed to register the window class.");



		sendMessage(
			RL_GAMECANVAS_MSG_CREATE,
			reinterpret_cast<rlGameCanvas_MsgParam>(&m_iCurrentMode),
			0
		);
		if (m_iCurrentMode >= config.iModeCount)
			m_iCurrentMode = config.iModeCount;

		try
		{
			// try to create the window
			if (CreateWindowExW(
				NULL,                                                   // dwExStyle
				s_szCLASSNAME,                                          // lpClassName
				UTF8toWindowsUnicode(m_sWindowCaption.c_str()).c_str(), // lpWindowName
				GenWindowStyle(m_eMaximization),                        // dwStyle
				CW_USEDEFAULT,                                          // X
				CW_USEDEFAULT,                                          // Y
				CW_USEDEFAULT,                                          // nWidth
				CW_USEDEFAULT,                                          // nHeight
				NULL,                                                   // hWndParent
				NULL,                                                   // hMenu
				s_hInstance,                                            // hInstance
				this                                                    // lpParam
			) == NULL)
				ThrowWithLastError("Failed to create the window.");

			// for compiler: C687 - "could be zero"
			if (m_hWnd == NULL)
				throw std::exception{ "Failed to create the window: No handle." };

			setWindowSize(GetForegroundWindow());

			if (m_hIconSmall != NULL)
				SendMessageW(m_hWnd, WM_SETICON, ICON_SMALL,
					reinterpret_cast<LPARAM>(m_hIconSmall)
				);

			if (m_hIconBig != NULL)
				SendMessageW(m_hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(m_hIconBig));


			ACCEL accAltEnter =
			{
				.fVirt = FALT,
				.key   = VK_RETURN,
				.cmd = 0
			};

			m_hAccel = CreateAcceleratorTableW(&accAltEnter, 1);
			if (m_hAccel == NULL)
			{
				DestroyWindow(m_hWnd);
				throw std::exception{ "Failed to create accelerator table." };
			}



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

					glMatrixMode(GL_PROJECTION);

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

				gl.glGenFramebuffers(1, &m_iIntScaledBufferFBO);
				glGenTextures(1, &m_iIntScaledBufferTexture);

				glBindTexture(GL_TEXTURE_2D, m_iIntScaledBufferTexture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}

			m_fnCreateState(&m_pvState_Updating);
			m_fnCreateState(&m_pvState_Drawing);

			initializeCurrentMode();
		}
		catch (...)
		{
			sendMessage(RL_GAMECANVAS_MSG_DESTROY, 0, 0);
			throw;
		}
	}

	GameCanvas::PIMPL::~PIMPL() { quit(); }

	bool GameCanvas::PIMPL::run()
	{
		m_oMainThreadID = std::this_thread::get_id();

		m_bRunning = true;
		ShowWindow(m_hWnd, SW_SHOW);
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
				if (!TranslateAcceleratorW(msg.hwnd, m_hAccel, &msg))
				{
					TranslateMessage(&msg);
					DispatchMessageW(&msg);
				}

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

			doUpdate();
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
		m_oGraphicsData.destroy();
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

	void GameCanvas::PIMPL::initializeCurrentMode()
	{
		createGraphicsData();
		
		if (m_eMaximization == Maximization::Windowed)
			setWindowSize(m_hWnd);

		calcRenderParams();
	}

	void GameCanvas::PIMPL::createGraphicsData()
	{
		const auto &mode = m_oModes[m_iCurrentMode];

		m_oGraphicsData.create(mode); // todo: error handling

		const size_t iLayerCount = mode.oLayerMetadata.size();

		m_iLayersForCallback_Size = iLayerCount * sizeof(LayerData);
		m_oLayersForCallback      = std::make_unique<LayerData[]>(iLayerCount);
		m_oLayersForCallback_Copy = std::make_unique<LayerData[]>(iLayerCount);
		m_oLayerSettings          = std::make_unique<LayerSettings[]>(iLayerCount);

		for (size_t iLayer = 0; iLayer < iLayerCount; ++iLayer)
		{
			const auto &oLayerSpecs = mode.oLayerMetadata[iLayer];
			auto &oLayerSettings    = m_oLayerSettings[iLayer];

			oLayerSettings.bVisible   = oLayerSpecs.bHide == 0;
			oLayerSettings.oScreenPos = oLayerSpecs.oScreenPos;

			m_oLayersForCallback[iLayer] =
			{
				.ppxData =
					reinterpret_cast<rlGameCanvas_Pixel*>(m_oGraphicsData.scanline(iLayer, 0)),
				.oLayerSize  = oLayerSpecs.oLayerSize,
				.poScreenPos = &oLayerSettings.oScreenPos,
				.pbVisible   = &oLayerSettings.bVisible
			};
		}
	}

	void GameCanvas::PIMPL::setWindowSize(HWND hWndOnTargetMonitor)
	{
		m_bIgnoreResize = true;
		const DWORD dwStyle = GenWindowStyle(m_eMaximization);
		SetWindowLongW(m_hWnd, GWL_STYLE, dwStyle);
		m_bIgnoreResize = false;

		int iWinX = 0;
		int iWinY = 0;
		int iWinWidth  = 0;
		int iWinHeight = 0;
		switch (m_eMaximization)
		{
		case Maximization::Windowed:
		{
			RECT rcWindow = {};

			if (m_bFBO && m_iPixelSize != m_iPixelSize_Restored)
				m_bGraphicsThread_NewFBOSize = true;
			m_iPixelSize = m_iPixelSize_Restored;

			GetRestoredCoordAndSize(hWndOnTargetMonitor, m_oModes[m_iCurrentMode].oScreenSize,
				m_iPixelSize, rcWindow);
			iWinX      = rcWindow.left;
			iWinY      = rcWindow.top;
			iWinWidth  = rcWindow.right  - rcWindow.left;
			iWinHeight = rcWindow.bottom - rcWindow.top;
			break;
		}

		case Maximization::Maximized:
			m_oClientSize = GetActualClientSize(m_hWnd);
			return; // nothing to do

		case Maximization::Fullscreen:
		{
			RECT rcWindow = {};
			GetFullscreenCoordAndSize(rcWindow, m_oClientSize);
			iWinX      = rcWindow.left;
			iWinY      = rcWindow.top;
			iWinWidth  = rcWindow.right  - rcWindow.left;
			iWinHeight = rcWindow.bottom - rcWindow.top;
			break;
		}
		}

		m_bIgnoreResize = true;
		SetWindowPos(
			m_hWnd,      // hWnd
			NULL,        // hWndInsertAfter
			iWinX,       // X
			iWinY,       // Y
			iWinWidth,   // cx
			iWinHeight,  // cy
			SWP_NOZORDER // uFlags
		);
		m_bIgnoreResize = false;

		if (m_eMaximization != Maximization::Fullscreen)
			m_oClientSize = GetActualClientSize(m_hWnd);
	}

	void GameCanvas::PIMPL::calcRenderParams()
	{
		const auto &oScreenSize = m_oModes[m_iCurrentMode].oScreenSize;

		const UInt iOldPixelSize = m_iPixelSize;

		// calculate drawing rectangle
		const double dRatio = (double)oScreenSize.x / oScreenSize.y;

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
		if (m_oClientSize.x < oScreenSize.x || m_oClientSize.y < oScreenSize.y)
		{
			m_iPixelSize = 1;
			// keep the pre-calculated size
		}

		// canvas fits into client area at least once
		else
		{
			m_iPixelSize = std::min(
				m_oClientSize.x / oScreenSize.x,
				m_oClientSize.y / oScreenSize.y
			);
			if (!m_bDontOversample &&
				m_oClientSize.x > oScreenSize.x * m_iPixelSize &&
				m_oClientSize.y > oScreenSize.y * m_iPixelSize)
			{
				++m_iPixelSize;
				// keep the pre-calculated size
			}
			else
			{
				iDisplayWidth  = oScreenSize.x * m_iPixelSize;
				iDisplayHeight = oScreenSize.y * m_iPixelSize;
			}
		}

		// calculate the actual drawing rectangle
		m_oDrawRect.iLeft   = (m_oClientSize.x  - iDisplayWidth) / 2;
		m_oDrawRect.iRight  = m_oDrawRect.iLeft + iDisplayWidth;
		m_oDrawRect.iTop    = (m_oClientSize.y - iDisplayHeight) / 2;
		m_oDrawRect.iBottom = m_oDrawRect.iTop + iDisplayHeight;


		if (m_bFBO && iOldPixelSize != m_iPixelSize)
			m_bGraphicsThread_NewFBOSize = true;
	}

	LRESULT GameCanvas::PIMPL::localWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (!m_bMinimized && m_fnOnWinMsg)
			m_fnOnWinMsg(m_oHandle, hWnd, uMsg, wParam, lParam);

		switch (uMsg)
		{
		case WM_CREATE:
			m_hWnd = hWnd;
			m_hDC  = GetDC(m_hWnd);
			break;

		case WM_SYSCOMMAND:
			switch (wParam)
			{
			case SC_MAXIMIZE:
				if (m_bFullscreenOnMaximize)
				{
					setFullscreenOnMaximize();
					return 0;
				}
				break;

			case SC_RESTORE:
				if (m_eMaximization != Maximization::Windowed)
				{
					m_eMaximization = Maximization::Windowed;
					const auto &oScreenSize = m_oModes[m_iCurrentMode].oScreenSize;

					const DWORD dwStyle = GetWindowLongW(m_hWnd, GWL_STYLE);
					RECT rcWindow;

					if (m_iPixelSize != m_iPixelSize_Restored)
						m_bGraphicsThread_NewFBOSize = true;
					m_iPixelSize = m_iPixelSize_Restored;

					GetRestoredCoordAndSize(m_hWnd, oScreenSize, m_iPixelSize, rcWindow);

					waitForGraphicsThread();

					WINDOWPLACEMENT wp = { sizeof(wp) };
					GetWindowPlacement(m_hWnd, &wp);
					wp.rcNormalPosition = rcWindow;
					SetWindowPlacement(m_hWnd, &wp);
					break;

					m_bRestoreHandled = true;
					return 0;
				}
				break;
			}
			break;

		case WM_SYSKEYDOWN:
			// [Alt] + [Return] --> toggle fullscreen
			if ((wParam == VK_RETURN) && (HIWORD(lParam) & KF_ALTDOWN))
			{
				m_bFullscreenToggled = true;
				return 0;
			}
			break;

		case WM_SETCURSOR:
			if (LOWORD(lParam) == HTCLIENT)
			{
				if (m_bHasFocus && m_bHideMouseCursor)
					SetCursor(NULL);
				else
					SetCursor(m_hCursor);
				return TRUE;
			}
			break;

		case WM_MOUSEMOVE:
		{
			const auto iClientX = GET_X_LPARAM(lParam);
			const auto iClientY = GET_Y_LPARAM(lParam);

			m_bMouseOverCanvas =
				iClientX >= (int)m_oDrawRect.iLeft && (UInt)iClientX <= m_oDrawRect.iRight &&
				iClientY >= (int)m_oDrawRect.iTop  && (UInt)iClientY <= m_oDrawRect.iBottom;

			if (m_bMouseOverCanvas)
			{
				const auto &oScreenSize = m_oModes[m_iCurrentMode].oScreenSize;
				const double dPixelSize =
					double(m_oDrawRect.iRight - m_oDrawRect.iLeft) / oScreenSize.x;

				const int iCanvasX = iClientX - m_oDrawRect.iLeft;
				const int iCanvasY = iClientY - m_oDrawRect.iTop;
				m_oCursorPos =
				{
					.x = std::min(UInt(iCanvasX / dPixelSize), oScreenSize.x - 1),
					.y = std::min(UInt(iCanvasY / dPixelSize), oScreenSize.y - 1)
				};

			}

			break;
		}

		case WM_MOUSELEAVE:
			m_bMouseOverCanvas = false;
			break;

		case WM_SIZE:
		{
			if (m_eGraphicsThreadState == GraphicsThreadState::NotStarted || m_bIgnoreResize)
				break;

			switch (wParam)
			{
			case SIZE_MAXIMIZED:
			{
				if (!m_bFullscreenOnMaximize)
					m_eMaximization = Maximization::Maximized;
				else
				{
					setFullscreenOnMaximize();
					return 0;
				}
				break;
			}

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

				if (!m_bRestoreHandled)
				{
					m_eMaximization = Maximization::Windowed;
					const auto &oScreenSize = m_oModes[m_iCurrentMode].oScreenSize;

					const DWORD dwStyle = GetWindowLongW(m_hWnd, GWL_STYLE);
					RECT rcWindow;

					if (m_iPixelSize != m_iPixelSize_Restored)
						m_bGraphicsThread_NewFBOSize = true;
					m_iPixelSize                 = m_iPixelSize_Restored;

					GetRestoredCoordAndSize(m_hWnd, oScreenSize, m_iPixelSize, rcWindow);

					if (
						LOWORD(lParam) != m_iPixelSize * oScreenSize.x ||
						HIWORD(lParam) != m_iPixelSize * oScreenSize.y
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
				handleResize(LOWORD(lParam), HIWORD(lParam));
			return 0;
		}

		case WM_KILLFOCUS:
			m_bHasFocus = false;
			sendMessage(RL_GAMECANVAS_MSG_LOSEFOCUS, 0, 0);
			break;

		case WM_SETFOCUS:
			m_bHasFocus = true;
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
			std::unique_lock lock(m_muxNextFrame);
			m_eGraphicsThreadState = GraphicsThreadState::Waiting;
			m_cvNextFrame.notify_one();
			m_cvNextFrame.wait(lock);
			
			std::unique_lock lock_state(m_muxAppState);
			if (!m_bRunning) // app is being shut down
				return; // cancel current frame rendering
			lock_state.unlock();
			

			m_eGraphicsThreadState = GraphicsThreadState::Running;

			if (m_bGraphicsThread_NewViewport)
			{
				glViewport(0, 0, m_oClientSize.x, m_oClientSize.y);
				m_bGraphicsThread_NewViewport = false;
			}
			if (m_bGraphicsThread_NewFBOSize)
			{
				const auto &oScreenSize = m_oModes[m_iCurrentMode].oScreenSize;

				glBindTexture(GL_TEXTURE_2D, m_iIntScaledBufferTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
					oScreenSize.x * m_iPixelSize, oScreenSize.y * m_iPixelSize,
					0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr
				);

				m_bGraphicsThread_NewFBOSize = false;
			}
		}

		// update the canvas
		memcpy_s(
			m_oLayersForCallback_Copy.get(), m_iLayersForCallback_Size,
			m_oLayersForCallback     .get(), m_iLayersForCallback_Size
		);
		auto &mode = m_oModes[m_iCurrentMode];

		UInt iDrawFlags = 0;
		if (m_bNewMode)
		{
			m_bNewMode = false;
			iDrawFlags |= RL_GAMECANVAS_DRW_NEWMODE;
		}

		m_fnDrawState(
			m_oHandle,                                               // canvas
			m_pvState_Drawing,                                       // pcvState
			m_iCurrentMode,                                          // iMode
			mode.oScreenSize,                                        // oScreenSize
			(UInt)mode.oLayerMetadata.size(),                        // iLayers
			m_oLayersForCallback_Copy.get(),                         // poLayers
			reinterpret_cast<rlGameCanvas_Pixel *>(&m_pxBackground), // ppxBackground
			iDrawFlags                                               // iFlags
		);

		m_pxBackground = RLGAMECANVAS_MAKEPIXELOPAQUE(m_pxBackground);

		glClearColor(
			m_pxBackground.rgba.r / 1.0f,
			m_pxBackground.rgba.g / 1.0f,
			m_pxBackground.rgba.b / 1.0f,
			1.0f
		);


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
			const auto &oScreenSize = m_oModes[m_iCurrentMode].oScreenSize;
			glViewport(0, 0, oScreenSize.x * m_iPixelSize, oScreenSize.y * m_iPixelSize);

			glClear(GL_COLOR_BUFFER_BIT);
			glLoadIdentity();
			glOrtho(-1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 1.0f);
			m_oGraphicsData.draw();



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
				glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // black background

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
			glViewport(0, 0, m_oClientSize.x, m_oClientSize.y); // todo: remove again?
			glLoadIdentity();
			glOrtho(0, m_oClientSize.x, m_oClientSize.y, 0, 0.0f, 1.0f);

			glClear(GL_COLOR_BUFFER_BIT);
			m_oGraphicsData.draw_Legacy(m_oDrawRect);
		}



		SwapBuffers(m_hDC);
	}

	// This function is supposed to be called from another thread than the graphics thread.
	// It waits until the graphics thread is waiting.
	// For optimization purposes, no check is done if the graphics thread has already given up
	// control over OpenGL.
	void GameCanvas::PIMPL::waitForGraphicsThread()
	{
		std::unique_lock lock(m_muxNextFrame);

		if (m_eGraphicsThreadState == GraphicsThreadState::Running)
			m_cvNextFrame.wait(lock); // wait for the graphics thread to finish drawing
	}

	// This function shall only be called after calling waitForGraphicsThread.
	void GameCanvas::PIMPL::resumeGraphicsThread()
	{
		m_fnCopyState(m_pvState_Updating, m_pvState_Drawing);
		m_cvNextFrame.notify_one();
	}

	void GameCanvas::PIMPL::setFullscreenOnMaximize()
	{
		waitForGraphicsThread();
		m_eNonFullscreenMaximization = m_eMaximization;
		m_eMaximization              = Maximization::Fullscreen;
		m_bMaxFullscreen             = true;

		RECT rcWindow = {};
		Resolution oClientSize = {};
		GetFullscreenCoordAndSize(rcWindow, oClientSize);

		m_bIgnoreResize = true;
		ShowWindow(m_hWnd, SW_HIDE);
		SetWindowLongW(m_hWnd, GWL_STYLE, GenWindowStyle(Maximization::Fullscreen));
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
	}

	void GameCanvas::PIMPL::doUpdate()
	{
		const Config cfgOld =
		{
			.iMode  = m_iCurrentMode,
			.iFlags =
				UInt(m_eMaximization == Maximization::Fullscreen ? RL_GAMECANVAS_CFG_FULLSCREEN : 0)
				|
				UInt(m_bHideMouseCursor                          ? RL_GAMECANVAS_CFG_HIDECURSOR : 0)
		};

		Config cfgNew = cfgOld;
		if (m_bFullscreenToggled)
		{
			cfgNew.iFlags ^= RL_GAMECANVAS_CFG_FULLSCREEN;
			m_bFullscreenToggled = false;
		}

		const bool bMouseOverCanvas = m_bMouseOverCanvas && m_bHasFocus;
		const State oCurrentState =
		{
			.oMousePos = m_oCursorPos,
			.iFlags    = (bMouseOverCanvas ? RL_GAMECANVAS_STA_MOUSE_ON_CANVAS : 0u)
		};



		m_tp2 = std::chrono::system_clock::now();
		if (m_tp1 == decltype(m_tp2){}) // not initialized?
			m_tp1 = m_tp2;

		const double dElapsedSeconds =
			std::chrono::duration_cast<std::chrono::duration<double>>(m_tp2 - m_tp1).count();

		m_bRunningUpdate = true;
		m_fnUpdateState(
			m_oHandle,          // canvas
			&oCurrentState,     // pcoReadonlyState
			m_pvState_Updating, // pvState
			dElapsedSeconds,    // dSecsSinceLastCall
			&cfgNew             // poConfig
		);
		m_bRunningUpdate = false;
		m_tp1 = m_tp2;



		if (cfgOld != cfgNew)
			updateConfig(cfgNew);
	}

	void GameCanvas::PIMPL::updateConfig(const Config &cfg, bool bForceUpdateAll)
	{
		const bool bFullscreen = cfg.iFlags & RL_GAMECANVAS_CFG_FULLSCREEN;
		const bool bHideCursor = cfg.iFlags & RL_GAMECANVAS_CFG_HIDECURSOR;

		const bool bNewMode           = bForceUpdateAll || cfg.iMode != m_iCurrentMode;
		const bool bFullscreenToggled = bForceUpdateAll ||
			bFullscreen != (m_eMaximization == Maximization::Fullscreen);


		if (m_bHideMouseCursor != bHideCursor)
		{
			m_bHideMouseCursor = bHideCursor;

			// if the mouse cursor is currently on top of the client area, manually refresh the
			// cursor immediately, as by default it will only change once it moves.
			if (m_bHasFocus)
			{
				POINT ptCursor;
				GetCursorPos(&ptCursor);
				ScreenToClient(m_hWnd, &ptCursor);
				if (ptCursor.x >= 0 && ptCursor.y >= 0 &&
					(UInt)ptCursor.x < m_oClientSize.x &&
					(UInt)ptCursor.y < m_oClientSize.y)
				{
					if (m_bHideMouseCursor)
						SetCursor(NULL);
					else
						SetCursor(m_hCursor);
				}
			}
		}

		if (!bNewMode && !bFullscreenToggled)
			return;



		if (bFullscreenToggled && bFullscreen)
			m_eNonFullscreenMaximization = m_eMaximization;
		m_eMaximization = bFullscreen ? Maximization::Fullscreen : m_eNonFullscreenMaximization;
		m_iCurrentMode  = cfg.iMode;

		const auto &mode    = m_oModes[cfg.iMode];
		const bool bRunning = m_eGraphicsThreadState != GraphicsThreadState::NotStarted;


		const bool bHidden = bRunning &&
			((bNewMode && m_eMaximization == Maximization::Windowed) || bFullscreenToggled);
		if (bHidden)
			ShowWindow(m_hWnd, SW_HIDE);

		if (bRunning)
			waitForGraphicsThread();

		if (bFullscreenToggled)
			setWindowSize(m_hWnd);

		if (bNewMode)
			initializeCurrentMode();

		calcRenderParams();

		if (bHidden)
			ShowWindow(m_hWnd, SW_SHOW);
	}

	void GameCanvas::PIMPL::handleResize(unsigned iClientWidth, unsigned iClientHeight)
	{
#ifndef NDEBUG
		printf("> Resize: %u x %u pixels\n", iClientWidth, iClientHeight);
#endif // NDEBUG

		waitForGraphicsThread();

		bool bResize = false;
		
		m_oClientSize.x = iClientWidth;
		m_oClientSize.y = iClientHeight;

		if (!m_bFBO)
			m_bGraphicsThread_NewViewport = true;

		
		calcRenderParams();
	}

	void GameCanvas::PIMPL::sendMessage(
			rlGameCanvas_UInt     iMsg,
			rlGameCanvas_MsgParam iParam1,
			rlGameCanvas_MsgParam iParam2
	)
	{
		if (m_fnOnMsg)
			m_fnOnMsg(m_oHandle, iMsg, iParam1, iParam2);
	}

}