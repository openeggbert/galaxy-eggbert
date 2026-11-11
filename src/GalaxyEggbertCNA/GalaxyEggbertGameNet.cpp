// GalaxyEggbertGame: the network game (plans/plan_gemultiplayer.md; the owner's of 2026-11-10,
// owner_questions.md Q-019): Speedy Blupi 2's, as ../blupi_recon reconstructs it. Its screens (the
// games on the network, the lobby with its chat), the match (each machine plays its own Blupi and
// sends his pose and what he changed; the host keeps the lobby and starts), its end.
#include "GalaxyEggbertGame.hpp"
#include "GameInternal.hpp"
#include "NetGame.hpp"

#include <GalaxyEggbert/Net/NetRules.hpp>
#include <GalaxyEggbert/PlayerStart.hpp>
#include <GalaxyEggbert/Editor/CustomWorldStorage.hpp>
#include <GalaxyEggbert/Game/ObjectDefinitionRegistry.hpp>
#include <GalaxyEggbert/Game/UserFolder.hpp>

#include <Microsoft/Xna/Framework/Input/Keys.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

#include "GameHelpers.hpp"

namespace GalaxyEggbert::CNA
{
    using namespace GalaxyEggbert::Game;
    using namespace GalaxyEggbert::CNA::detail;

    namespace
    {
        constexpr float kFindEverySeconds = 4.0f;  // the list of games searched again
        constexpr int kPoseEveryTicks = 3;         // of the 60 a second: 20 poses, the original's 50 ms
        constexpr float kNoticeSeconds = 5.0f;     // a notice's life (the original's NOTIFDELAY 200 ticks)
        constexpr std::size_t kChatLines = 6;      // MAXCHAT
        constexpr std::size_t kChatInput = 60;     // what may be typed (the line takes "<name> " too)
        constexpr float kAcceptSeconds = 8.0f;     // a client the host never put in its lobby gives up
        constexpr int kListRows = 6;
        constexpr std::size_t kAddressInput = 63; // a host's name or address, ":port" included
        constexpr const char* kAddressFile = "multiplayer.txt"; // in the user folder: the address
        // A player from whom nothing came for so long is gone (his network fell, his game stopped):
        // the others are told as if he had left; the host so, the game ends. One who never sent a
        // pose (his machine still loading the world) has longer. A pose is sent at least this often,
        // paused too.
        constexpr float kSilentSeconds = 6.0f;
        constexpr float kSilentBeforePoseSeconds = 20.0f;
        constexpr float kAliveEverySeconds = 0.25f;

        // The screens' actions (NetScreenLayout's buttons).
        enum Action : int
        {
            kRow0 = 0,
            kJoin = 10, kNewGame, kUpdate, kBack, kAddress,
            kColour = 20, kReady, kChat, kStart, kLeave, kWorldPrev, kWorldNext,
        };
    }

    struct GalaxyEggbertGame::NetScreenLayout
    {
        std::string title;
        std::vector<PanelButton> buttons;
        std::vector<int> actions;
        std::vector<PanelLine> lines;
    };

    bool GalaxyEggbertGame::NetMatchActive() const noexcept { return net_ && net_->match; }
    bool GalaxyEggbertGame::NetScreenShown() const noexcept
    {
        return net_ && net_->screen != NetGame::Screen::None;
    }
    int GalaxyEggbertGame::NetTeam() const noexcept { return NetMatchActive() ? net_->team : 0; }

    void GalaxyEggbertGame::OpenMultiplayer()
    {
        if (!net_) net_ = std::make_unique<NetGame>(*this);
        auto& n = *net_;
        n.screen = NetGame::Screen::Browser;
        n.selected = -1;
        n.message.clear();
        // Signed in automatically as the selected player (owner Q-019 Q6); a test names itself.
        const char* forced = std::getenv("GALAXY_NET_NAME");
        n.name = Net::CleanText(forced != nullptr && *forced != 0
                                    ? std::string(forced)
                                    : std::string("Player ") + static_cast<char>('A' + saveData_.GetSelectedGamer()),
                                Net::kMaxName);
        // What both machines must share: the multiplayer worlds (SB2's numbers) and the model's clips.
        n.worlds.clear();
        std::uint64_t hash = Net::HashBytes("galaxy-eggbert-net");
        hash = Net::HashBytes(std::string(1, static_cast<char>(Net::kProtocolVersion)), hash);
        for (int world = Net::kFirstWorld; world <= Net::kLastWorld; ++world)
        {
            if (const auto bytes = ReadWorldBytes(MissionWorldPath(world)))
            {
                n.worlds.push_back(world);
                hash = Net::HashBytes(*bytes, hash);
            }
        }
        n.clipNames.clear();
        if (blupiModel_)
            for (const auto& [clip, ignored] : blupiModel_->Clips) n.clipNames.push_back(clip);
        std::sort(n.clipNames.begin(), n.clipNames.end());
        for (const auto& clip : n.clipNames) hash = Net::HashBytes(clip + "\n", hash);
        n.contentHash = hash;
        if (!NetSession::Available()) n.message = "This version has no network game.";
        else if (n.worlds.empty()) n.message = "No multiplayer world is installed.";
        n.signedIn = n.session.SignIn(n.name);
        n.findSeconds = kFindEverySeconds; // the first search at once
        // The host's address searched too (GALAXY_NET_ADDRESS for the tests, else the one kept).
        if (const char* forced = std::getenv("GALAXY_NET_ADDRESS"); forced != nullptr)
            NetSetAddress(forced, false);
        else if (std::ifstream in(GalaxyEggbert::Game::UserPath(kAddressFile)); in)
        {
            std::string kept;
            std::getline(in, kept);
            NetSetAddress(kept, false);
        }
        std::cout << "GalaxyEggbertCNA: multiplayer: " << n.name << ", " << n.worlds.size()
                  << " worlds, content " << std::hex << n.contentHash << std::dec << "." << std::endl;
    }

    void GalaxyEggbertGame::CloseMultiplayer()
    {
        if (!net_) return;
        net_->session.Leave();
        net_->screen = NetGame::Screen::None;
        net_->typing = false;
        net_->match = false;
    }

    void GalaxyEggbertGame::NetNotice(const std::string& text)
    {
        if (!net_) return;
        std::cout << "GalaxyEggbertCNA: multiplayer: " << text << std::endl;
        net_->notices.emplace_back(text, kNoticeSeconds);
        while (net_->notices.size() > 4) net_->notices.pop_front();
    }

    std::vector<std::string> GalaxyEggbertGame::NetNotices() const
    {
        std::vector<std::string> lines;
        if (!NetMatchActive()) return lines;
        if (NetRemotePaused()) lines.emplace_back(Net::kTextPause);
        for (const auto& [text, seconds] : net_->notices) lines.push_back(text);
        return lines;
    }

    void GalaxyEggbertGame::NetBroadcastLobby()
    {
        Net::Message m;
        m.kind = Net::MessageKind::Lobby;
        m.lobby = net_->lobby.state;
        net_->session.Send(m);
    }

