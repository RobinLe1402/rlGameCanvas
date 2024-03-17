#include <rlGameCanvas++/GameCanvas.hpp>
#include "private/GameCanvasPIMPL.hpp"

namespace rlGameCanvasLib
{

	GameCanvas::GameCanvas(const StartupConfig &config)
	{
		try
		{
			m_pPIMPL = new GameCanvas::PIMPL(config, reinterpret_cast<rlGameCanvas>(this));
		}
		catch (...)
		{
			delete m_pPIMPL;
			m_pPIMPL = nullptr;
			throw;
		}
	}

	GameCanvas::~GameCanvas() = default;

	bool GameCanvas::run() { return m_pPIMPL->run(); }

	void GameCanvas::quit() { m_pPIMPL->quit(); }

}