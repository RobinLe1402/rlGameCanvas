#include <rlGameCanvas++/GameCanvas.hpp>
#include <rlGameCanvas++/Bitmap.hpp>
#include <rlGameCanvas/Definitions.h>

#include <cstdio>
#include <exception>

namespace lib = rlGameCanvasLib;

struct GameState
{
	bool   bBlinkingChanged;
	bool   bBlinking;
	double dTimeSinceLastBlink;
	double dTimeBlinking;
};


void __stdcall CreateState(void **ppvData)
{
	*ppvData = new GameState{};
}
void __stdcall DestroyState(void *pvData)
{
	delete pvData;
}
void __stdcall CopyState(const void *pcvSrc, void *pvDest)
{
	*reinterpret_cast<GameState *>(pvDest) = *reinterpret_cast<const GameState *>(pcvSrc);
}

void __stdcall UpdateState(
	rlGameCanvas      canvas,
	const lib::State *pcoReadonlyState,
	void             *pvState,
	double            dSecsSinceLastCall,
	lib::Config      *poConfig
)
{
	auto &state = *reinterpret_cast<GameState *>(pvState);

	state.bBlinkingChanged = false;
	if (!state.bBlinking)
	{
		state.dTimeSinceLastBlink += dSecsSinceLastCall;
		if (state.dTimeSinceLastBlink >= 4.0)
		{
			state.bBlinking           = true;
			state.dTimeSinceLastBlink = 0.0;
			state.bBlinkingChanged    = true;
		}
	}
	else
	{
		state.dTimeBlinking += dSecsSinceLastCall;
		if (state.dTimeBlinking >= 0.2)
		{
			state.bBlinking        = false;
			state.dTimeBlinking    = 0.0;
			state.bBlinkingChanged = true;
		}
	}
}

#define X 0xFF000000
#define _ 0x00000000
constexpr lib::PixelInt pxData[] =
{
	_,_,_,_,_,_,_,_,_,_,
	_,_,X,X,_,_,X,X,_,_,
	_,X,_,_,_,_,_,_,X,_,
	_,_,_,X,_,_,X,_,_,_,
	_,_,_,X,_,_,X,_,_,_,
	_,_,_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,_,_,
	_,X,_,_,_,_,_,_,X,_,
	_,_,X,X,X,X,X,X,_,_,
	_,_,_,_,_,_,_,_,_,_,
};
#undef X
#undef _

constexpr lib::Bitmap bmpTest =
{
	.ppxData = const_cast<lib::PixelInt *>(pxData),
	.size = { .x = 10, .y = 10 }
};

void __stdcall DrawState_(
	rlGameCanvas    canvas,
	const void     *pcvState,
	lib::UInt       iMode,
	lib::Resolution oScreenSize,
	lib::UInt       iLayers,
	lib::LayerData *poLayers,
	lib::PixelInt  *ppxBackground,
	lib::UInt       iFlags
)
{
	if (iFlags & RL_GAMECANVAS_DRW_NEWMODE)
	{
		*ppxBackground = lib::Color::White;

		lib::ApplyBitmapOverlay(&poLayers[0].bmp, &bmpTest, 0, 0, lib::BitmapOverlayStrategy::Replace);
	}

	const auto &state = *reinterpret_cast<const GameState *>(pcvState);
	if (state.bBlinkingChanged)
	{
		const auto pxEyeTop    = (state.bBlinking) ? lib::Color::Blank : lib::Color::Black;
		const auto pxEyeBottom = (state.bBlinking) ? lib::Color::Black : lib::Color::Blank;

		poLayers[0].bmp.ppxData[33] = pxEyeTop;
		poLayers[0].bmp.ppxData[36] = pxEyeTop;
		poLayers[0].bmp.ppxData[42] = pxEyeBottom;
		poLayers[0].bmp.ppxData[47] = pxEyeBottom;
	}
}



int main(int argc, char* argv[])
{
	lib::StartupConfig sc{};

	constexpr lib::LayerMetadata oLAYER_METADATA =
	{
		.oLayerSize = { .x = 10, .y = 10 }
	};
	const lib::Mode oMODE =
	{
		.oScreenSize = { .x = 10, .y = 10 },
		.iLayerCount = 1,
		.pcoLayerMetadata = &oLAYER_METADATA
	};

	sc.iModeCount = 1;
	sc.pcoModes   = &oMODE;

	constexpr char8_t szCAPTION[] = u8"Hi there! :)";
	sc.szWindowCaption = szCAPTION;

	sc.hIconBig = LoadIconW(GetModuleHandleW(NULL), L"ROBINLE_ICON");

	sc.fnCreateState  = CreateState;
	sc.fnDestroyState = DestroyState;
	sc.fnCopyState    = CopyState;

	sc.fnUpdateState  = UpdateState;
	sc.fnDrawState    = DrawState_;

	try
	{
		lib::GameCanvas gc(sc);
		gc.run();
	}
	catch (const std::exception &e)
	{
		std::printf("Exception: %s\n", e.what());
	}

	return 0;
}