    void GalaxyEggbertGame::UpdateNet()
    {
        NetTestStep();
        if (!net_) return;
        auto& n = *net_;
        n.session.Update();
        const float dt = demoClockSeconds_;
        for (auto& notice : n.notices) notice.second -= dt;
        while (!n.notices.empty() && n.notices.front().second <= 0.0f) n.notices.pop_front();
        if (!n.signedIn && n.screen != NetGame::Screen::None) n.signedIn = n.session.SignIn(n.name);

        if (!n.session.Active())
        {
            if (n.screen == NetGame::Screen::Browser && n.signedIn && !n.session.Finding())
            {
                n.findSeconds += dt;
                if (n.findSeconds >= kFindEverySeconds)
                {
                    n.findSeconds = 0.0f;
                    n.session.BeginFind(n.contentHash);
                }
            }
            if (n.screen == NetGame::Screen::Browser && n.selected >= static_cast<int>(n.session.Found().size()))
                n.selected = -1;
            return;
        }

        std::string reason;
        if (n.session.TakeEnded(reason))
        {
            NetSessionEnded(reason);
            return;
        }
        // Who came: logged; the host shows them its lobby (it takes them at their Hello).
        for (const auto& [id, name] : n.session.TakeJoined())
        {
            std::cout << "GalaxyEggbertCNA: multiplayer: " << name << " (" << static_cast<int>(id) << ") joined." << std::endl;
            if (n.session.IsHost() && !n.match) NetBroadcastLobby();
        }
        for (const std::uint8_t id : n.session.TakeLeft()) NetPlayerLeft(id);

        if (n.session.HasLocalGamer())
        {
            const std::uint8_t me = n.session.LocalId();
            if (n.session.IsHost() && n.lobby.SlotOf(me) < 0 && !n.match)
            {
                n.lobby.Add(me, n.name);
                NetBroadcastLobby();
            }
            if (!n.session.IsHost() && !n.helloSent)
            {
                Net::Message hello;
                hello.kind = Net::MessageKind::Hello;
                hello.simulationRevision = Net::kProtocolVersion;
                hello.contentHash = n.contentHash;
                hello.name = n.name;
                n.helloSent = n.session.Send(hello);
                if (n.helloSent) std::cout << "GalaxyEggbertCNA: multiplayer: hello sent to the host." << std::endl;
            }
        }
        for (auto& received : n.session.TakeReceived()) NetHandle(received.sender, received.message);
        if (!net_ || !n.session.Active()) return;
        if (n.match)
        {
            // Our sign of life while the step does not run (a pause, ours or another's).
            n.poseSeconds += dt;
            if (n.poseSeconds >= kAliveEverySeconds && !n.lost && n.session.HasLocalGamer()) NetSendPose();
            // Who is silent too long is gone (a hitch of our own does not count: a frame adds 0.25 s at most).
            for (auto& peer : n.peers)
            {
                if (!peer.present) continue;
                peer.silence += std::min(dt, 0.25f);
                if (peer.silence < (peer.hasPose ? kSilentSeconds : kSilentBeforePoseSeconds)) continue;
                std::cout << "GalaxyEggbertCNA: multiplayer: nothing from " << peer.name << " for "
                          << static_cast<int>(peer.silence) << " s." << std::endl;
                if (peer.id == n.hostId)
                {
                    NetSessionEnded("the host does not answer");
                    return;
                }
                peer.present = false;
                peer.paused = false;
                NetNotice(Net::LeftNotice(peer.name));
                NetScoreOut(NetSlotOf(peer.id));
            }
        }

        // A client the host does not take (another version, a full game) leaves after a while.
        if (n.screen == NetGame::Screen::Lobby && !n.session.IsHost() && !n.match)
        {
            if (n.session.HasLocalGamer() && n.lobby.SlotOf(n.session.LocalId()) >= 0) n.lobbySeconds = 0.0f;
            else if ((n.lobbySeconds += dt) > kAcceptSeconds)
            {
                n.session.Leave();
                n.screen = NetGame::Screen::Browser;
                n.message = "The game did not take us (full, or another version).";
            }
        }
        if (n.match) NetCheckWin();
    }

    void GalaxyEggbertGame::NetHandle(std::uint8_t sender, const Net::Message& m)
    {
        auto& n = *net_;
        const bool host = n.session.IsHost();
        const auto peer = [&n, sender]() -> NetGame::Peer* {
            for (auto& p : n.peers)
                if (p.id == sender) return &p;
            return nullptr;
        };
        if (auto* p = peer()) p->silence = 0.0f;
        switch (m.kind)
        {
        case Net::MessageKind::Hello:
            if (!host || n.match) return;
            if (m.contentHash != n.contentHash || m.simulationRevision != Net::kProtocolVersion)
            {
                std::cout << "GalaxyEggbertCNA: multiplayer: " << m.name << " has other content; not taken." << std::endl;
                return;
            }
            std::cout << "GalaxyEggbertCNA: multiplayer: " << m.name << " says hello, place "
                      << n.lobby.Add(sender, m.name) << "." << std::endl;
            NetBroadcastLobby();
            return;
        case Net::MessageKind::Lobby:
            if (!host) n.lobby.state = m.lobby;
            return;
        case Net::MessageKind::Team:
            if (!host) return;
            if (const int slot = n.lobby.SlotOf(sender); slot >= 0 && !n.lobby.state.slots[static_cast<std::size_t>(slot)].ready)
                n.lobby.SetTeam(sender, m.team);
            NetBroadcastLobby();
            return;
        case Net::MessageKind::Ready:
            if (!host) return;
            n.lobby.Back(sender); // a player's Ready says he is in the lobby (back from a match)
            n.lobby.SetReady(sender, m.ready);
            NetBroadcastLobby();
            return;
        case Net::MessageKind::Chat:
            n.chat.push_back(m.text);
            while (n.chat.size() > kChatLines) n.chat.pop_front();
            return;
        case Net::MessageKind::Start:
            if (!host && n.screen == NetGame::Screen::Lobby && !n.match) StartNetMatch(m.world);
            return;
        case Net::MessageKind::Pose:
            if (auto* p = peer(); p != nullptr && n.match && (!p->hasPose || m.pose.tick > p->pose.tick))
            {
                if (!p->hasPose)
                    std::cout << "GalaxyEggbertCNA: multiplayer: " << p->name << " seen at (" << m.pose.x << ", "
                              << m.pose.y << ", " << m.pose.z << ")." << std::endl;
                p->previous = p->hasPose ? p->pose : m.pose;
                p->interval = std::clamp(p->age, 0.02f, 0.25f);
                p->pose = m.pose;
                p->age = 0.0f;
                p->hasPose = true;
                p->lives = m.pose.lives;
            }
            return;
        case Net::MessageKind::Events:
            if (n.match) n.incoming.insert(n.incoming.end(), m.events.begin(), m.events.end());
            return;
        case Net::MessageKind::Lifts:
            if (!host && n.match && sender == n.hostId) n.liftsIn.insert(n.liftsIn.end(), m.lifts.begin(), m.lifts.end());
            return;
        case Net::MessageKind::StopCloud:
            if (n.match && n.session.HasLocalGamer() && m.target == n.session.LocalId() && peer() != nullptr)
            {
                blupi_.StopCloud();
                std::cout << "GalaxyEggbertCNA: multiplayer: our cloud touched another; it is gone." << std::endl;
            }
            return;
        case Net::MessageKind::Sounds:
            if (auto* p = peer(); p != nullptr && n.match && p->present)
                n.soundsIn.insert(n.soundsIn.end(), m.sounds.begin(), m.sounds.end());
            return;
        case Net::MessageKind::Die:
            if (auto* p = peer(); p != nullptr && n.match)
            {
                p->lives = m.lives;
                NetNotice(Net::DieNotice(p->name, m.lives));
                NetScoreDeath(NetSlotOf(sender), m.killer);
            }
            return;
        case Net::MessageKind::Lost:
        case Net::MessageKind::Abandon:
            if (auto* p = peer(); p != nullptr && n.match && p->present)
            {
                p->present = false;
                NetNotice(m.kind == Net::MessageKind::Lost ? Net::LostNotice(p->name) : Net::LeftNotice(p->name));
                if (m.kind == Net::MessageKind::Lost) NetScoreDeath(NetSlotOf(sender), m.killer);
                NetScoreOut(NetSlotOf(sender));
            }
            return;
        case Net::MessageKind::Pause:
            // Each player's own pause: one who left or lost holds no one up.
            if (auto* p = peer(); p != nullptr && n.match) p->paused = m.ready;
            return;
        }
    }

    void GalaxyEggbertGame::NetPlayerLeft(std::uint8_t id)
    {
        auto& n = *net_;
        const int slot = n.lobby.SlotOf(id);
        if (n.session.IsHost() && slot >= 0)
        {
            n.lobby.Remove(id);
            NetBroadcastLobby();
        }
        for (auto& p : n.peers)
        {
            if (p.id != id || !p.present) continue;
            p.present = false;
            NetNotice(Net::LeftNotice(p.name));
            if (n.match) NetScoreOut(slot);
        }
    }

