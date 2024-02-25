/*
	WINDOWS-SPECIFIC CODE
	While this whole project is highly windows-specific, this file contains code only ever necessary
	on Windows.
*/
#ifndef RLGAMECANVAS_WINDOWS
#define RLGAMECANVAS_WINDOWS





#include <rlGameCanvas++/Types.hpp>
#include <string>



namespace rlGameCanvasLib
{

	std::wstring UTF8toWindowsUnicode(const U8Char *szUTF8);

	std::string WindowsLastErrorString(DWORD dwLastError);

}





#endif // RLGAMECANVAS_WINDOWS