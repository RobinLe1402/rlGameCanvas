#include "private/Windows.hpp"

namespace rlGameCanvasLib
{

	std::wstring UTF8toWindowsUnicode(const U8Char *szUTF8)
	{
		const char *szAsChar = reinterpret_cast<const char *>(szUTF8);
		const auto iLen = MultiByteToWideChar(CP_UTF8, 0, szAsChar, -1, nullptr, 0);
		if (iLen == 0)
			return std::wstring{};

		std::wstring sResult(iLen - 1, '\0');
		MultiByteToWideChar(CP_UTF8, 0, szAsChar, -1, sResult.data(),
			int(sResult.length() + 1));

		return sResult;
	}

	std::string WindowsLastErrorString(DWORD dwLastError)
	{
		LPSTR pMsgBuf = nullptr;

		size_t len = FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM     |
			FORMAT_MESSAGE_IGNORE_INSERTS,             // dwFlags
			NULL,                                      // lpSource
			dwLastError,                               // dwMessageId
			MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), // dwLanguageId
			(LPSTR)&pMsgBuf,                           // lpBuffer
			0,                                         // nSize
			NULL                                       // Arguments
		);
		std::string sResult(pMsgBuf, len);
		LocalFree(pMsgBuf);

		return sResult;
	}

}
