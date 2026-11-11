#pragma once

#include "NetProtocol.hpp"

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace GalaxyEggbert::Net
{
    // Speedy Blupi 2's network game rules (../blupi_recon/games/sb2/src, the owner's choice of
    // 2026-11-10, owner_questions.md Q-019), as Galaxy plays them. Engine-independent.

    // SB2's multiplayer worlds are its missions 201-212 (event.cpp:2584, m_multi + 200); Galaxy's
    // are worlds3d/world201.vwr ... with their identity (hand-authored in 3D, as every world), then
    // 213-217 of Galaxy's own design (the owner's Q-022).
    inline constexpr int kFirstWorld = 201;
    inline constexpr int kLastWorld = 217;

    // PlayPrepare (decblock.cpp:1430-1438): in a network game the crates (12), the goal (7), the
    // secret goal (21, TYPE_CLE) and the enemy Blupis with their helicopter (32) and tank (33)
    // are taken out of the world as it starts.
    [[nodiscard]] constexpr bool RemovedInMultiplayer(int type) noexcept
    {
        return type == 7 || type == 12 || type == 21 || type == 32 || type == 33;
    }

    // What a player makes that the others must have too (ObjectStart(..., TRUE), sent reliably): a
    // personal bomb laid (200-203, its team's, decblupi.cpp:2395), a vehicle left where he got out
    // of it (helicopter 13, jeep 19, skateboard 24, tank 28, overcraft 46: taking one is an
    // ObjectDelete, leaving it an ObjectStart, decblupi.cpp:3285-3300), a lit dynamite (56), whose
    // blast each machine then plays on its own (killing whoever is near on that machine), and his
    // tank's or helicopter's shot (23, TYPE_BALLE, decblupi.cpp:715, 1885), which kills the Blupi
    // it meets there (decblupi.cpp:3514). The bombs' and vehicles' end (taken, blown up) is sent
    // too; a fuse and a shot run to their end everywhere.
    [[nodiscard]] constexpr bool IsVehicle(int type) noexcept
    {
        return type == 13 || type == 19 || type == 24 || type == 28 || type == 46;
    }
    [[nodiscard]] constexpr bool IsReplicatedSpawn(int type) noexcept
    {
        return type == 23 || type == 56 || (type >= 200 && type <= 203) || IsVehicle(type);
    }
    [[nodiscard]] constexpr bool IsPersistentSpawn(int type) noexcept
    {
        return (type >= 200 && type <= 203) || IsVehicle(type);
    }

    // decblupi.cpp:4097-4110: once fewer than two players are present (the others left or lost),
    // the one still playing wins.
    [[nodiscard]] constexpr bool LastSurvivor(int playersPresent) noexcept { return playersPresent < 2; }

    // The colours, as the editor's player starts (PlayerStart.hpp) and SB2's teams 0-3.
    inline constexpr const char* kTeamNames[kTeams] = {"Yellow", "Orange", "Blue", "Green"};

    // The game's messages (Speedy Eggbert 2, games/sb2/res/sb2-eggbert-en.rc 125, 243-262, 286-287).
    inline constexpr std::string_view kTextMultiplayer = "Multiplayer";              // 243
    inline constexpr std::string_view kTextAvailableGames = "Available games on the network :"; // 244
    inline constexpr std::string_view kTextJoin = "Join selected game";              // 247
    inline constexpr std::string_view kTextNewGame = "New game";                     // 248
    inline constexpr std::string_view kTextUpdateList = "Update list";               // 249
    inline constexpr std::string_view kTextPlayerList = "Player list :";             // 254
    inline constexpr std::string_view kTextChatZone = "Chat zone :";                 // 256
    inline constexpr std::string_view kTextSendMessage = "Send message";             // 262
    inline constexpr std::string_view kTextReady = "Ready ?";                        // 286
    inline constexpr std::string_view kTextWon = "Congratulations, you won, you're the only survivor!"; // 287
    inline constexpr std::string_view kTextPause = "<<<<    P A U S E    >>>>";      // 125
    // Galaxy's (Q-022, not the original's): the match's time limit.
    inline constexpr std::string_view kTextTimeUp = "Time is up !";
    inline constexpr std::string_view kTextTimeUpWon = "Time is up ! You won: the most lives left.";
    inline constexpr std::string_view kTextTimeUpShared = "Time is up ! You share the first place.";
    // The time limits the host offers (minutes; 0 none) and its lives (the original's ten, fewer).
    inline constexpr int kTimeLimits[] = {0, 3, 5, 10, 15, 20, 30};
    [[nodiscard]] inline std::string TimeLimitText(int minutes)
    {
        return minutes <= 0 ? std::string("none") : std::to_string(minutes) + " min";
    }
    [[nodiscard]] inline std::string ClockText(float seconds)
    {
        const int s = seconds <= 0.0f ? 0 : static_cast<int>(seconds + 0.999f);
        return std::to_string(s / 60) + ":" + (s % 60 < 10 ? "0" : "") + std::to_string(s % 60);
    }

    [[nodiscard]] inline std::string MissionTitle(int world)                         // 255
    {
        return "Mission #" + std::to_string(world);
    }
    [[nodiscard]] inline std::string LeftNotice(const std::string& name)              // 257
    {
        return name + " has left the game ...";
    }
    [[nodiscard]] inline std::string LostNotice(const std::string& name)              // 258
    {
        return name + " has been just knocked out ...";
    }
    // PK_DIE (decmove.cpp:218-232): the lives a player has left after a death.
    [[nodiscard]] inline std::string DieNotice(const std::string& name, int lives)    // 259-261
    {
        if (lives == 0) return name + " has no life left !";
        if (lives == 1) return name + " has only one more life left !";
        return name + " has " + std::to_string(lives) + " lives left !";
    }

    // Galaxy's (Q-022, not the original's): who got whom, beside PK_DIE's notice.
    [[nodiscard]] inline std::string KillNotice(const std::string& killer, const std::string& victim)
    {
        return killer + " got " + victim + " !";
    }

    // FNV-1a over bytes: the content both machines must share (the multiplayer worlds, the Blupi
    // model's clips), compared before a game is listed or joined.
    [[nodiscard]] constexpr std::uint64_t HashBytes(std::string_view bytes,
                                                    std::uint64_t hash = 14695981039346656037ULL) noexcept
    {
        for (const char c : bytes)
        {
            hash ^= static_cast<std::uint8_t>(c);
            hash *= 1099511628211ULL;
        }
        return hash;
    }
}