    void GalaxyEggbertGame::NetSessionEnded(const std::string& reason)
    {
        auto& n = *net_;
        const bool wasMatch = n.match;
        n.session.Leave();
        n.match = false;
        sound_.SetOutbox(nullptr);
        n.typing = false;
        n.screen = NetGame::Screen::Browser;
        n.message = "The network game ended: " + reason + ".";
        std::cout << "GalaxyEggbertCNA: multiplayer: " << n.message << std::endl;
        if (wasMatch)
        {
            interaction_ = InteractionSystem();
            SetPhase(GalaxyEggbert::Def::GamePhase::Init, /*bypassFade=*/true);
        }
    }

    // ---- The screens ----

    GalaxyEggbertGame::NetScreenLayout GalaxyEggbertGame::BuildNetScreen(int w, int h) const
    {
        NetScreenLayout l;
        const auto& n = *net_;
        const auto rect = [w, h](float x0, float y0, float x1, float y1) {
            return InputPad::PanelRect(x0, y0, x1, y1, w, h);
        };
        const float s = static_cast<float>(h) / 480.0f;
        const float left = static_cast<float>(w) * 0.5f - 290.0f * s;
        const float centre = static_cast<float>(w) * 0.5f;
        const auto button = [&](int action, float x0, float x1, const std::string& label, bool enabled = true,
                                bool held = false) {
            PanelButton b;
            b.rect = rect(x0, 412.0f, x1, 466.0f);
            b.label = label;
            b.size = 0.5f;
            b.enabled = enabled;
            b.held = held;
            l.buttons.push_back(b);
            l.actions.push_back(action);
        };
        const auto line = [&](const std::string& text, float x, float y, float size = 0.45f, bool faint = false,
                              bool centred = false) {
            l.lines.push_back(PanelLine{text, x, y * s, size, centred, faint});
        };
        if (n.screen == NetGame::Screen::Browser)
        {
            l.title = std::string(Net::kTextMultiplayer);
            if (n.typing)
            {
                line("Host address:  " + n.draft + "_", left, 268.0f, 0.5f);
                line("Its IP address or name (192.168.1.20, a ZeroTier address, a router's address).", left, 300.0f, 0.42f, true);
                line("Empty: the local network only. OK keeps it.", left, 322.0f, 0.42f, true);
                return l;
            }
            line(n.signedIn ? "You play as " + n.name + "." : "Signing in as " + n.name + " ...", left, 62.0f);
            if (NetSession::AddressSearch())
            {
                PanelButton address;
                address.rect = rect(330.0f, 80.0f, 620.0f, 108.0f);
                address.label = "Host address: " + (n.address.empty() ? std::string("none") : n.address);
                address.size = 0.42f;
                address.enabled = !n.session.Active();
                l.buttons.push_back(address);
                l.actions.push_back(kAddress);
            }
            line(std::string(Net::kTextAvailableGames), left, 88.0f);
            const auto& found = n.session.Found();
            for (int i = 0; i < kListRows && i < static_cast<int>(found.size()); ++i)
            {
                const auto& game = found[static_cast<std::size_t>(i)];
                PanelButton row;
                row.rect = rect(20.0f, 114.0f + 40.0f * static_cast<float>(i), 620.0f, 148.0f + 40.0f * static_cast<float>(i));
                row.label = game.host + "   -   " + std::to_string(game.players) + " / " +
                            std::to_string(Net::kMaxPlayers) + "   -   " + Net::MissionTitle(game.world);
                row.size = 0.5f;
                row.row = true;
                row.held = n.selected == i;
                l.buttons.push_back(row);
                l.actions.push_back(kRow0 + i);
            }
            if (found.empty())
                line(n.signedIn ? "No game found yet. Make a new one, or wait for one." : "", centre, 200.0f, 0.45f,
                     true, true);
            if (!n.message.empty()) line(n.message, centre, 384.0f, 0.45f, false, true);
            button(kJoin, 20.0f, 215.0f, std::string(Net::kTextJoin), n.selected >= 0 && n.signedIn);
            button(kNewGame, 225.0f, 375.0f, std::string(Net::kTextNewGame), n.signedIn && !n.worlds.empty());
            button(kUpdate, 385.0f, 530.0f, std::string(Net::kTextUpdateList), n.signedIn);
            button(kBack, 540.0f, 620.0f, "Back");
            return l;
        }

        // The lobby.
        const bool host = n.session.IsHost();
        const std::uint8_t me = n.session.LocalId();
        const int mySlot = n.session.HasLocalGamer() ? n.lobby.SlotOf(me) : -1;
        const bool ready = mySlot >= 0 && n.lobby.state.slots[static_cast<std::size_t>(mySlot)].ready;
        l.title = Net::MissionTitle(n.lobby.state.world);
        if (n.typing)
        {
            line(std::string(Net::kTextSendMessage) + ":  " + n.draft + "_", left, 268.0f, 0.5f);
            const int shown = static_cast<int>(std::min<std::size_t>(n.chat.size(), 5));
            for (int j = 0; j < shown; ++j)
                line(n.chat[n.chat.size() - static_cast<std::size_t>(shown - j)], left, 296.0f + 20.0f * static_cast<float>(j),
                     0.42f);
            return l;
        }
        if (host && n.worlds.size() > 1)
        {
            for (const auto& [action, x0, x1, label] :
                 {std::tuple<int, float, float, const char*>{kWorldPrev, 20.0f, 70.0f, "<"},
                  std::tuple<int, float, float, const char*>{kWorldNext, 570.0f, 620.0f, ">"}})
            {
                PanelButton b;
                b.rect = rect(x0, 26.0f, x1, 62.0f);
                b.label = label;
                b.size = 0.6f;
                l.buttons.push_back(b);
                l.actions.push_back(action);
            }
        }
        line(std::string(Net::kTextPlayerList), left, 66.0f);
        for (int i = 0; i < Net::kMaxPlayers; ++i)
        {
            const auto& slot = n.lobby.state.slots[static_cast<std::size_t>(i)];
            const float y = 92.0f + 26.0f * static_cast<float>(i);
            if (!slot.present)
            {
                line(std::to_string(i + 1) + ".   -", left + 10.0f * s, y, 0.45f, true);
                continue;
            }
            std::string who = std::to_string(i + 1) + ".   " + slot.name;
            if (i == 0) who += "  (host)";
            if (i == mySlot) who += "  (you)";
            line(who, left + 10.0f * s, y);
            line(Net::kTeamNames[slot.team % Net::kTeams], centre + 40.0f * s, y);
            if (slot.playing) line("playing", centre + 170.0f * s, y, 0.45f, true);
            else line(slot.ready ? "Ready" : "...", centre + 170.0f * s, y, 0.45f, !slot.ready);
            // The last match (Q-021): his place, kills / deaths.
            if (const auto& score = n.scores[static_cast<std::size_t>(i)]; n.scored && score.played && score.name == slot.name)
            {
                static const char* const kPlaces[] = {"-", "1st", "2nd", "3rd", "4th"};
                const int place = NetPlaceOf(i);
                line(std::string(kPlaces[place >= 0 && place <= 4 ? place : 0]) + " " + std::to_string(score.kills) + "/" +
                         std::to_string(score.deaths),
                     centre + 235.0f * s, y, 0.42f, true);
            }
        }
        line(std::string(Net::kTextChatZone), left, 204.0f);
        for (std::size_t j = 0; j < n.chat.size(); ++j)
            line(n.chat[j], left + 10.0f * s, 228.0f + 22.0f * static_cast<float>(j), 0.42f);
        if (!n.message.empty()) line(n.message, centre, 384.0f, 0.45f, false, true);
        else if (mySlot < 0) line("Waiting for the host ...", centre, 384.0f, 0.45f, true, true);
        else if (host && !n.lobby.CanStart())
            line("Start when every player is ready (two at least).", centre, 384.0f, 0.42f, true, true);
        button(kColour, 20.0f, 130.0f, "Colour", mySlot >= 0 && !ready);
        button(kReady, 140.0f, 260.0f, ready ? "Ready" : std::string(Net::kTextReady), mySlot >= 0, ready);
        button(kChat, 270.0f, 420.0f, std::string(Net::kTextSendMessage), mySlot >= 0);
        if (host) button(kStart, 430.0f, 520.0f, "Start", n.lobby.CanStart());
        button(kLeave, 530.0f, 620.0f, "Leave");
        return l;
    }

