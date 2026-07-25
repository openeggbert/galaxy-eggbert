#pragma once
#include <cstdint>

namespace GalaxyEggbert::Def {

// High-level game screen/mode. The game is always in exactly one phase.
// Transitions are performed by the game state machine.
enum class GamePhase : uint8_t
{
    None,       // initial / uninitialised
    First,      // very first frame after startup
    Wait,       // waiting for async operation
    Init,       // main-menu / gamer-select screen
    Play,       // active gameplay
    Pause,      // pause overlay
    Lost,       // player lost the current level
    Win,        // player completed the current level
    Trial,      // trial/demo mode — purchase prompt
    MainSetup,  // settings screen from main menu
    PlaySetup,  // settings screen during gameplay
    Resume,     // resume-from-checkpoint confirmation
    Ranking,    // high-score screen
    Editor,     // in-game 3D world editor (not a mobile-eggbert phase --
                // content-creation tooling, see plan.md section 6)
};

constexpr uint8_t  ToRaw(GamePhase p)      { return static_cast<uint8_t>(p); }
constexpr GamePhase ToGamePhase(int v)     { return static_cast<GamePhase>(static_cast<uint8_t>(v)); }

} // namespace GalaxyEggbert::Def
