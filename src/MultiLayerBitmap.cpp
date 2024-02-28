#include "private/MultiLayerBitmap.hpp"
#include <cassert>

MultiLayerBitmap::~MultiLayerBitmap()
{
	for (size_t i = 0; i < m_oData.size(); ++i)
	{
		auto &iID = m_oTextureIDs[i];
		if (iID)
		{
			glDeleteTextures(1, &iID);
			iID = 0;
		}

		// todo: instead, introduce functions freeAll and freeLayer, call freeAll here
	}
}

bool MultiLayerBitmap::create(lib::UInt iLayerCount, GLsizei iWidth, GLsizei iHeight)
{
	destroy();

	if (iLayerCount == 0 || iWidth == 0 || iHeight == 0)
		return false;

	m_iWidth  = iWidth;
	m_iHeight = iHeight;

	m_oData.resize(iLayerCount); // initialized with default value (zero) => blank pixels
	m_oTextureIDs.resize(iLayerCount); // initialized with default value (zero) => "no ID"

	for (size_t i = 0; i < iLayerCount; ++i)
	{
		m_oData[i] = std::make_unique<lib::Pixel[]>(size_t(iWidth) * iHeight);
	}

	return true;
}

void MultiLayerBitmap::destroy()
{
	if (!hasData())
		return;

	freeAll();
	m_oTextureIDs.clear();
	m_oData.clear();

	m_iWidth  = 0;
	m_iHeight = 0;
}

void MultiLayerBitmap::clearLayer(size_t iLayer)
{
	assert(iLayer < m_oData.size());

	memset(m_oData[iLayer].get(), 0, sizeof(lib::Pixel) * m_iWidth * m_iHeight);
}

void MultiLayerBitmap::clearAll()
{
	for (size_t i = 0; i < m_oData.size(); ++i)
	{
		clearLayer(i);
	}
}

void MultiLayerBitmap::uploadLayer(size_t iLayer)
{
	assert(iLayer < m_oData.size());

	if (m_oTextureIDs[iLayer] == 0)
	{
		glGenTextures(1, m_oTextureIDs.data() + iLayer);

		glBindTexture(GL_TEXTURE_2D, m_oTextureIDs[iLayer]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_iWidth, m_iHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
			m_oData[iLayer].get());
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, m_oTextureIDs[iLayer]);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_iWidth, m_iHeight, GL_RGBA, GL_UNSIGNED_BYTE,
			m_oData[iLayer].get());
	}
}

void MultiLayerBitmap::uploadAll()
{
	for (size_t iLayer = 0; iLayer < m_oData.size(); ++iLayer)
	{
		uploadLayer(iLayer);
	}
}

void MultiLayerBitmap::freeLayer(size_t iLayer)
{
	assert(iLayer < m_oTextureIDs.size());

	auto &iID = m_oTextureIDs[iLayer];

	if (iID == 0)
		return;

	glDeleteTextures(1, &iID);
	iID = 0;
}

void MultiLayerBitmap::freeAll()
{
	for (size_t i = 0; i < m_oTextureIDs.size(); ++i)
	{
		freeLayer(i);
	}
}

GLuint MultiLayerBitmap::textureID(size_t iLayer) const
{
	assert(iLayer < m_oTextureIDs.size());

	return m_oTextureIDs[iLayer];
}