    std::vector<QuadBatch::Rect> GalaxyEggbertGame::NetFocusTargets(int w, int h) const
    {
        if (!NetScreenShown()) return {};
        if (net_->typing) return net_->keyboard.Rects();
        std::vector<QuadBatch::Rect> rects;
        for (const auto& b : BuildNetScreen(w, h).buttons)
            if (b.enabled) rects.push_back(b.rect);
        return rects;
    }

    void GalaxyEggbertGame::NetStartTyping(int w, int h, bool address)
    {
        auto& n = *net_;
        n.typing = true;
        n.typingAddress = address;
        n.draft = address ? n.address : std::string();
        n.typedKeys.clear();
        n.keyboard.Layout(InputPad::PanelRect(20.0f, 64.0f, 620.0f, 254.0f, w, h), address);
        n.keyboard.Begin(n.draft, !address);
    }

    void GalaxyEggbertGame::NetSetAddress(const std::string& typed, bool save)
    {
        auto& n = *net_;
        std::string address;
        for (const char c : typed)
            if (c != ' ' && c >= 0x21 && c < 0x7f && address.size() < kAddressInput) address += c;
        if (!n.session.SetSearchAddress(address))
        {
            n.message = "Not an address: " + address + " (" + n.session.LastError() + ").";
            return;
        }
        n.address = address;
        n.findSeconds = kFindEverySeconds; // searched again at once
        if (save)
        {
            n.message = address.empty() ? "Games are searched on the local network." : "Games are searched at " + address + " too.";
            std::ofstream out(GalaxyEggbert::Game::UserPath(kAddressFile), std::ios::trunc);
            out << address << '\n';
        }
    }

    void GalaxyEggbertGame::NetFinishTyping(bool send)
    {
        auto& n = *net_;
        n.typing = false;
        n.typedKeys.clear();
        if (n.typingAddress)
        {
            n.typingAddress = false;
            if (send) NetSetAddress(n.draft, true);
            n.draft.clear();
            return;
        }
        const std::string text = Net::CleanText(n.draft, kChatInput);
        n.draft.clear();
        if (!send || text.empty()) return;
        // ChatSend (event.cpp:4554-4580): "<name> text", shown here and sent to everyone.
        Net::Message chat;
        chat.kind = Net::MessageKind::Chat;
        chat.text = Net::CleanText("<" + n.name + "> " + text, Net::kMaxChat);
        n.chat.push_back(chat.text);
        while (n.chat.size() > kChatLines) n.chat.pop_front();
        n.session.Send(chat);
    }

    void GalaxyEggbertGame::NetTypedKey(Microsoft::Xna::Framework::Input::Keys key, bool shift)
    {
        if (net_ && net_->typing && net_->typedKeys.size() < 64) net_->typedKeys.emplace_back(key, shift);
    }

    bool GalaxyEggbertGame::NetWantsText() const noexcept { return net_ && net_->screen != NetGame::Screen::None && net_->typing; }

    void GalaxyEggbertGame::UpdateNetScreen(const Microsoft::Xna::Framework::Input::MouseState& mouse, int w, int h,
                                            bool escapePressed)
    {
        auto& n = *net_;
        if (n.typing)
        {
            for (const auto& [key, shift] : n.typedKeys)
            {
                const auto result = ScreenKeyboard::TypeKey(key, shift, n.draft, n.typingAddress ? kAddressInput : kChatInput,
                                                            n.typingAddress);
                if (result == ScreenKeyboard::Result::Done)
                {
                    NetFinishTyping(true);
                    return;
                }
            }
            n.typedKeys.clear();
            const auto input = inputPad_.UpdatePanel(mouse, {}, &n.keyboard);
            if (input.keyboardKey >= 0)
            {
                const auto result = n.keyboard.Press(input.keyboardKey, n.draft, n.typingAddress ? kAddressInput : kChatInput);
                if (result == ScreenKeyboard::Result::Done) NetFinishTyping(true);
                else if (result == ScreenKeyboard::Result::Cancel) NetFinishTyping(false);
            }
            else if (input.outsideClick || escapePressed)
            {
                NetFinishTyping(input.outsideClick && !escapePressed);
            }
            return;
        }
        if (escapePressed)
        {
            if (n.screen == NetGame::Screen::Lobby) NetLeaveLobby();
            else CloseMultiplayer();
            return;
        }
        const auto layout = BuildNetScreen(w, h);
        const auto input = inputPad_.UpdatePanel(mouse, layout.buttons);
        if (input.button < 0 || input.button >= static_cast<int>(layout.actions.size())) return;
        const int action = layout.actions[static_cast<std::size_t>(input.button)];
        const bool host = n.session.IsHost();
        const std::uint8_t me = n.session.LocalId();
        const int mySlot = n.session.HasLocalGamer() ? n.lobby.SlotOf(me) : -1;
        switch (action)
        {
        case kJoin:
            NetJoinGame(n.selected);
            return;
        case kNewGame:
            NetHostGame();
            return;
        case kUpdate:
            n.findSeconds = kFindEverySeconds;
            n.message.clear();
            return;
        case kBack:
            CloseMultiplayer();
            return;
        case kAddress:
            n.message.clear();
            NetStartTyping(w, h, true);
            return;
        case kColour:
            if (mySlot >= 0)
            {
                Net::Message team;
                team.kind = Net::MessageKind::Team;
                team.team = static_cast<std::uint8_t>((n.lobby.state.slots[static_cast<std::size_t>(mySlot)].team + 1) % Net::kTeams);
                if (host)
                {
                    n.lobby.SetTeam(me, team.team);
                    NetBroadcastLobby();
                }
                else n.session.Send(team);
            }
            return;
        case kReady:
            if (mySlot >= 0) NetSetReady(!n.lobby.state.slots[static_cast<std::size_t>(mySlot)].ready);
            return;
        case kChat:
            NetStartTyping(w, h);
            return;
        case kStart:
            NetStartAsHost();
            return;
        case kLeave:
            NetLeaveLobby();
            return;
        case kWorldPrev:
        case kWorldNext:
            if (host && !n.worlds.empty())
            {
                auto it = std::find(n.worlds.begin(), n.worlds.end(), static_cast<int>(n.lobby.state.world));
                std::size_t index = it == n.worlds.end() ? 0 : static_cast<std::size_t>(it - n.worlds.begin());
                index = action == kWorldNext ? (index + 1) % n.worlds.size() : (index + n.worlds.size() - 1) % n.worlds.size();
                n.lobby.state.world = static_cast<std::uint16_t>(n.worlds[index]);
                NetBroadcastLobby();
            }
            return;
        default:
            if (action >= kRow0 && action < kRow0 + kListRows) n.selected = action - kRow0;
            return;
        }
    }

    bool GalaxyEggbertGame::NetHostGame()
    {
        auto& n = *net_;
        if (n.worlds.empty() || !n.session.Host(n.contentHash, n.worlds.front()))
        {
            n.message = n.session.LastError();
            return false;
        }
        n.lobby = Net::Lobby{};
        n.lobby.state.world = static_cast<std::uint16_t>(n.worlds.front());
        n.chat.clear();
        n.message.clear();
        n.screen = NetGame::Screen::Lobby;
        return true;
    }

    bool GalaxyEggbertGame::NetJoinGame(int index)
    {
        auto& n = *net_;
        if (index < 0 || !n.session.Join(index))
        {
            n.message = n.session.LastError().empty() ? "Could not join that game." : n.session.LastError();
            n.findSeconds = kFindEverySeconds;
            return false;
        }
        n.lobby = Net::Lobby{};
        n.chat.clear();
        n.helloSent = false;
        n.lobbySeconds = 0.0f;
        n.message.clear();
        n.screen = NetGame::Screen::Lobby;
        return true;
    }

    void GalaxyEggbertGame::NetSetReady(bool ready)
    {
        auto& n = *net_;
        if (!n.session.HasLocalGamer() || n.lobby.SlotOf(n.session.LocalId()) < 0) return;
        if (n.session.IsHost())
        {
            n.lobby.SetReady(n.session.LocalId(), ready);
            NetBroadcastLobby();
            return;
        }
        Net::Message m;
        m.kind = Net::MessageKind::Ready;
        m.ready = ready;
        n.session.Send(m);
    }

