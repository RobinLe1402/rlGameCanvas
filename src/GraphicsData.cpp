#include "private/GraphicsData.hpp"
#include "include-thirdparty/gl/glext.h"

#include <cassert>





GraphicsData::Layer::Layer(const Layer &other) :
	m_iWidth     (other.m_iWidth),
	m_iHeight    (other.m_iHeight),
	m_oScreenSize(other.m_oScreenSize),
	m_up_pxData  (std::make_unique<lib::Pixel[]>(m_iWidth * m_iHeight)),
	m_oScreenPos (other.m_oScreenPos),
	m_fTexLeft   (other.m_fTexLeft),
	m_fTexTop    (other.m_fTexTop),
	m_fTexRight  (other.m_fTexRight),
	m_fTexBottom (other.m_fTexBottom)
{
	const size_t iDataSize = (size_t)m_iWidth * m_iHeight * sizeof(lib::Pixel);
	memcpy_s(m_up_pxData.get(), iDataSize, other.m_up_pxData.get(), iDataSize);
}

GraphicsData::Layer::Layer(GLsizei iWidth, GLsizei iHeight, const lib::Resolution &oScreenPos,
	const lib::Resolution &oScreenSize)
	:
	m_iWidth (iWidth ),
	m_iHeight(iHeight),
	m_oScreenSize(oScreenSize),
	m_up_pxData(std::make_unique<lib::Pixel[]>(iWidth * iHeight))
{
	setScreenPos(oScreenPos);
}

GraphicsData::Layer::~Layer()
{
	if (m_iTextureID)
		glDeleteTextures(1, &m_iTextureID);
}

void GraphicsData::Layer::setScreenPos(const lib::Resolution &oScreenPos)
{
	m_oScreenPos =
	{
		.x = oScreenPos.x % m_oScreenSize.x,
		.y = oScreenPos.y % m_oScreenSize.y
	};

	m_fTexLeft   =        (float(m_oScreenPos.x                  ) / m_iWidth );
	m_fTexTop    = 1.0f - (float(m_oScreenPos.y                  ) / m_iHeight);
	m_fTexRight  =        (float(m_oScreenPos.x + m_oScreenSize.x) / m_iWidth );
	m_fTexBottom = 1.0f - (float(m_oScreenPos.y + m_oScreenSize.y) / m_iHeight);
}

void GraphicsData::Layer::drawFilling()
{
	upload();

	// draw texture (upside down)
	glBegin(GL_TRIANGLE_STRIP);
	{
		// first triangle
		//
		// TEXTURE:  SCREEN:
		//  1--3      2
		//  | /       |\
		//  |/        | \
		//  2         1--3

		glTexCoord2f(m_fTexLeft,  m_fTexTop);    glVertex3f(-1.0f, -1.0f, 0.0f);
		glTexCoord2f(m_fTexLeft,  m_fTexBottom); glVertex3f(-1.0f,  1.0f, 0.0f);
		glTexCoord2f(m_fTexRight, m_fTexTop);    glVertex3f( 1.0f, -1.0f, 0.0f);


		// second triangle (sharing an edge with the first one)
		//
		// TEXTURE:  SCREEN:
		//     3      2--4
		//    /|       \ |
		//   / |        \|
		//  2--4         3

		glTexCoord2f(m_fTexRight, m_fTexBottom); glVertex3f(1.0f, 1.0f, 0.0f);
	}
	glEnd();
}

void GraphicsData::Layer::drawAtIntCoords(GLint iLeft, GLint iTop, GLint iRight, GLint iBottom)
{
	upload();

	// draw texture (but upside down)
	glBegin(GL_TRIANGLE_STRIP);
	{
		// first triangle
		//
		// TEXTURE:  SCREEN:
		//  1--3      2
		//  | /       |\
		//  |/        | \
		//  2         1--3

		glTexCoord2f(m_fTexLeft,  m_fTexTop);    glVertex3i(iLeft,  iBottom, 0);
		glTexCoord2f(m_fTexLeft,  m_fTexBottom); glVertex3i(iLeft,  iTop,    0);
		glTexCoord2f(m_fTexRight, m_fTexTop);    glVertex3i(iRight, iBottom, 0);


		// second triangle (sharing an edge with the first one)
		//
		// TEXTURE:  SCREEN:
		//     3      2--4
		//    /|       \ |
		//   / |        \|
		//  2--4         3

		glTexCoord2f(m_fTexRight, m_fTexBottom); glVertex3i(iRight, iTop, 0);
	}
	glEnd();
}

void GraphicsData::Layer::upload()
{
	if (m_iTextureID == 0)
	{
		glGenTextures(1, &m_iTextureID);


		glBindTexture(GL_TEXTURE_2D, m_iTextureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_iWidth, m_iHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
			m_up_pxData.get());
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, m_iTextureID);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_iWidth, m_iHeight, GL_RGBA, GL_UNSIGNED_BYTE,
			m_up_pxData.get());
	}
}





bool GraphicsData::create(const lib::Mode_CPP &mode)
{
	destroy();

	if (mode.oLayerMetadata.empty() || mode.oScreenSize.x == 0 || mode.oScreenSize.y == 0)
		return false;

	m_oLayers .reserve(mode.oLayerMetadata.size());
	m_oVisible.reserve(mode.oLayerMetadata.size());

	for (size_t i = 0; i < mode.oLayerMetadata.size(); ++i)
	{
		const auto &setup = mode.oLayerMetadata[i];

		lib::Resolution oLayerSize =
		{
			setup.oLayerSize.x,
			setup.oLayerSize.y
		};

		if (oLayerSize.x == 0)
			oLayerSize.x = mode.oScreenSize.x;
		if (oLayerSize.y == 0)
			oLayerSize.y = mode.oScreenSize.y;

		m_oLayers.push_back(
			Layer(setup.oLayerSize.x, setup.oLayerSize.y, setup.oScreenPos, mode.oScreenSize)
		);
		m_oVisible.push_back(!setup.bHide);
	}

	return true;
}

void GraphicsData::destroy()
{
	m_oLayers .clear();
	m_oVisible.clear();
}

void GraphicsData::draw()
{
	for (size_t iLayer = 0; iLayer < m_oLayers.size(); ++iLayer)
	{
		if (m_oVisible[iLayer])
			m_oLayers[iLayer].drawFilling();
	}
}

void GraphicsData::draw_Legacy(const lib::Rect &oDrawRect)
{
	for (size_t iLayer = 0; iLayer < m_oLayers.size(); ++iLayer)
	{
		if (m_oVisible[iLayer])
			m_oLayers[iLayer].drawAtIntCoords(
				oDrawRect.iLeft,
				oDrawRect.iTop,
				oDrawRect.iRight,
				oDrawRect.iBottom
			);
	}
}
