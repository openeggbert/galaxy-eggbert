#include "GalaxyEggbertGame.hpp"

#include <cstring>

int main(int argc, char** argv)
{
    auto* game = new GalaxyEggbert::CNA::GalaxyEggbertGame();
    // INFRA-001 (plan.md §7): a permanent, committed deterministic
    // golden-screenshot capture mode -- loads the fixed demo world, runs a
    // fixed number of ticks, captures screenshots at fixed tick indices to
    // well-known filenames, then exits. See GalaxyEggbertGame::
    // EnableGoldenCaptureMode()'s own comment for the full behavior.
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--golden-capture") == 0)
        {
            game->EnableGoldenCaptureMode();
        }
        // INFRA-002's "behavioral trace" half (plan.md §7) -- see
        // EnableGoldenTraceMode()'s own comment for the full behavior.
        if (std::strcmp(argv[i], "--golden-capture-trace") == 0)
        {
            game->EnableGoldenTraceMode();
        }
    }
    game->Run();
    delete game;
    return 0;
}