    void GalaxyEggbertGame::NetStartAsHost()
    {
        auto& n = *net_;
        if (!n.session.IsHost() || !n.lobby.CanStart()) return;
        n.lobby.MatchStarted();
        NetBroadcastLobby();
        Net::Message start;
        start.kind = Net::MessageKind::Start;
        start.world = n.lobby.state.world;
        start.worldHash = n.contentHash;
        n.session.Send(start);
        n.session.StartGame();
        StartNetMatch(n.lobby.state.world);
    }

    void GalaxyEggbertGame::NetTestStep()
    {
        // GALAXY_NET_TEST=host|join (tools/net_two_players.sh): the screens gone through without a
        // pointer -- the host makes a game, the other joins the first found, both are ready, the
        // host starts once two are there; the match is then the --autoplay script's.
        static const char* const mode = std::getenv("GALAXY_NET_TEST");
        if (mode == nullptr || (std::strcmp(mode, "host") != 0 && std::strcmp(mode, "join") != 0)) return;
        const bool host = std::strcmp(mode, "host") == 0;
        if (!net_)
        {
            if (phase_ == GalaxyEggbert::Def::GamePhase::Init && fadeOutPhase_ == GalaxyEggbert::Def::GamePhase::None &&
                phaseTimeSeconds_ > 1.0f)
                OpenMultiplayer();
            return;
        }
        auto& n = *net_;
        if (n.match || !n.signedIn) return;
        if (n.screen == NetGame::Screen::Browser && !n.session.Active())
        {
            if (host) NetHostGame();
            else if (!n.session.Found().empty()) NetJoinGame(0);
            return;
        }
        if (n.screen != NetGame::Screen::Lobby || !n.session.HasLocalGamer()) return;
        const int slot = n.lobby.SlotOf(n.session.LocalId());
        if (slot < 0) return;
        // GALAXY_NET_WORLD: the mission the test's host chooses (one installed), else the first.
        if (host && n.session.IsHost())
            if (const char* forced = std::getenv("GALAXY_NET_WORLD"); forced != nullptr && *forced != 0)
            {
                const int world = std::atoi(forced);
                if (world != n.lobby.state.world && std::find(n.worlds.begin(), n.worlds.end(), world) != n.worlds.end())
                {
                    n.lobby.state.world = static_cast<std::uint16_t>(world);
                    NetBroadcastLobby();
                }
            }
        if (host && n.lobby.Present() < 2) return;
        if (!n.lobby.state.slots[static_cast<std::size_t>(slot)].ready && !n.testReadyAsked)
        {
            NetSetReady(true);
            n.testReadyAsked = true; // once; the host's next lobby shows it
        }
        if (host) NetStartAsHost();
    }

    void GalaxyEggbertGame::NetLeaveLobby()
    {
        auto& n = *net_;
        n.session.Leave();
        n.screen = NetGame::Screen::Browser;
        n.typing = false;
        n.selected = -1;
        n.message.clear();
        n.findSeconds = kFindEverySeconds;
    }

    void GalaxyEggbertGame::DrawNetScreen(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int w, int h)
    {
        const auto layout = BuildNetScreen(w, h);
        inputPad_.DrawPanel(device, w, h, layout.title, layout.buttons, layout.lines,
                            net_->typing ? &net_->keyboard : nullptr, focusHighlight_);
    }

    // ---- The match ----

    void GalaxyEggbertGame::StartNetMatch(int worldNumber)
    {
        auto& n = *net_;
        const int slot = n.session.HasLocalGamer() ? n.lobby.SlotOf(n.session.LocalId()) : -1;
        const auto bytes = ReadWorldBytes(MissionWorldPath(worldNumber));
        std::istringstream in(bytes ? *bytes : std::string{}, std::ios::in | std::ios::binary);
        if (slot < 0 || !bytes || !worldRuntime_.LoadFromVwrStream(in))
        {
            n.message = "Mission #" + std::to_string(worldNumber) + " could not be started.";
            return;
        }
        n.slot = slot;
        n.team = n.lobby.state.slots[static_cast<std::size_t>(slot)].team;
        n.world = worldNumber;
        n.peers.clear();
        n.scores = {};
        n.outCount = 0;
        n.scored = true;
        n.killer = Net::kNoKiller;
        for (int i = 0; i < Net::kMaxPlayers; ++i)
        {
            const auto& s = n.lobby.state.slots[static_cast<std::size_t>(i)];
            if (s.present)
            {
                auto& score = n.scores[static_cast<std::size_t>(i)];
                score.played = true;
                score.name = s.name;
                score.team = s.team;
            }
            if (!s.present || i == slot) continue;
            NetGame::Peer peer;
            peer.id = s.gamerId;
            peer.team = s.team;
            peer.name = s.name;
            n.peers.push_back(peer);
        }

        titleSceneActive_ = false;
        titleSceneRequested_ = false;
        winCheerActive_ = false;
        pendingSkate_ = {};
        teleportArrivalGuard_ = false;
        loadedMissionPath_.clear(); // no demo is recorded, no mission saved
        StopVehicleMotorSound();
        RebuildWorldPresentation();
        // PlayPrepare (decblock.cpp:1430-1438): the crates, the goals and the enemy Blupis go.
        auto& objects = worldRuntime_.GetMobileObjectsMutable();
        for (auto& object : objects)
            if (Net::RemovedInMultiplayer(static_cast<int>(object.type))) object.active = false;

        // ... ten lives, his team's bombs, at his team's start (decblock.cpp:1349-1353).
        interaction_ = InteractionSystem();
        // GALAXY_NET_LIVES (a test's short match: tools/net_two_players.sh), else the original's ten.
        int lives = Net::kMultiplayerLives;
        if (const char* forced = std::getenv("GALAXY_NET_LIVES")) lives = std::clamp(std::atoi(forced), 0, Net::kMultiplayerLives);
        interaction_.SetLives(lives);
        interaction_.SetPersoTeam(n.team);
        interaction_.SeedRandom(std::random_device{}());
        blupi_ = BlupiController();
        blupi_.SetSolidBoxes(worldRuntime_.SolidBigDecorBoxes());
        wasInHelicopter_ = false;
        hideGhosts_.clear();
        hideGhostTracking_ = false;
        std::optional<GalaxyEggbert::PlayerStart> start;
        try
        {
            start = GalaxyEggbert::FindPlayerStart(worldRuntime_.GetWorld(), static_cast<PlayerStartColor>(n.team));
            if (!start) start = GalaxyEggbert::FindPlayerStart(worldRuntime_.GetWorld(), PlayerStartColor::Yellow);
        }
        catch (const std::exception&)
        {
            start.reset();
        }
        if (start)
        {
            PlaceBlupi(static_cast<float>(start->x) - WorldRuntime::kWorldCenterX, static_cast<float>(start->y),
                       static_cast<float>(start->z) - WorldRuntime::kWorldCenterZ);
            blupi_.SetYaw(GalaxyEggbert::PlayerStartYaw(*start));
        }
        else
        {
            PlaceBlupi(GalaxyEggbert::Editor::kEditorStarterSpawnRenderX, GalaxyEggbert::Editor::kEditorStarterSpawnY,
                       GalaxyEggbert::Editor::kEditorStarterSpawnRenderZ);
        }
        chaseCamera_.Reset(blupi_.GetYaw());
        cameraSmoothedInitialized_ = false;
        missionStartPending_ = false;
        recordingResume_ = false;

        n.replicator.Begin(objects, slot);
        n.replicator.Snapshot(worldRuntime_.GetWorld(), objects);
        n.incoming.clear();
        n.soundsIn.clear();
        n.soundsOut.clear();
        n.liftsIn.clear();
        n.liftSteps.clear();
        n.soundsLogged = n.liftsLogged = 0;
        n.hostId = n.session.HostId();
        n.match = true;
        n.tick = 0;
        n.lost = n.won = false;
        n.localPaused = false;
        n.notices.clear();
        n.typing = false;
        n.screen = NetGame::Screen::None;
        SetPhase(GalaxyEggbert::Def::GamePhase::Play, /*bypassFade=*/true);
        std::cout << "GalaxyEggbertCNA: multiplayer: mission " << worldNumber << " starts, " << n.name << " in "
                  << Net::kTeamNames[n.team % Net::kTeams] << " with " << n.peers.size() << " others." << std::endl;
    }

