#ifndef RLGAMECANVAS_GAMECANVAS_GRAPHICSDATA
#define RLGAMECANVAS_GAMECANVAS_GRAPHICSDATA





#include <rlGameCanvas++/Types.hpp>
#include <rlGameCanvas++/Pixel.hpp>
#include "PrivateTypes.hpp"

namespace lib = rlGameCanvasLib;

#include <gl/GL.h>
#include <vector>
#include <memory>



class GraphicsData final
{
private: // types

	class Layer final
	{
	public: // methods

		Layer(const Layer &other);
		Layer(GLsizei iWidth, GLsizei iHeight, const lib::Resolution &oScreenPos,
			const lib::Resolution &oScreenSize);
		~Layer();

		GLsizei width()  const { return m_iWidth;  }
		GLsizei height() const { return m_iHeight; }
		lib::Pixel *scanline(lib::UInt iY) { return m_up_pxData.get() + (iY * m_iWidth); }

		const lib::Resolution &getScreenPos() const { return m_oScreenPos; }
		void setScreenPos(const lib::Resolution &oScreenPos);

		void drawFilling();
		void drawAtIntCoords(GLint iLeft, GLint iTop, GLint iRight, GLint iBottom);


	private: // methods

		void upload();


	private: // variables

		GLuint m_iTextureID = 0;
		const GLsizei m_iWidth, m_iHeight;
		const lib::Resolution m_oScreenSize;
		const std::unique_ptr<lib::Pixel[]> m_up_pxData;

		lib::Resolution m_oScreenPos = {};
		float m_fTexLeft   = 0.0f;
		float m_fTexTop    = 1.0f;
		float m_fTexRight  = 1.0f;
		float m_fTexBottom = 0.0f;

	};


public: // methods

	bool create(const lib::Mode_CPP &mode);
	void destroy();


	lib::Pixel *scanline(size_t iLayer, lib::UInt iY) { return m_oLayers[iLayer].scanline(iY); }

	void setScreenPos(size_t iLayer, const lib::Resolution &oScreenPos)
	{
		m_oLayers[iLayer].setScreenPos(oScreenPos);
	}
	lib::Resolution getScreenPos(size_t iLayer)
	{
		return m_oLayers[iLayer].getScreenPos();
	}
	void setVisible  (size_t iLayer, bool bVisible) { m_oVisible[iLayer] = bVisible; }

	void draw();
	void draw_Legacy(const lib::Rect &oDrawRect);


private: // variables

	std::vector<Layer> m_oLayers;
	std::vector<bool>  m_oVisible;

};





#endif // RLGAMECANVAS_GAMECANVAS_GRAPHICSDATA