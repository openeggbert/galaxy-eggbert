//using GalaxyEggbert;

#include <iostream>

// CNA/Entrypoint.hpp handles the SDL_main renaming required on Android so that
// SDL's Java bridge (SDLActivity.nativeRunMain) can locate main() as SDL_main.
// Game code must never include <SDL3/SDL_main.h> directly.
#include "CNA/Entrypoint.hpp"

#include "CNA/Logger.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "GalaxyEggbert/Game1.hpp"

int main(int argc, char* args[])
{
    return 0;

    CNA::Logger::Info("SpeedyBlupi: main entered");
    CNA::Logger::SetMinimumLevel(CNA::LogLevel::TRACE);
    CNA::Logger::Info("SpeedyBlupi: before Game1 construction");
    try
    {
        GalaxyEggbert::Game1 game;
        CNA::Logger::Info("SpeedyBlupi: Game1 constructed, entering Run()");
        game.Run();
        CNA::Logger::Info("SpeedyBlupi: Run() returned normally");
    }
    catch (const std::exception& e)
    {
        CNA::Logger::Error(std::string("SpeedyBlupi: fatal exception in main: ") + e.what());
        return 1;
    }
    catch (...)
    {
        CNA::Logger::Error("SpeedyBlupi: unknown fatal exception in main");
        return 1;
    }
    CNA::Logger::Info("SpeedyBlupi: exiting normally");
    return 0;
}
