#include <rlGameCanvas/Core.h>

#include <rlGameCanvas++/GameCanvas.hpp>

#include <exception>
#include <string>

namespace lib = rlGameCanvasLib;



namespace
{

	void ShowError(const char *szErrorMsg)
	{
		std::wstring sMessage(strlen(reinterpret_cast<const char *>(szErrorMsg)), '\0');
		for (size_t i = 0; i < sMessage.length(); ++i)
		{
			sMessage[i] = szErrorMsg[i];
		}

		MessageBoxW(NULL, sMessage.c_str(), L"rlGameCanvas", MB_SYSTEMMODAL | MB_ICONERROR | MB_OK);
	}



	/*
		The following functions are equivalent to C-style casts (i.e. after compiler optimization),
		but I prefer it this way as it's the "cleaner" C++-style way.
	*/

	inline rlGameCanvas PointerToHandle(lib::GameCanvas *pointer)
	{
		return reinterpret_cast<rlGameCanvas>(pointer);
	}

	inline lib::GameCanvas *HandleToPointer(rlGameCanvas handle)
	{
		return reinterpret_cast<lib::GameCanvas *>(handle);
	}

}



RLGAMECANVAS_API rlGameCanvas RLGAMECANVAS_LIB rlGameCanvas_Create(
	const rlGameCanvas_StartupConfig *pStartupConfig
)
{
	if (!pStartupConfig)
		return nullptr;

	lib::GameCanvas *pResult = nullptr;
	try
	{
		pResult = new lib::GameCanvas(*pStartupConfig);
	}
	catch (const std::exception &e)
	{
		ShowError(e.what());
		return nullptr;
	}

	return PointerToHandle(pResult);
}

RLGAMECANVAS_API void RLGAMECANVAS_LIB rlGameCanvas_Destroy(
	rlGameCanvas canvas
)
{
	if (!canvas)
		return;

	delete HandleToPointer(canvas);
}

RLGAMECANVAS_API rlGameCanvas_Bool RLGAMECANVAS_LIB rlGameCanvas_Run(
	rlGameCanvas canvas
)
{
	if (!canvas)
		return false;

	return HandleToPointer(canvas)->run();
}

RLGAMECANVAS_API void RLGAMECANVAS_LIB rlGameCanvas_Quit(
	rlGameCanvas canvas
)
{
	if (!canvas)
		return;

	HandleToPointer(canvas)->quit();
}
