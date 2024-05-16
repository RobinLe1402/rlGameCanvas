#ifndef RLGAMECANVAS_GAMECANVAS_OPENGL
#define RLGAMECANVAS_GAMECANVAS_OPENGL





#include <functional>
#include <string>

typedef struct IUnknown IUnknown;
#include <Windows.h>
#include <gl/GL.h>

#include <gl/glext.h>



namespace rlGameCanvasLib
{

	class OpenGL
	{
	public: // types

		struct Version
		{
			unsigned iMajor   = 0;
			unsigned iMinor   = 0;
			unsigned iRelease = 0;

			 Version() = default;
			~Version() = default;

			Version(const char *szVersion)
			{
				std::function<unsigned(const char *&)> fnUInt =
					[&](const char *&szVersion) -> unsigned
					{
						char c = szVersion[0];

						if (c < '0' || c > '9')
							return 0;

						unsigned i = 0;
						do
						{
							i *= 10;
							i += (c - '0');

							++szVersion;
							c = szVersion[0];
						} while (c >= '0' && c <= '9');

						return i;
					};

				const char *sz = szVersion;

				iMajor = fnUInt(sz);
				++sz;
				iMinor = fnUInt(sz);
				if (sz[0] == '.')
				{
					++sz;
					iRelease = fnUInt(sz);
				}
			}

			Version(unsigned iMajor, unsigned iMinor, unsigned iRelease) :
				iMajor(iMajor), iMinor(iMinor), iRelease(iRelease) {}

			bool operator<(const Version &other) const
			{
				if (iMajor < other.iMajor)
					return true;
				if (iMajor > other.iMajor)
					return false;

				if (iMinor < other.iMinor)
					return true;
				if (iMinor > other.iMinor)
					return false;

				if (iRelease < other.iRelease)
					return true;
				if (iRelease > other.iRelease)
					return false;

				// equal --> not <
				return false;
			}

			bool operator>=(const Version &other) const { return !(*this < other); }
			
			bool operator==(const Version &other) const
			{
				return
					iMajor   == other.iMajor &&
					iMinor   == other.iMinor &&
					iRelease == other.iRelease;
			}

			bool operator!=(const Version &other) const
			{
				return !(*this == other);
			}

			double asDouble() const
			{
				double dResult = iMajor;

				double dMinor = iMinor;
				while (dMinor > 1.0)
					dMinor *= 0.1;
				dResult += dMinor;

				return dResult;
			}

		};


	public: // methods

		OpenGL();
		~OpenGL() = default;
		OpenGL(const OpenGL & ) = delete;
		OpenGL(      OpenGL &&) = delete;



	private: // variables

		const std::string m_sVersion;
		const std::string m_sFuncSuffix;
		Version m_iVersion = {};


	public: // variables

		const PFNGLGENFRAMEBUFFERSPROC        glGenFramebuffers;
		const PFNGLDELETEFRAMEBUFFERSPROC     glDeleteFramebuffers;
		const PFNGLBINDFRAMEBUFFERPROC        glBindFramebuffer;
		const PFNGLFRAMEBUFFERTEXTURE2DPROC   glFramebufferTexture2D;
		const PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;


	public: // methods

		const std::string &versionStr() const { return m_sVersion; }
		const Version &versionInt() const { return m_iVersion; }

	};

}





#endif // RLGAMECANVAS_GAMECANVAS_OPENGL