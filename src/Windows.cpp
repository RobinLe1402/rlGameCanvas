#include "private/Windows.hpp"

namespace rlGameCanvasLib
{

	std::wstring UTF8toWindowsUnicode(const U8Char *szUTF8)
	{
		const char *szAsChar = reinterpret_cast<const char *>(szUTF8);
		const auto iLen = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, szAsChar, -1, nullptr, 0);
		if (iLen == 0)
			return std::wstring{};

		std::wstring sResult(iLen - 1, '\0');
		MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, szAsChar, -1, sResult.data(),
			sResult.length() + 1);

		return sResult;
	}

}
