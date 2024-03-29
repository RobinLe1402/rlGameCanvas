#ifndef RLGAMECANVAS_GAMECANVAS_CPP
#define RLGAMECANVAS_GAMECANVAS_CPP





#include <rlGameCanvas++/Types.hpp>



namespace rlGameCanvasLib
{

	class GameCanvas final
	{
	public: // methods

		GameCanvas(const StartupConfig &config);
		GameCanvas(const GameCanvas &) = delete;
		~GameCanvas();

		bool run();
		void quit();

		
	private: // types

		class PIMPL;
		PIMPL *m_pPIMPL = nullptr;

	};

}





#endif // RLGAMECANVAS_GAMECANVAS_CPP