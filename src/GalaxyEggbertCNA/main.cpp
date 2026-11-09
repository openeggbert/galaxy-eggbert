#include "GalaxyEggbertGame.hpp"

// On Android the Java side starts the game through SDL_main: CNA's entry point header renames
// main() to it (empty on the other platforms).
#include <CNA/Platform/Entrypoint.hpp>

#if defined(__ANDROID__)
#include "AndroidAssets.hpp"
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>

namespace
{
    int RunGame(int argc, char** argv);
}

int main(int argc, char** argv)
{
    // An exception escaping to the browser arrives as a bare pointer (no type, no text): say what
    // it was first, then let it go on as before.
    try
    {
        return RunGame(argc, argv);
    }
    catch (const std::exception& error)
    {
        std::cerr << "GalaxyEggbertCNA: fatal exception: " << error.what() << std::endl;
        throw;
    }
    catch (...)
    {
        std::cerr << "GalaxyEggbertCNA: fatal exception (not a std::exception)" << std::endl;
        throw;
    }
}

namespace
{
int RunGame(int argc, char** argv)
{
#if defined(__ANDROID__)
    GalaxyEggbert::CNA::RedirectStdioToLogcat();
    if (!GalaxyEggbert::CNA::PrepareAndroidAssets())
    {
        return 1;
    }
#endif
    auto* game = new GalaxyEggbert::CNA::GalaxyEggbertGame();
    // Main-menu demos (DemoReplay.hpp): `--record-demo <file>` with `--mission N` (optionally
    // `--autoplay <script>`, `--demo-ticks N`, `--demo-title <text>`) records one, `--verify-demo
    // <file>` replays one and compares its end (tools/record_demo.sh, tools/verify_demos.sh).
    std::string recordDemo, demoTitle;
    int demoTicks = 0;
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
        // Development/QA: start straight in mission N (see
        // EnableStartMission()'s own comment).
        if (std::strcmp(argv[i], "--mission") == 0 && i + 1 < argc)
        {
            game->EnableStartMission(std::atoi(argv[++i]));
        }
        // QA: play mission --mission N by a script (EnableAutoplay()'s own comment).
        if (std::strcmp(argv[i], "--autoplay") == 0 && i + 1 < argc && !game->EnableAutoplay(argv[++i]))
        {
            delete game;
            return 2;
        }
        if (std::strcmp(argv[i], "--record-demo") == 0 && i + 1 < argc)
        {
            recordDemo = argv[++i];
        }
        if (std::strcmp(argv[i], "--demo-ticks") == 0 && i + 1 < argc)
        {
            demoTicks = std::atoi(argv[++i]);
        }
        if (std::strcmp(argv[i], "--demo-title") == 0 && i + 1 < argc)
        {
            demoTitle = argv[++i];
        }
        if (std::strcmp(argv[i], "--verify-demo") == 0 && i + 1 < argc)
        {
            game->EnableDemoVerify(argv[++i]);
        }
        // QA: `--mission N` from the selected player's own galaxy (BD-0119), with its progress.
        if (std::strcmp(argv[i], "--galaxy") == 0)
        {
            game->EnableGalaxyStart();
        }
        if (std::strcmp(argv[i], "--spawn") == 0 && i + 1 < argc)
        {
            float x = 0.0f, y = 0.0f, z = 0.0f, yaw = 0.0f;
            const int read = std::sscanf(argv[++i], "%f,%f,%f,%f", &x, &y, &z, &yaw);
            if (read >= 3)
            {
                game->EnableStartSpawn(x, y, z, yaw);
            }
        }
    }
    if (!recordDemo.empty())
    {
        game->EnableDemoRecording(recordDemo, demoTicks, demoTitle);
    }
    game->Run();
    const int exitCode = game->AutoplayExitCode();
    delete game;
    return exitCode;
}
}
