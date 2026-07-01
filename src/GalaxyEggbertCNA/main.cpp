#include "GalaxyEggbertCnaGame.hpp"

int main(int, char**)
{
    auto* game = new GalaxyEggbert::CNA::GalaxyEggbertCnaGame();
    game->Run();
    delete game;
    return 0;
}