    void GalaxyEggbertGame::NetBeforeStep()
    {
        auto& n = *net_;
        auto& world = worldRuntime_.GetWorldMutable();
        auto& objects = worldRuntime_.GetMobileObjectsMutable();
        for (const auto& event : n.incoming)
        {
            n.replicator.Apply(event, world, objects);
            if (event.kind == Net::EventKind::ObjectSpawned)
                std::cout << "GalaxyEggbertCNA: multiplayer: object " << event.a << " of another at (" << event.x << ", "
                          << event.y << ", " << event.z << ")." << std::endl;
        }
        n.incoming.clear();
        // The host's lifts (decmove.cpp:283-311, NetAscenseurSynchro): each at the start of the stop
        // the host's has just begun, so that a lift is where the host's is on every machine. The
        // original's copy of the step to every other lift of the same shape is not needed: the
        // host sends each of its own.
        for (const auto& lift : n.liftsIn)
        {
            if (lift.index < 0 || static_cast<std::size_t>(lift.index) >= objects.size()) continue;
            auto& obj = objects[static_cast<std::size_t>(lift.index)];
            if (!NetIsMovingLift(obj)) continue;
            obj.patrolStep = lift.step;
            obj.patrolTime = 0.0f;
            const bool atStart = lift.step == 1;
            obj.currentX = atStart ? obj.posStartX : obj.posEndX;
            obj.currentY = atStart ? obj.posStartY : obj.posEndY;
            obj.currentZ = atStart ? obj.posStartZ : obj.posEndZ;
            if (n.liftsLogged < 8)
            {
                ++n.liftsLogged;
                std::cout << "GalaxyEggbertCNA: multiplayer: lift #" << lift.index << " follows the host's (step "
                          << static_cast<int>(lift.step) << ")." << std::endl;
            }
        }
        n.liftsIn.clear();
        if (n.session.IsHost())
        {
            n.liftSteps.assign(objects.size(), -1);
            for (std::size_t i = 0; i < objects.size(); ++i)
                if (NetIsMovingLift(objects[i])) n.liftSteps[i] = objects[i].patrolStep;
        }
        n.replicator.Snapshot(world, objects);
        // The others' sounds, where they made them (decmove.cpp:183-190); then his own noted.
        for (const auto& made : n.soundsIn)
        {
            const auto channel = static_cast<GalaxyEggbert::Def::SoundChannel>(made.channel);
            if (made.stop) sound_.Stop(channel);
            else sound_.PlayRemote(channel, made.x, made.y, made.z);
            if (n.soundsLogged < 12)
            {
                ++n.soundsLogged;
                std::cout << "GalaxyEggbertCNA: multiplayer: sound " << static_cast<int>(made.channel)
                          << (made.stop ? " stopped" : "") << " of another at (" << made.x << ", " << made.y << ", "
                          << made.z << ")." << std::endl;
            }
        }
        n.soundsIn.clear();
        n.soundsOut.clear();
        sound_.SetOutbox(&n.soundsOut);
    }

    void GalaxyEggbertGame::NetAfterStep()
    {
        auto& n = *net_;
        NetCheckCloudTouch();
        // Who killed him (the statistics): the maker of the shot or of the bomb he met, or for a bomb
        // of the world the one player of its colour.
        if (const int index = interaction_.DeathObject(); index >= 0)
        {
            const auto& objects = worldRuntime_.GetMobileObjects();
            int killer = n.replicator.OwnerOf(static_cast<std::size_t>(index));
            if (killer < 0 && static_cast<std::size_t>(index) < objects.size())
            {
                const int type = static_cast<int>(objects[static_cast<std::size_t>(index)].type);
                if (type >= 200 && type <= 203)
                {
                    for (int i = 0; i < Net::kMaxPlayers; ++i)
                    {
                        const auto& score = n.scores[static_cast<std::size_t>(i)];
                        if (!score.played || i == n.slot || score.team != type - 200) continue;
                        killer = killer < 0 ? i : -2; // two of that colour: nobody's in particular
                    }
                }
            }
            if (killer >= 0 && killer < Net::kMaxPlayers && killer != n.slot) n.killer = static_cast<std::uint8_t>(killer);
        }
        sound_.SetOutbox(nullptr);
        for (std::size_t first = 0; first < n.soundsOut.size(); first += Net::kMaxSounds)
        {
            Net::Message m;
            m.kind = Net::MessageKind::Sounds;
            for (std::size_t i = first; i < std::min(n.soundsOut.size(), first + Net::kMaxSounds); ++i)
            {
                const auto& made = n.soundsOut[i];
                m.sounds.push_back({GalaxyEggbert::Def::ToRaw(made.channel), made.stop, made.x, made.y, made.z});
            }
            n.session.Send(m);
        }
        n.soundsOut.clear();
        // The host: each lift whose stop at an end has just begun (decmove.cpp:571-579, 620-628).
        if (n.session.IsHost())
        {
            const auto& objects = worldRuntime_.GetMobileObjects();
            Net::Message lifts;
            lifts.kind = Net::MessageKind::Lifts;
            for (std::size_t i = 0; i < objects.size() && i < n.liftSteps.size(); ++i)
            {
                const int step = objects[i].patrolStep;
                if (n.liftSteps[i] < 0 || step == n.liftSteps[i] || (step != 1 && step != 3) || !NetIsMovingLift(objects[i]))
                    continue;
                lifts.lifts.push_back({static_cast<std::int32_t>(i), static_cast<std::uint8_t>(step)});
                if (lifts.lifts.size() == Net::kMaxLifts)
                {
                    n.session.Send(lifts);
                    lifts.lifts.clear();
                }
            }
            if (!lifts.lifts.empty()) n.session.Send(lifts);
        }
        const auto events = n.replicator.Diff(worldRuntime_.GetWorld(), worldRuntime_.GetMobileObjects());
        for (const auto& event : events)
        {
            // What this machine changed in the world, as sent (rare: a pickup, a bomb, a wall).
            if (event.kind == Net::EventKind::ObjectSpawned)
                std::cout << "GalaxyEggbertCNA: multiplayer: object " << event.a << " sent from (" << event.x << ", "
                          << event.y << ", " << event.z << ")." << std::endl;
            else if (event.kind == Net::EventKind::ObjectRetired)
                std::cout << "GalaxyEggbertCNA: multiplayer: object #" << event.id << " gone, sent." << std::endl;
            else if (event.kind == Net::EventKind::SpawnRetired)
                std::cout << "GalaxyEggbertCNA: multiplayer: object " << std::hex << event.id << std::dec << " gone, sent." << std::endl;
            else
                std::cout << "GalaxyEggbertCNA: multiplayer: block (" << event.a << ", " << event.b << ", " << event.c
                          << ") sent." << std::endl;
        }
        for (std::size_t first = 0; first < events.size(); first += Net::kMaxEvents)
        {
            Net::Message m;
            m.kind = Net::MessageKind::Events;
            m.events.assign(events.begin() + static_cast<std::ptrdiff_t>(first),
                            events.begin() + static_cast<std::ptrdiff_t>(std::min(events.size(), first + Net::kMaxEvents)));
            n.session.Send(m);
        }
        ++n.tick;
        if (n.tick % kPoseEveryTicks == 0 && !n.lost) NetSendPose();
        NetCheckWin();
    }

    void GalaxyEggbertGame::NetAgePeers(float dt)
    {
        if (!NetMatchActive()) return;
        for (auto& peer : net_->peers) peer.age += dt;
    }

