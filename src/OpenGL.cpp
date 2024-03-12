#include "private/OpenGL.hpp"
#include <cstdlib>

namespace rlGameCanvasLib
{


#define rlGLFUNC(s) ((std::string(s) + m_sFuncSuffix).c_str())
	OpenGL::OpenGL() :
		m_sVersion(reinterpret_cast<const char *>(glGetString(GL_VERSION))),
		m_iVersion(m_sVersion.c_str()),
		m_sFuncSuffix(m_iVersion.asDouble() >= 3.0 ? "" : "EXT"),
		glGenFramebuffers
		((void(*)(GLsizei, GLuint*))wglGetProcAddress(rlGLFUNC("glGenFramebuffers"))),
		glDeleteFramebuffers
		((void(*)(GLsizei, GLuint*))wglGetProcAddress(rlGLFUNC("glDeleteFramebuffers"))),
		glBindFramebuffer
		((void(*)(GLenum, GLuint))wglGetProcAddress(rlGLFUNC("glBindFramebuffer"))),
		glFramebufferTexture2D
		((void(*)(GLenum, GLenum, GLenum, GLuint, GLint))wglGetProcAddress(
			rlGLFUNC("glFramebufferTexture2D")
		)),
		glCheckFramebufferStatus
		((GLenum(*)(GLenum))wglGetProcAddress(rlGLFUNC("glCheckFramebufferStatus")))
	{
		
	}

#undef FUNCNAME

}