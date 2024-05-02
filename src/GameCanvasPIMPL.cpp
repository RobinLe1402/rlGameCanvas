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

		constexpr double dMaxWindowedSize = 0.80; // times the monitor width and height

		constexpr DWORD dwStyle_Windowed = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME;
		constexpr DWORD dwStyle_Fullscreen = WS_POPUP;

		// time, in seconds, before cursor is hidden over the nonclient area.
		constexpr double dCursorHideExTimeout = 2.0;



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

		bool operator==(const Resolution &o1, const Resolution &o2)
		{
			return memcmp(&o1, &o2, sizeof(Resolution)) == 0;
		}

		bool operator!=(const Config &o1, const Config &o2)
		{
			return memcmp(&o1, &o2, sizeof(Config)) != 0;
		}



		Resolution GetScaledResolution(Resolution oResolution, Resolution oAvailableSpace,
			bool bPreferPixelPerfect)
		{
			const double dRatio = (double)oResolution.x / oResolution.y;

			// calculate the biggest possible size that keeps the aspect ratio
			Resolution oResult =
			{
				.x = UInt(oAvailableSpace.y * dRatio),
				.y = oAvailableSpace.y
			};


			if (oResult.x > oAvailableSpace.x)
			{
				oResult.x  = oAvailableSpace.x;
				oResult.y = UInt(oResult.x / dRatio);

				if (oResult.y > oAvailableSpace.y)
					oResult.y = oAvailableSpace.y;
			}

			// canvas fits into client area at least once
			if (oAvailableSpace.x >= oResolution.x && oAvailableSpace.y >= oResolution.y)
			{
				const UInt iPixelSize = std::min(
					oAvailableSpace.x / oResolution.x,
					oAvailableSpace.y / oResolution.y
				);
				if (bPreferPixelPerfect ||
					oAvailableSpace.x <= oResolution.x * iPixelSize ||
					oAvailableSpace.y <= oResolution.y * iPixelSize)
				{
					oResult.x = oResolution.x * iPixelSize;
					oResult.y = oResolution.y * iPixelSize;
				}
			}

			return oResult;
		}


		struct ScaledResolution
		{
			Rect oDisplayRect;
			UInt iPixelSize;
		};

		ScaledResolution GetScaledResolutionEx(Resolution oResolution, Resolution oAvailableSpace,
			bool bDontOversample)
		{
			ScaledResolution oResult = {};

			Resolution dispres;

			dispres = GetScaledResolution(oResolution, oAvailableSpace,
				bDontOversample);
			oResult.iPixelSize = (UInt)std::max(1.0,
				std::max(
					std::ceil((double)dispres.x / oResolution.x),
					std::ceil((double)dispres.y / oResolution.y)
				)
			);

			oResult.oDisplayRect.iLeft = (oAvailableSpace.x - dispres.x) / 2;
			oResult.oDisplayRect.iTop  = (oAvailableSpace.y - dispres.y) / 2;

			oResult.oDisplayRect.iRight  = oResult.oDisplayRect.iLeft + dispres.x;
			oResult.oDisplayRect.iBottom = oResult.oDisplayRect.iTop  + dispres.y;

			return oResult;
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
		m_bPreferPixelPerfect  (config.iFlags & RL_GAMECANVAS_SUP_PREFER_PIXELPERFECT),
		m_bRestrictCursor      (config.iFlags & RL_GAMECANVAS_SUP_RESTRICT_CURSOR    ),
		m_bHideCursor          (config.iFlags & RL_GAMECANVAS_SUP_HIDE_CURSOR        ),
		m_bMaximized           (config.iFlags & RL_GAMECANVAS_SUP_MAXIMIZED          ),
		m_bFullscreen          (config.iFlags & RL_GAMECANVAS_SUP_FULLSCREEN         )
	{
		// check if the configuration is valid
		bool bValidConfig =
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

					// check if layer size is smaller than the screen size
					if (oLayerSize.x < input.oScreenSize.x || oLayerSize.y < input.oScreenSize.y)
					{
						bValidConfig = false;
						break;
					}
				}

				if (!bValidConfig)
					break;
			}
		}

		if (!bValidConfig)
			throw std::exception{ "Invalid startup configuration." };



		// get the windowed border size
		{
			RECT rcBorder = {};
			AdjustWindowRect(&rcBorder, dwStyle_Windowed, FALSE);

			m_oWindowedBorderSize.x = rcBorder.right  - rcBorder.left;
			m_oWindowedBorderSize.y = rcBorder.bottom - rcBorder.top;
		}



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
				dwStyle_Windowed,                                       // dwStyle
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



			const bool bMaximize   = m_bMaximized;
			const bool bFullscreen = m_bFullscreen;
			m_bMaximized  = false;
			m_bFullscreen = false;

			// determine the monitor to initially show the window on
			{
				// launched in fullscreen mode
				// => show on primary monitor to not confuse the user when exiting fullsceen mode
				if (bFullscreen)
					m_hMon = MonitorFromPoint({}, MONITOR_DEFAULTTOPRIMARY);

				// launched in windowed/maximized mode
				// => check for currently active window, if found launch on same monitor.
				else
				{
					const HWND hWndForeground = GetForegroundWindow();
					if (hWndForeground != NULL)
						m_hMon = MonitorFromWindow(hWndForeground, MONITOR_DEFAULTTONEAREST);
					else
						m_hMon = MonitorFromPoint({}, MONITOR_DEFAULTTOPRIMARY);
				}
			}

			// non-fullscreen state must be correct when calling enterFullscreenMode(),
			// so it makes sense to always adjust the windowed size here; no matter if the window
			// is initially set to fullscreen or not.
			adjustWindowedSize();

			// place at center of initial monitor
			{
				MONITORINFO mi{ sizeof(mi) };
				GetMonitorInfoW(m_hMon, &mi);

				RECT rcWindow;
				GetWindowRect(m_hWnd, &rcWindow);

				const int iWinWidth  = rcWindow.right  - rcWindow.left;
				const int iWinHeight = rcWindow.bottom - rcWindow.top;

				const int iMonWidth  = mi.rcWork.right  - mi.rcWork.left;
				const int iMonHeight = mi.rcWork.bottom - mi.rcWork.top;

				// the left window border can be out of bounds.
				// the top one, however, should be visible, so that the user can use the title bar.
				const int iWinX = mi.rcWork.left +             (iMonWidth  - iWinWidth ) / 2;
				const int iWinY = mi.rcWork.top  + std::max(0, (iMonHeight - iWinHeight) / 2);

				SetWindowPos(
					m_hWnd,                   // hWnd
					NULL,                     // hWndInsertAfter
					iWinX,                    // X
					iWinY,                    // Y
					0,                        // cx
					0,                        // cy
					SWP_NOZORDER | SWP_NOSIZE // uFlags
				);
			}

			if (bMaximize)
			{
				SetWindowLongW(m_hWnd, GWL_STYLE, dwStyle_Windowed | WS_MAXIMIZE);
				m_bMaximized = true;
			}

			if (bFullscreen)
				enterFullscreenMode();

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
		logicFrame(); // draw 1st frame immediately
		SetForegroundWindow(m_hWnd);

		wglMakeCurrent(NULL, NULL); // give up control over OpenGL

		// initialize + wait for graphics thread
		{
			std::unique_lock lock(m_muxGraphicsThread);
			m_oGraphicsThread = std::thread(&GameCanvas::PIMPL::graphicsThreadProc, this);
			m_cvGraphicsThread.wait(lock);
		}

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
					runGraphicsTask(GraphicsThreadTask::Pause);
					while (m_bMinimized)
					{
						if (GetMessageW(&msg, m_hWnd, 0, 0) == 0)
							break; // while
						TranslateMessage(&msg);
						DispatchMessageW(&msg);
					}
					m_bMinimized = false;
					m_cvGraphicsThread.notify_one(); // continue graphics thread

					if (!m_bRunning)
						goto lbClose;
				}
			}

			doUpdate();
			runGraphicsTask(GraphicsThreadTask::Draw);
		}
	lbClose:

		m_oMainThreadID = {};

		runGraphicsTask(GraphicsThreadTask::Stop);
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
		if (
			m_eGraphicsThreadState == GraphicsThreadState::NotStarted ||
			m_eGraphicsThreadState == GraphicsThreadState::Stopped
		)
			return;

		PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
	}

	void GameCanvas::PIMPL::initializeCurrentMode()
	{
		createGraphicsData();

		adjustWindowedSize();

		calcRenderParams();
		if (m_bFBO)
			m_bGraphicsThread_NewFBOSize = true;
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
				.bmp =
				{
					.ppxData =
						reinterpret_cast<rlGameCanvas_Pixel*>(m_oGraphicsData.scanline(iLayer, 0)),
					.size  = oLayerSpecs.oLayerSize,
				},
				.poScreenPos = &oLayerSettings.oScreenPos,
				.pbVisible   = &oLayerSettings.bVisible
			};
		}
	}

	void GameCanvas::PIMPL::enterFullscreenMode()
	{
		if (m_bFullscreen)
			return; // nothing to do

		GetWindowPlacement(m_hWnd, &m_wndpl); // save the current window placement
		waitForGraphicsThread();

		m_bIgnoreResize = true;

		const bool bWasVisible = IsWindowVisible(m_hWnd);
		if (bWasVisible)
			ShowWindow(m_hWnd, SW_HIDE);

		SetWindowLongW(m_hWnd, GWL_STYLE, dwStyle_Fullscreen);

		const HMONITOR hMon = MonitorFromPoint({ 0 }, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi{ sizeof(mi) };
		GetMonitorInfoW(hMon, &mi);

		const Resolution oMonitorSize =
		{
			.x = UInt(mi.rcMonitor.right  - mi.rcMonitor.left),
			.y = UInt(mi.rcMonitor.bottom - mi.rcMonitor.top)
		};

		m_oClientSize = oMonitorSize;

		SetWindowPos(
			m_hWnd,             // hWnd
			NULL,               // hWndInsertAfter
			0,                  // X
			0,                  // Y
			oMonitorSize.x + 1, // cx
			oMonitorSize.y,     // cy
			SWP_NOZORDER        // uFlags
		);

		m_bMaximized                  = false;
		m_bFullscreen                 = true;
		m_bGraphicsThread_NewViewport = true;

		calcRenderParams();

		if (bWasVisible)
			ShowWindowAsync(m_hWnd, SW_SHOW);
	}

	void GameCanvas::PIMPL::exitFullscreenMode()
	{
		if (!m_bFullscreen)
			return; // nothing to do

		waitForGraphicsThread();

		ShowWindow(m_hWnd, SW_HIDE);

		SetWindowLongW(m_hWnd, GWL_STYLE, dwStyle_Windowed);
		m_bIgnoreResize = false;
		SetWindowPlacement(m_hWnd, &m_wndpl);

		m_bFullscreen                 = false;
		m_bGraphicsThread_NewViewport = true;

		calcRenderParams();


		ShowWindowAsync(m_hWnd, SW_SHOW);
	}

	void GameCanvas::PIMPL::adjustWindowedSize()
	{
		const auto &mode = currentMode();

		// determine windowed pixel size
		{
			MONITORINFO mi{ sizeof(mi) };
			GetMonitorInfoW(m_hMon, &mi);

			const Resolution oMonRes =
			{
				.x = UInt(mi.rcWork.right  - mi.rcWork.left),
				.y = UInt(mi.rcWork.bottom - mi.rcWork.top )
			};

			m_iPixelSize_Win = std::max<UInt>(
				1,
				std::min(
					(UInt)((oMonRes.x - m_oWindowedBorderSize.x) * 0.8) / mode.oScreenSize.x,
					(UInt)((oMonRes.y - m_oWindowedBorderSize.y) * 0.8) / mode.oScreenSize.y
				)
			);
		}

		const Resolution oClientSize =
		{
			.x = m_iPixelSize_Win * mode.oScreenSize.x,
			.y = m_iPixelSize_Win * mode.oScreenSize.y
		};

		const Resolution oWindowSize =
		{
			.x = std::max(m_oMinWinSize.x, oClientSize.x + m_oWindowedBorderSize.x),
			.y = std::max(m_oMinWinSize.y, oClientSize.y + m_oWindowedBorderSize.y)
		};

		// currently not in windowed mode --> modify saved WINDOWPLACEMENT struct
		if (m_bFullscreen || m_bMaximized)
		{
			if (m_bMaximized)
				GetWindowPlacement(m_hWnd, &m_wndpl);

			m_wndpl.rcNormalPosition.right  = m_wndpl.rcNormalPosition.left + oWindowSize.x;
			m_wndpl.rcNormalPosition.bottom = m_wndpl.rcNormalPosition.top  + oWindowSize.y;

			if (m_bMaximized)
				SetWindowPlacement(m_hWnd, &m_wndpl);

			return;
		}

		waitForGraphicsThread();

		if (m_bFBO)
			m_bGraphicsThread_NewFBOSize = true;

		SetWindowPos(
			m_hWnd,                   // hWnd
			NULL,                     // hWndInsertAfter
			0,                        // X
			0,                        // Y
			oWindowSize.x,            // cx
			oWindowSize.y,            // cy
			SWP_NOZORDER | SWP_NOMOVE // uFlags
		);
	}

	void GameCanvas::PIMPL::calcRenderParams()
	{
		const auto &oScreenSize = m_oModes[m_iCurrentMode].oScreenSize;

		const UInt iOldPixelSize = m_iPixelSize;

		const auto oScaledRes =
			GetScaledResolutionEx(oScreenSize, m_oClientSize, m_bPreferPixelPerfect);
		m_iPixelSize = oScaledRes.iPixelSize;
		m_oDrawRect = oScaledRes.oDisplayRect;


		applyCursorRestriction();

		if (m_bFBO && iOldPixelSize != m_iPixelSize)
			m_bGraphicsThread_NewFBOSize = true;
	}

	void GameCanvas::PIMPL::applyCursorRestriction()
	{
		if (!m_bRestrictCursor)
		{
			ClipCursor(NULL);

#ifndef NDEBUG
			printf("> Cursor restriction disabled\n");
#endif // NDEBUG
		}
		else if (m_bHasFocus && m_bMouseOverCanvas)
		{
			const RECT rcClipCursor = getDrawRect();
			ClipCursor(&rcClipCursor);

#ifndef NDEBUG
			printf("> Cursor restriction enabled\n");
#endif // NDEBUG
		}
	}

	void GameCanvas::PIMPL::applyCursor()
	{
		if (!m_bHasFocus)
			return;


		if (
			(m_bMouseOverCanvas && m_bHideCursor) ||
			(!m_bMouseOverCanvas && m_bHideCursorEx && !m_bMouseCursorOutsideClient)
		)
			SetCursor(NULL);
		else if (!m_bMouseCursorOutsideClient)
			SetCursor(LoadCursorW(NULL, IDC_ARROW));
	}

	RECT GameCanvas::PIMPL::getDrawRect()
	{
		POINT ptTopLeft =
		{
			.x = LONG(m_oDrawRect.iLeft),
			.y = LONG(m_oDrawRect.iTop)
		};
		ClientToScreen(m_hWnd, &ptTopLeft);

		POINT ptBottomRight =
		{
			.x = LONG(m_oDrawRect.iRight),
			.y = LONG(m_oDrawRect.iBottom)
		};
		ClientToScreen(m_hWnd, &ptBottomRight);

		RECT rcResult =
		{
			.left   = ptTopLeft.x,
			.top    = ptTopLeft.y,
			.right  = ptBottomRight.x,
			.bottom = ptBottomRight.y
		};

		return rcResult;
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
				applyCursor();
			else
				SetCursor(LoadCursorW(NULL, IDC_ARROW));
			return TRUE;

		case WM_NCMOUSEMOVE:
			m_dTimeSinceLastMouseMove = 0.0;
			m_bMouseCursorOutsideClient = true;
			m_bMouseOverCanvas = false;
			applyCursor();
			break;

		case WM_MOUSEMOVE:
		{
			m_dTimeSinceLastMouseMove = 0.0;

			m_bMouseCursorOutsideClient = false;
			bool bCursorChanged = false;

			if (m_bHideCursorEx)
			{
				bCursorChanged  = true;
				m_bHideCursorEx = false;
			}

			const auto iClientX = GET_X_LPARAM(lParam);
			const auto iClientY = GET_Y_LPARAM(lParam);

			const bool bMouseOverCanvasBefore = m_bHasFocus && m_bMouseOverCanvas;
			m_bMouseOverCanvas =
				iClientX >= (int)m_oDrawRect.iLeft && (UInt)iClientX <= m_oDrawRect.iRight &&
				iClientY >= (int)m_oDrawRect.iTop  && (UInt)iClientY <= m_oDrawRect.iBottom;

			const auto &oScreenSize = m_oModes[m_iCurrentMode].oScreenSize;
			const double dPixelSize =
				double(m_oDrawRect.iRight - m_oDrawRect.iLeft) / oScreenSize.x;

			if (m_bMouseOverCanvas)
			{
				const int iCanvasX = iClientX - m_oDrawRect.iLeft;
				const int iCanvasY = iClientY - m_oDrawRect.iTop;
				m_oCursorPos =
				{
					.x = std::min(UInt(iCanvasX / dPixelSize), oScreenSize.x - 1),
					.y = std::min(UInt(iCanvasY / dPixelSize), oScreenSize.y - 1)
				};

				if (m_bRestrictCursor && !bMouseOverCanvasBefore)
					applyCursorRestriction();
			}

			if (m_bMouseOverCanvas != bMouseOverCanvasBefore)
				bCursorChanged = true;

			if (bCursorChanged)
				applyCursor();

			break;
		}

		case WM_MOUSELEAVE:
			m_bMouseCursorOutsideClient = true;
			m_bMouseOverCanvas = false;
			break;

		case WM_SIZE:
		{
			if (m_bIgnoreResize)
				break;

			if (m_bMinimized)
			{
				m_bMinimized = false;
				sendMessage(RL_GAMECANVAS_MSG_MINIMIZE, 0, 0);
			}

			switch (wParam)
			{
			case SIZE_MINIMIZED:
				m_bMinimized = true;
				sendMessage(RL_GAMECANVAS_MSG_MINIMIZE, 1, 0);
				break;

			case SIZE_MAXIMIZED:
				m_bMaximized = true;
				break;

			default:
				m_bMaximized = false; // todo: move somewhere else?
			}

			if (m_bMinimized)
				break;

			const Resolution oNewClientSize =
			{
				.x = LOWORD(lParam),
				.y = HIWORD(lParam)
			};

			if (oNewClientSize != m_oClientSize)
			{
				waitForGraphicsThread();

				m_oClientSize = oNewClientSize;

				if (!m_bFBO)
					m_bGraphicsThread_NewViewport = true;


				calcRenderParams();
			}

			break;
		}
			
		case WM_MOVING:
		{
			const auto hMon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
			if (hMon == m_hMon)
				break;

#ifndef NDEBUG
			std::printf("> Monitor changed\n");
#endif
			m_hMon = hMon;
			adjustWindowedSize();
			break;
		}

		case WM_SETFOCUS:
			m_bHasFocus = true;
			sendMessage(RL_GAMECANVAS_MSG_GAINFOCUS, 0, 0);
			applyCursorRestriction();
			break;

		case WM_KILLFOCUS:
			m_bHasFocus = false;
			sendMessage(RL_GAMECANVAS_MSG_LOSEFOCUS, 0, 0);
			m_bMouseOverCanvas = false;
			break;

		case WM_CLOSE:
		{
			m_bRunning = false;
			runGraphicsTask(GraphicsThreadTask::Stop);

			DestroyWindow(m_hWnd);
			return 0;
		}

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
		std::unique_lock lock(m_muxGraphicsThread);
		wglMakeCurrent(m_hDC, m_hOpenGL);
		m_bGraphicsThreadHasControlOverGL = true;

		// tell main thread that the graphics thread is set up
		m_cvGraphicsThread.notify_one();

		bool bRunning = true;
		do
		{
			m_eGraphicsThreadState = GraphicsThreadState::Waiting;
			m_cvGraphicsThread.wait(lock);

			std::unique_lock lockTask(m_muxGraphicsTask);
			m_cvGraphicsTask.notify_one();
			lockTask.unlock();

			switch (m_eGraphicsThreadTask)
			{
			case GraphicsThreadTask::Draw:
				m_eGraphicsThreadState = GraphicsThreadState::Drawing;
				if (!m_bGraphicsThreadHasControlOverGL)
				{
					if (!wglMakeCurrent(m_hDC, m_hOpenGL))
						fprintf(stderr, "Error: Graphics thread couldn't re-claim OpenGL.\n");
					// todo: proper error handling

					m_bGraphicsThreadHasControlOverGL = true;
				}
				renderFrame();
				break;


			case GraphicsThreadTask::GiveUpOpenGL:
				m_bGraphicsThreadHasControlOverGL = false;
				wglMakeCurrent(NULL, NULL);
				break;


			case GraphicsThreadTask::Pause:
				m_eGraphicsThreadState = GraphicsThreadState::Asleep;
				m_cvGraphicsThread.wait(lock); // wait for logic thread to wake up graphics thread
				break;

			case GraphicsThreadTask::Stop:
				bRunning = false;

				m_eGraphicsThreadState = GraphicsThreadState::Stopped;
				wglMakeCurrent(NULL, NULL);
				break;
			}


		} while (bRunning);
	}

	void GameCanvas::PIMPL::renderFrame()
	{
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
		std::unique_lock lock(m_muxGraphicsThread);
	}

	void GameCanvas::PIMPL::runGraphicsTask(GraphicsThreadTask eTask)
	{
		if (m_eGraphicsThreadState == GraphicsThreadState::Stopped)
			return;

		// wait for graphics thread to finish current task
		waitForGraphicsThread();


		switch (eTask)
		{
		case GraphicsThreadTask::Draw:
			if (!m_bGraphicsThreadHasControlOverGL)
				wglMakeCurrent(NULL, NULL);
			m_fnCopyState(m_pvState_Updating, m_pvState_Drawing);
			break;

		case GraphicsThreadTask::GiveUpOpenGL:
			if (!m_bGraphicsThreadHasControlOverGL)
				return; // already in control
		}

		std::unique_lock lock(m_muxGraphicsTask);
		m_eGraphicsThreadTask = eTask;
		m_cvGraphicsThread.notify_one(); // notify graphics thread about new task
		m_cvGraphicsTask.wait(lock);   // wait for verification that the task was accepted
		lock.unlock();

		switch (eTask)
		{
		case GraphicsThreadTask::Draw:
			// return immediately
			break;

		case GraphicsThreadTask::GiveUpOpenGL:
			waitForGraphicsThread();
			wglMakeCurrent(m_hDC, m_hOpenGL);
			break;
		default:
			waitForGraphicsThread();
			break;
		}
	}

	// Updates the state and draws the next frame.
	// Assumes that the logic has already aquired ownership of OpenGL.
	void GameCanvas::PIMPL::logicFrame()
	{
		doUpdate();
		m_fnCopyState(m_pvState_Updating, m_pvState_Drawing);
		renderFrame();
	}

	void GameCanvas::PIMPL::doUpdate()
	{
		const bool bPrevFullscreen = m_bFullscreen;
		const Config cfgOld =
		{
			.iMode  = m_iCurrentMode,
			.iFlags =
				UInt(bPrevFullscreen   ? RL_GAMECANVAS_CFG_FULLSCREEN      : 0) |
				UInt(m_bRestrictCursor ? RL_GAMECANVAS_CFG_RESTRICT_CURSOR : 0) |
				UInt(m_bHideCursor     ? RL_GAMECANVAS_CFG_HIDE_CURSOR     : 0)
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

		m_fnUpdateState(
			m_oHandle,          // canvas
			&oCurrentState,     // pcoReadonlyState
			m_pvState_Updating, // pvState
			dElapsedSeconds,    // dSecsSinceLastCall
			&cfgNew             // poConfig
		);
		m_tp1 = m_tp2;



		const bool bHideCursorEx_Before = m_bHideCursorEx;
		if (!m_bHideCursorEx)
		{
			m_dTimeSinceLastMouseMove += dElapsedSeconds;
			if (m_dTimeSinceLastMouseMove >= dCursorHideExTimeout)
			{
				m_dTimeSinceLastMouseMove = 0.0;
				m_bHideCursorEx = true;

				if (
					(cfgNew.iFlags & RL_GAMECANVAS_CFG_HIDE_CURSOR) ==
					(cfgOld.iFlags & RL_GAMECANVAS_CFG_HIDE_CURSOR)
				)
					applyCursor();

#ifndef NDEBUG
				printf("> Cursor is automatically hidden\n");
#endif // NDEBUG
			}
		}

		if (cfgOld != cfgNew)
			updateConfig(cfgNew);
	}

	void GameCanvas::PIMPL::updateConfig(const Config &cfg)
	{
		const bool bFullscreen     = cfg.iFlags & RL_GAMECANVAS_CFG_FULLSCREEN;
		const bool bRestrictCursor = cfg.iFlags & RL_GAMECANVAS_CFG_RESTRICT_CURSOR;
		const bool bHideCursor     = cfg.iFlags & RL_GAMECANVAS_CFG_HIDE_CURSOR;

		m_bNewMode = cfg.iMode != m_iCurrentMode;
		const bool bFullscreenToggled = bFullscreen != m_bFullscreen;


		if (m_bRestrictCursor != bRestrictCursor)
		{
			m_bRestrictCursor = bRestrictCursor;
			applyCursorRestriction();
		}
		
		if (m_bHideCursor != bHideCursor)
		{
			m_bHideCursor   = bHideCursor;

#ifndef NDEBUG
			if (bHideCursor)
				printf("> Cursor is hidden\n");
			else
				printf("> Cursor is visible\n");
#endif // NDEBUG

			// manually refresh the cursor immediately, as by default it will only change once it
			// moves.
			applyCursor();
		}

		if (!m_bNewMode && !bFullscreenToggled)
			return;



		m_iCurrentMode  = cfg.iMode;

		const auto &mode    = m_oModes[cfg.iMode];
		const bool bRunning = m_eGraphicsThreadState != GraphicsThreadState::NotStarted;


		if (bRunning)
			waitForGraphicsThread(); // todo: can be removed later

		if (bFullscreenToggled)
		{
			if (bFullscreen)
				enterFullscreenMode();
			else
				exitFullscreenMode();
		}

		if (m_bNewMode)
			initializeCurrentMode();

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