    void GalaxyEggbertGame::NetSendPose()
    {
        auto& n = *net_;
        n.poseSeconds = 0.0f;
        Net::Message m;
        m.kind = Net::MessageKind::Pose;
        auto& pose = m.pose;
        pose.tick = n.tick;
        pose.x = blupi_.GetX();
        pose.y = blupi_.GetY();
        pose.z = blupi_.GetZ();
        auto charred = BlupiCharredShape();
        const bool glued = BlupiGlueModel() != nullptr;
        if (glued) charred = GalaxyEggbert::Game::CharredShape{true, 0.06f, 1.5f, false};
        const auto world = BlupiModelWorld(charred);
        pose.world = {world.M11, world.M12, world.M13, world.M21, world.M22, world.M23,
                      world.M31, world.M32, world.M33, world.M41, world.M42, world.M43};
        const auto clip = std::lower_bound(n.clipNames.begin(), n.clipNames.end(), blupiActiveClipName_);
        const bool known = clip != n.clipNames.end() && *clip == blupiActiveClipName_;
        pose.clip = known ? static_cast<std::uint16_t>(clip - n.clipNames.begin()) : 0;
        pose.clipSeconds = static_cast<float>(std::fmod(blupiClipTimeSeconds_, 1000.0));
        pose.alpha = static_cast<std::uint8_t>(std::clamp(blupi_.GetDrawOpacity(worldRuntime_.GetAnimPhase()), 0.0f, 1.0f) * 255.0f);
        // The original draws no remote Blupi whose icon is -1 (decblock.cpp:1256): hidden, dying as
        // a sprite, between lives.
        const bool hidden = !known || !blupiModelLoaded_ || (blupiModelIsReal_ && blupi_.IsAnimIconInvisible()) ||
                            BlupiDrawnAsDeathSprite() || blupi_.IsDeathHidden();
        pose.flags = static_cast<std::uint8_t>((hidden ? Net::kPoseHidden : 0) | (charred.charred && !glued ? Net::kPoseCharred : 0) |
                                               (glued ? Net::kPoseGlued : 0));
        pose.lives = static_cast<std::uint8_t>(std::clamp(interaction_.Lives(), 0, Net::kMultiplayerLives));
        // m_blupiSec (decblock.cpp:1733-1778): the power he has; none while dying or teleporting.
        pose.aura = blupi_.IsDeathLocked() || blupi_.IsTeleporting() ? 0 : GalaxyEggbert::Def::ToRaw(blupi_.GetSecretPower());
        n.session.Send(m);
    }

