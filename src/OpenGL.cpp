#include "private/OpenGL.hpp"
#include <cstdlib>

namespace rlGameCanvasLib
{


#define rlGLFUNC(s) ((std::string(s) + m_sFuncSuffix).c_str())
	OpenGL::OpenGL() :
		m_sVersion(reinterpret_cast<const char *>(glGetString(GL_VERSION))),
		m_iVersion(m_sVersion.c_str()),
		m_sFuncSuffix(m_iVersion.asDouble() >= 3.0 ? "" : "EXT"),
		glGenFramebuffers(
			(PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress(rlGLFUNC("glGenFramebuffers"))),
		glDeleteFramebuffers(
			(PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress(rlGLFUNC("glDeleteFramebuffers"))),
		glBindFramebuffer(
			(PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress(rlGLFUNC("glBindFramebuffer"))),
		glFramebufferTexture2D(
			(PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress(rlGLFUNC("glFramebufferTexture2D"))),
		glCheckFramebufferStatus(
			(PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress(
				rlGLFUNC("glCheckFramebufferStatus")))
	{
		
	}

#undef FUNCNAME

}