#pragma once

#include "NetSession.hpp"

#include <GalaxyEggbert/Game/ScreenKeyboard.hpp>
#include <GalaxyEggbert/Game/Sound.hpp>
#include <GalaxyEggbert/Net/NetLobby.hpp>
#include <GalaxyEggbert/Net/NetProtocol.hpp>
#include <GalaxyEggbert/Net/NetReplication.hpp>
#include <GalaxyEggbert/Net/NetRules.hpp>

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace GalaxyEggbert::CNA
{
    // The network game's state (plans/plan_gemultiplayer.md; Speedy Blupi 2's, as ../blupi_recon
    // reconstructs it): its screens (the games on the network, the lobby), and a match being
    // played. Created when the player opens Multiplayer; the game alone never has one.
    struct NetGame
    {
        explicit NetGame(Microsoft::Xna::Framework::Game& game) : session(game) {}

        NetSession session;
        std::string name;              // who we are on the network ("Player A", GALAXY_NET_NAME)
        bool signedIn = false;
        std::uint64_t contentHash = 0; // the multiplayer worlds and the Blupi model's clips
        std::vector<int> worlds;       // the multiplayer worlds installed (201-212)
        std::vector<std::string> clipNames; // the Blupi model's clips, sorted (a pose's clip index)

        enum class Screen : std::uint8_t { None, Browser, Lobby };
        Screen screen = Screen::None;
        float findSeconds = 0.0f; // since the last search began (searched again every few seconds)
        int selected = -1;        // the game chosen in the list
        std::string message;      // a line under the list or the players

        // The lobby: the host's own, a client's as the host last sent it.
        Net::Lobby lobby;
        bool helloSent = false;
        float lobbySeconds = 0.0f;
        std::deque<std::string> chat; // the last lines (the original's MAXCHAT 6)
        bool typing = false;
        std::string draft;
        Game::ScreenKeyboard keyboard;
        std::vector<std::pair<Microsoft::Xna::Framework::Input::Keys, bool>> typedKeys;
        bool testReadyAsked = false; // GALAXY_NET_TEST: Ready sent

        // A match.
        struct Peer
        {
            std::uint8_t id = 0;
            int team = 0;
            std::string name;
            bool present = true;
            bool hasPose = false;
            Net::Pose pose, previous;
            float age = 0.0f;      // seconds since the pose arrived
            float interval = 0.05f; // between the last two poses
            int lives = Net::kMultiplayerLives;
            bool paused = false; // his pause stops everyone's game (PK_PAUSE) while he is there
            float silence = 0.0f; // seconds since anything came from him (gone after a while)
        };
        bool match = false;
        int world = Net::kFirstWorld;
        int team = 0;
        int slot = 0;
        int soundsLogged = 0, liftsLogged = 0; // the log's first few of a match
        std::uint8_t hostId = 0; // the host's gamer (its lifts are the ones followed)
        std::uint32_t tick = 0;
        float poseSeconds = 0.0f; // since our last pose (sent paused too, a sign of life)
        std::vector<Peer> peers;
        Net::Replicator replicator;
        std::vector<Net::WorldEvent> incoming;
        std::vector<Game::Sound::MadeSound> soundsOut; // his Blupi's sounds of this step, to send
        std::vector<Net::SoundEvent> soundsIn;         // the others', played before the next step
        std::vector<int> liftSteps;                    // the host: each lift's patrol step before the step (-1 none)
        std::vector<Net::LiftSync> liftsIn;            // a client: the host's lifts, put so before the next step
        bool localPaused = false;
        std::uint8_t killer = Net::kNoKiller; // who caused the death being played (his lobby place)
        bool lost = false;   // no life left: the Lost screen, out of the match
        bool won = false;
        std::deque<std::pair<std::string, float>> notices; // the original's NotifPush, with their seconds
    };
}
