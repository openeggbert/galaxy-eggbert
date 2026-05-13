//using GalaxyEggbert;

#include <iostream>

// CNA/Entrypoint.hpp handles the SDL_main renaming required on Android so that
// SDL's Java bridge (SDLActivity.nativeRunMain) can locate main() as SDL_main.
// Game code must never include <SDL3/SDL_main.h> directly.
#include "CNA/Entrypoint.hpp"

#include "CNA/Logger.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "GalaxyEggbert/Worlds/test_worlds.hpp"

int main(int argc, char* args[])
{
    GalaxyEggbert::Worlds::test_worlds();

}
