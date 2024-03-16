#ifndef RLGAMECANVAS_GAMECANVAS_MULTILAYERBITMAP
#define RLGAMECANVAS_GAMECANVAS_MULTILAYERBITMAP





#include <rlGameCanvas++/Pixel.hpp>
namespace lib = rlGameCanvasLib;

#include <gl/GL.h>
#include <vector>
#include <memory>



class MultiLayerBitmap
{
public: // methods

	MultiLayerBitmap() = default;
	MultiLayerBitmap(const MultiLayerBitmap &) = delete;
	MultiLayerBitmap(MultiLayerBitmap &&) = default;
	~MultiLayerBitmap();

	bool  hasData() const { return m_iWidth > 0; }
	operator bool() const { return hasData();    }

	bool create(lib::UInt iLayerCount, GLsizei iWidth, GLsizei iHeight);
	void destroy();

	lib::UInt width     () const { return m_iWidth      ; }
	lib::UInt height    () const { return m_iHeight     ; }
	size_t    layerCount() const { return m_oData.size(); }

	      lib::Pixel *scanline(size_t iLayer, size_t iY)
	{
		return m_oData[iLayer].get() + iY;
	}
	const lib::Pixel *scanline(size_t iLayer, size_t iY) const
	{
		return m_oData[iLayer].get() + (iY * m_iWidth);
	}

	void clearLayer(size_t iLayer);
	void clearAll();

	void uploadLayer(size_t iLayer);
	void uploadAll();

	void freeLayer(size_t iLayer);
	void freeAll();

	GLuint textureID(size_t iLayer) const;


private: // variables

	GLsizei m_iWidth  = 0;
	GLsizei m_iHeight = 0;
	std::vector<std::unique_ptr<lib::Pixel[]>> m_oData;
	std::vector<GLuint> m_oTextureIDs;

};





#endif // RLGAMECANVAS_GAMECANVAS_MULTILAYERBITMAP