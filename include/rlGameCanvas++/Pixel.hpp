#ifndef RLGAMECANVAS_PIXEL_CPP
#define RLGAMECANVAS_PIXEL_CPP





#include "../rlGameCanvas/Pixel.h"
#include "Types.hpp"



namespace rlGameCanvasLib
{

	union Pixel
	{
		struct
		{
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;
		} rgba;

		PixelInt val;


		constexpr Pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) :
			rgba({ r, g, b, a })
		{}

		constexpr Pixel(PixelInt val = rlGameCanvas_Color_Blank) : val(val) {}


		operator PixelInt() const { return val; }
	};

	namespace Color
	{
		constexpr Pixel Blank = rlGameCanvas_Color_Blank;
		constexpr Pixel Black = rlGameCanvas_Color_Black;
		constexpr Pixel White = rlGameCanvas_Color_White;
	}

}





#endif // RLGAMECANVAS_PIXEL_CPP