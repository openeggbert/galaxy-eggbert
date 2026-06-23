#include "GalaxyEggbertSimpleGame.hpp"

int main(int, char**) {
    auto game = Simple3D::CreateGame<GalaxyEggbertSimpleGame>();
#if defined(GALAXY_EGGBERT_U3D_BIN_PATH)
    game->SetResourcePrefixPaths(GALAXY_EGGBERT_U3D_BIN_PATH);
#endif
    return game->Run();
}