    void GalaxyEggbertGame::NetCheckWin()
    {
        auto& n = *net_;
        if (!n.match || n.lost || n.won || phase_ != GalaxyEggbert::Def::GamePhase::Play) return;
        int present = 1; // he himself
        for (const auto& peer : n.peers) present += peer.present ? 1 : 0;
        if (!Net::LastSurvivor(present)) return;
        // decblupi.cpp:4097-4125: sound 14, ACTION_WIN, and at its end the win screen.
        n.won = true;
        n.scores[static_cast<std::size_t>(n.slot)].won = true;
        NetLogResults();
        NetNotice(std::string(Net::kTextWon));
        sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel14);
        if (blupi_.IsInVehicle()) blupi_.TriggerDismount();
        if (blupi_.TriggerOneShotAnim(BlupiController::AnimState::Win, BlupiController::kWinDuration))
        {
            winCheerActive_ = true;
            const float dx = camera_.Position().X - blupi_.GetX();
            const float dz = camera_.Position().Z - blupi_.GetZ();
            blupi_.SetYaw(std::atan2(dx, -dz));
        }
        else SetPhase(GalaxyEggbert::Def::GamePhase::Win);
    }

    void GalaxyEggbertGame::NetSendDie(int livesLeft)
    {
        if (!NetMatchActive()) return;
        Net::Message m;
        m.kind = Net::MessageKind::Die;
        m.lives = static_cast<std::uint8_t>(std::clamp(livesLeft, 0, Net::kMultiplayerLives));
        m.killer = net_->killer;
        net_->killer = Net::kNoKiller;
        NetScoreDeath(net_->slot, m.killer);
        net_->session.Send(m);
    }

    void GalaxyEggbertGame::NetSendLost()
    {
        if (!NetMatchActive() || net_->lost) return;
        net_->lost = true;
        Net::Message m;
        m.kind = Net::MessageKind::Lost;
        m.killer = net_->killer;
        net_->killer = Net::kNoKiller;
        net_->session.Send(m);
        NetScoreDeath(net_->slot, m.killer); // the last death (PK_LOST alone, no PK_DIE)
        NetScoreOut(net_->slot);
        NetLogResults();
    }

    void GalaxyEggbertGame::LeaveNetMatch()
    {
        if (!net_) return;
        auto& n = *net_;
        n.session.Leave();
        n.match = false;
        n.peers.clear();
        n.incoming.clear();
        n.soundsIn.clear();
        n.soundsOut.clear();
        n.liftsIn.clear();
        n.liftSteps.clear();
        n.soundsLogged = n.liftsLogged = 0;
        n.hostId = n.session.HostId();
        sound_.SetOutbox(nullptr);
        n.notices.clear();
        n.screen = NetGame::Screen::None;
        interaction_ = InteractionSystem(); // his ten lives stay in the network game
        StopVehicleMotorSound();
    }

    void GalaxyEggbertGame::NetPhaseChanged(GalaxyEggbert::Def::GamePhase previous, GalaxyEggbert::Def::GamePhase next)
    {
        using GalaxyEggbert::Def::GamePhase;
        if (!NetMatchActive()) return;
        auto& n = *net_;
        // PK_PAUSE (event.cpp:1565-1580): a pause stops everyone's game, going on starts it again.
        const bool paused = next == GamePhase::Pause || next == GamePhase::PlaySetup;
        if (paused != n.localPaused && (next == GamePhase::Play || paused) && !n.lost)
        {
            n.localPaused = paused;
            Net::Message m;
            m.kind = Net::MessageKind::Pause;
            m.ready = paused;
            n.session.Send(m);
        }
        // Pause's Menu: PK_ABANDON (event.cpp:2737-2742), the game left.
        if (next == GamePhase::Init && previous != GamePhase::Win && previous != GamePhase::Lost)
        {
            Net::Message m;
            m.kind = Net::MessageKind::Abandon;
            if (n.localPaused)
            {
                Net::Message going;
                going.kind = Net::MessageKind::Pause;
                going.ready = false;
                n.session.Send(going);
            }
            n.session.Send(m);
            n.session.Update(); // flushed before the session goes
            LeaveNetMatch();
        }
    }

    float GalaxyEggbertGame::NetPoseBlend(const NetGame& n, std::size_t index)
    {
        const auto& peer = n.peers[index];
        const auto& p = peer.pose;
        const auto& q = peer.previous;
        const float jump = std::fabs(p.world[9] - q.world[9]) + std::fabs(p.world[10] - q.world[10]) +
                           std::fabs(p.world[11] - q.world[11]);
        if (jump > 3.0f) return 1.0f; // a new life, a teleport: there at once
        return std::clamp(peer.age / std::max(peer.interval, 0.02f), 0.0f, 1.0f);
    }

    std::vector<GalaxyEggbertGame::NetAura> GalaxyEggbertGame::NetPeerAuras() const
    {
        using GalaxyEggbert::Def::SecretPower;
        std::vector<NetAura> auras;
        if (!NetMatchActive() || phase_ == GalaxyEggbert::Def::GamePhase::Editor) return auras;
        const auto& n = *net_;
        for (std::size_t i = 0; i < n.peers.size(); ++i)
        {
            const auto& peer = n.peers[i];
            const auto& p = peer.pose;
            const auto power = GalaxyEggbert::Def::ToSecretPower(p.aura);
            if (!peer.present || !peer.hasPose || (p.flags & Net::kPoseHidden) != 0 ||
                (power != SecretPower::Shield && power != SecretPower::Power && power != SecretPower::Cloud))
                continue;
            const float t = NetPoseBlend(n, i);
            const auto lerp = [t](float a, float b) { return a + (b - a) * t; };
            NetAura aura;
            aura.centre = Microsoft::Xna::Framework::Vector3(lerp(peer.previous.x, p.x),
                                                             lerp(peer.previous.y, p.y) - 0.5f + 30.0f / 64.0f,
                                                             lerp(peer.previous.z, p.z));
            aura.power = power;
            aura.rank = static_cast<int>(i);
            auras.push_back(aura);
        }
        return auras;
    }

    void GalaxyEggbertGame::NetCheckCloudTouch()
    {
        using GalaxyEggbert::Def::SecretPower;
        auto& n = *net_;
        if (!n.match || n.lost || n.won || phase_ != GalaxyEggbert::Def::GamePhase::Play) return;
        // decblupi.cpp:3838-3841: not swimming, surfing, shielded, a cloud, hidden, Super Blupi; focused.
        const auto mine = blupi_.GetSecretPower();
        if (blupi_.IsNage() || blupi_.IsSurf() || mine == SecretPower::Shield || mine == SecretPower::Cloud ||
            mine == SecretPower::Hide || blupi_.GetCheatSuperBlupi() || !BlupiFocusRaw())
            return;
        // NetPlayerCollide (decmove.cpp:314-345): the two Blupis' rects (28 x 47 px of his 60 px
        // sprite) meet, the cloud's grown by 40 px each way: centres within 68 px across (here in X
        // and in Z) and 87 px up and down. The original takes the first player met, cloud or not;
        // here any cloud met (the order of the others is no rule of the game).
        constexpr float kAcross = 68.0f / 64.0f;
        constexpr float kUpDown = 87.0f / 64.0f;
        for (std::size_t i = 0; i < n.peers.size(); ++i)
        {
            const auto& peer = n.peers[i];
            if (!peer.present || !peer.hasPose || GalaxyEggbert::Def::ToSecretPower(peer.pose.aura) != SecretPower::Cloud) continue;
            const float t = NetPoseBlend(n, i);
            const float x = peer.previous.x + (peer.pose.x - peer.previous.x) * t;
            const float y = peer.previous.y + (peer.pose.y - peer.previous.y) * t;
            const float z = peer.previous.z + (peer.pose.z - peer.previous.z) * t;
            if (std::fabs(x - blupi_.GetX()) >= kAcross || std::fabs(z - blupi_.GetZ()) >= kAcross ||
                std::fabs(y - blupi_.GetY()) >= kUpDown)
                continue;
            Net::Message stop;
            stop.kind = Net::MessageKind::StopCloud;
            stop.target = peer.id;
            n.session.Send(stop);
            // ACTION 57 (the electrocuted Blupi, m_blupiRestart, a network game's restart where he
            // stood safely): his vehicle left where he is (ObjectStart), SOUND_ELECTRO (59). Galaxy
            // has no clip of the electrocution: he dies as by lightning (Clear1, BD-0120).
            n.killer = static_cast<std::uint8_t>(NetSlotOf(peer.id));
            DismountAndDepositVehicle();
            StopVehicleMotorSound();
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel59);
            blupi_.TriggerDeathLock(GalaxyEggbert::Game::BlupiController::DeathCause::Clear1, true);
            std::cout << "GalaxyEggbertCNA: multiplayer: " << peer.name << "'s cloud electrocutes us." << std::endl;
            return;
        }
    }

    void GalaxyEggbertGame::NetScoreDeath(int slot, std::uint8_t killer)
    {
        if (!net_ || slot < 0 || slot >= Net::kMaxPlayers) return;
        auto& n = *net_;
        ++n.scores[static_cast<std::size_t>(slot)].deaths;
        if (killer < Net::kMaxPlayers && killer != slot && n.scores[killer].played) ++n.scores[killer].kills;
    }

    void GalaxyEggbertGame::NetScoreOut(int slot)
    {
        if (!net_ || slot < 0 || slot >= Net::kMaxPlayers) return;
        auto& score = net_->scores[static_cast<std::size_t>(slot)];
        if (score.played && score.out == 0 && !score.won) score.out = ++net_->outCount;
    }

    std::vector<std::string> GalaxyEggbertGame::NetResultLines() const
    {
        // "1st  Ann (Yellow)   2 kills, 3 deaths": the winner, then who went out last, ...; those still
        // playing (seen from the Lost screen) first, without a place.
        std::vector<std::string> lines;
        if (!net_ || !net_->scored) return lines;
        const auto& n = *net_;
        std::vector<std::pair<int, int>> order; // (place, slot); place 0: still playing
        for (int i = 0; i < Net::kMaxPlayers; ++i)
            if (n.scores[static_cast<std::size_t>(i)].played) order.emplace_back(NetPlaceOf(i), i);
        std::sort(order.begin(), order.end());
        static const char* const kPlaces[] = {"", "1st", "2nd", "3rd", "4th"};
        for (const auto& [place, i] : order)
        {
            const auto& score = n.scores[static_cast<std::size_t>(i)];
            std::string line = place > 0 && place <= 4 ? kPlaces[place] : "playing";
            line += "  " + score.name + " (" + Net::kTeamNames[score.team % Net::kTeams] + ")  " + std::to_string(score.kills) +
                    (score.kills == 1 ? " kill, " : " kills, ") + std::to_string(score.deaths) +
                    (score.deaths == 1 ? " death" : " deaths");
            lines.push_back(line);
        }
        return lines;
    }

    int GalaxyEggbertGame::NetPlaceOf(int slot) const
    {
        // The winner 1; who went out last 2, ...; 0 still playing (or no match).
        if (!net_ || slot < 0 || slot >= Net::kMaxPlayers || !net_->scores[static_cast<std::size_t>(slot)].played) return 0;
        const auto& n = *net_;
        int played = 0, in = 0, lastIn = -1;
        for (int i = 0; i < Net::kMaxPlayers; ++i)
        {
            const auto& score = n.scores[static_cast<std::size_t>(i)];
            if (!score.played) continue;
            ++played;
            if (score.out == 0)
            {
                ++in;
                lastIn = i;
            }
        }
        const auto& score = n.scores[static_cast<std::size_t>(slot)];
        if (score.won || (in == 1 && slot == lastIn && played > 1)) return 1;
        return score.out > 0 ? played - score.out + 1 : 0;
    }

    void GalaxyEggbertGame::NetLogResults() const
    {
        for (const auto& line : NetResultLines()) std::cout << "GalaxyEggbertCNA: multiplayer: result: " << line << std::endl;
    }

    void GalaxyEggbertGame::NetReturnToLobby()
    {
        // A rematch (a Galaxy addition, Q-021): from the win or lost screen back to the lobby of the
        // same game, the others' places kept, to choose a mission and play again. The original closed
        // the network game there (event.cpp:2688-2700).
        if (!net_) return;
        auto& n = *net_;
        n.match = false;
        n.peers.clear();
        n.incoming.clear();
        n.soundsIn.clear();
        n.soundsOut.clear();
        n.liftsIn.clear();
        sound_.SetOutbox(nullptr);
        n.notices.clear();
        n.localPaused = false;
        n.typing = false;
        n.message.clear();
        n.lobbySeconds = 0.0f;
        n.testReadyAsked = false;
        n.screen = NetGame::Screen::Lobby;
        interaction_ = InteractionSystem();
        StopVehicleMotorSound();
        const std::uint8_t me = n.session.LocalId();
        if (n.session.IsHost())
        {
            n.lobby.Back(me);
            n.lobby.SetReady(me, false);
            n.session.EndGame();
            NetBroadcastLobby();
        }
        else
        {
            Net::Message ready;
            ready.kind = Net::MessageKind::Ready;
            ready.ready = false;
            n.session.Send(ready);
        }
        std::cout << "GalaxyEggbertCNA: multiplayer: back in the lobby." << std::endl;
    }

    int GalaxyEggbertGame::NetSlotOf(std::uint8_t gamerId) const noexcept
    {
        return net_ ? net_->lobby.SlotOf(gamerId) : -1;
    }

    bool GalaxyEggbertGame::NetIsMovingLift(const GalaxyEggbert::Game::MobileObjSpec& obj) noexcept
    {
        // The original's: a lift whose ends differ (decmove.cpp:569).
        return obj.active && GetObjectDefinition(obj.type).semanticKind == ObjectSemanticKind::Lift &&
               (obj.posStartX != obj.posEndX || obj.posStartY != obj.posEndY || obj.posStartZ != obj.posEndZ);
    }

    bool GalaxyEggbertGame::NetRemotePaused() const noexcept
    {
        if (!NetMatchActive()) return false;
        for (const auto& peer : net_->peers)
            if (peer.present && peer.paused) return true;
        return false;
    }
}
