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
        constexpr const char* kAddressFile = "multiplayer.txt"; // in the user folder: the address, the server
        constexpr const char* kDefaultServerPort = "47831";      // cna-gamer-services-server's

        // The gamer services server's endpoint from what a player types (host or host:port; a full
        // URL as it is): HTTPS, plain HTTP only on this machine's loopback (a test's server).
        std::string ServerEndpoint(const std::string& typed)
        {
            if (typed.empty() || typed.find("://") != std::string::npos) return typed;
            std::string host = typed;
            const bool bracketed = !host.empty() && host.front() == '[';
            const auto colon = bracketed ? host.find("]:") : host.rfind(':');
            if (colon == std::string::npos || (!bracketed && host.find(':') != colon)) host += std::string(":") + kDefaultServerPort;
            const bool loopback = host.rfind("127.0.0.1", 0) == 0 || host.rfind("[::1]", 0) == 0;
            return (loopback ? "http://" : "https://") + host + "/cna/v1";
        }
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
            kJoin = 10, kNewGame, kUpdate, kBack, kAddress, kServer, kInternet,
            kColour = 20, kReady, kChat, kStart, kLeave, kWorldPrev, kWorldNext, kLives, kTimeLimit,
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
        // Signed in automatically as the selected player (owner Q-019 Q6), by the nickname set in the
        // Setup (2026-11-11) or as "Player A"; a test names itself.
        const char* forced = std::getenv("GALAXY_NET_NAME");
        n.name = Net::CleanText(forced != nullptr && *forced != 0 ? std::string(forced)
                                                                  : saveData_.GetPlayerName(saveData_.GetSelectedGamer()),
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
        // The selected player's worlds of the editor, the host's to play (Q-022): slot n "My world n".
        n.customWorlds.clear();
        for (int slot = 1; slot <= GalaxyEggbert::Editor::kCustomWorldSlotCount; ++slot)
        {
            const auto path = GalaxyEggbert::Editor::SlotWorldPath(saveData_.GetSelectedGamer(), slot);
            std::error_code error;
            if (std::filesystem::is_regular_file(path, error))
                n.customWorlds.push_back({TrFormat("My world {0}", {std::to_string(slot)}), path});
        }
        if (const char* forced = std::getenv("GALAXY_NET_CUSTOM"); forced != nullptr && *forced != 0)
            n.customWorlds.push_back({std::filesystem::path(forced).stem().string(), forced}); // a test's
        if (!NetSession::Available()) n.message = Tr("This version has no network game.");
        else if (n.worlds.empty()) n.message = Tr("No multiplayer world is installed.");
        // multiplayer.txt: the host's address searched too, then the gamer services server (Q-022);
        // GALAXY_NET_ADDRESS and GALAXY_NET_SERVER for the tests.
        std::string keptAddress, keptServer;
        if (std::ifstream in(GalaxyEggbert::Game::UserPath(kAddressFile)); in)
        {
            std::getline(in, keptAddress);
            std::getline(in, keptServer);
        }
        if (const char* forced = std::getenv("GALAXY_NET_SERVER"); forced != nullptr) keptServer = forced;
        n.server = keptServer;
        if (!n.server.empty() && !n.session.SetServer(ServerEndpoint(n.server)))
            n.message = TrFormat("Not a server: {0} ({1}).", {n.server, n.session.LastError()});
        n.signedIn = n.session.SignIn(n.name);
        n.findSeconds = kFindEverySeconds; // the first search at once
        if (const char* forced = std::getenv("GALAXY_NET_ADDRESS"); forced != nullptr)
            NetSetAddress(forced, false);
        else
            NetSetAddress(keptAddress, false);
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
        if (NetRemotePaused()) lines.emplace_back(Tr(Net::kTextPause));
        if (net_->typing) lines.insert(lines.begin(), TrFormat("Say:  {0}", {net_->draft + "_"})); // the chat line typed (Enter)
        // The time limit's clock (Q-022), first.
        if (net_->limitSeconds > 0.0f && !net_->timeUp)
            lines.insert(lines.begin(), TrFormat("Time left {0}", {Net::ClockText(net_->limitSeconds - net_->matchSeconds)}));
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
        if (!n.signedIn && n.screen != NetGame::Screen::None)
        {
            n.signedIn = n.session.SignIn(n.name);
            // Signed in with an account (a server, Q-022): its gamertag is his name in the games.
            if (n.signedIn && n.session.Online())
            {
                n.name = Net::CleanText(n.session.LocalName(), Net::kMaxName);
                std::cout << "GalaxyEggbertCNA: multiplayer: signed in at the server as " << n.name << "." << std::endl;
                if (const char* internet = std::getenv("GALAXY_NET_INTERNET"); internet != nullptr && *internet == '1')
                    n.session.SetInternet(true);
            }
        }

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
                    NetSessionEnded(TrMark("the host does not answer"));
                    return;
                }
                peer.present = false;
                peer.paused = false;
                NetNotice(Net::LeftNotice(peer.name));
                NetScoreOut(peer.slot);
                NetMarkLeft(peer.slot);
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
                n.message = Tr("The game did not take us (full, or another version).");
            }
        }
        if (n.match && n.typing)
        {
            // The chat line typed over the play (Enter sends, Escape gives it up).
            for (const auto& [key, shift] : n.typedKeys)
            {
                if (key == Microsoft::Xna::Framework::Input::Keys::Escape)
                {
                    NetFinishTyping(false);
                    break;
                }
                if (ScreenKeyboard::TypeKey(key, shift, n.draft, kChatInput) == ScreenKeyboard::Result::Done)
                {
                    NetFinishTyping(true);
                    break;
                }
            }
            n.typedKeys.clear();
        }
        if (n.match && n.limitSeconds > 0.0f && !n.timeUp && !NetRemotePaused() && !n.localPaused)
        {
            // The time limit's clock: the host's decides (each runs its own for the display).
            n.matchSeconds += std::min(dt, 0.25f);
            if (n.session.IsHost() && n.matchSeconds >= n.limitSeconds) NetTimeUp();
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
            if (!host) return;
            if (n.match)
            {
                NetTryRejoin(sender, m); // only one who dropped from this match, at his place
                return;
            }
            if (m.contentHash != n.contentHash || m.simulationRevision != Net::kProtocolVersion)
            {
                std::cout << "GalaxyEggbertCNA: multiplayer: " << m.name << " has other content; not taken." << std::endl;
                return;
            }
            std::cout << "GalaxyEggbertCNA: multiplayer: " << m.name << " says hello, place "
                      << n.lobby.Add(sender, m.name) << "." << std::endl;
            NetBroadcastLobby();
            if (n.lobby.state.customHash != 0) NetSendCustomWorld(); // (to everyone: the pieces are small)
            return;
        case Net::MessageKind::Lobby:
            // The host's alone (another's would rewrite the places or start nothing of his).
            if (!host && sender == n.session.HostId()) n.lobby.state = m.lobby;
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
            if (n.match) NetNotice(m.text); // over the play (Q-022)
            return;
        case Net::MessageKind::Start:
            if (!host && sender == n.session.HostId() && n.screen == NetGame::Screen::Lobby && !n.match)
            {
                n.lobby.state.lives = m.lives; // the settings the host started with
                n.lobby.state.minutes = m.minutes;
                n.lobby.state.customHash = m.customHash;
                StartNetMatch(m.world);
            }
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
            // From a player of this match (not one who joined the session meanwhile); one just knocked
            // out too: his last step's changes come after his PK_LOST.
            if (peer() != nullptr && n.match)
                n.incoming.insert(n.incoming.end(), m.events.begin(), m.events.end());
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
                NetNoticeKill(m.killer, NetSlotOf(sender));
                NetScoreDeath(NetSlotOf(sender), m.killer);
            }
            return;
        case Net::MessageKind::Lost:
        case Net::MessageKind::Abandon:
            if (auto* p = peer(); p != nullptr && n.match && p->present)
            {
                p->present = false;
                NetNotice(m.kind == Net::MessageKind::Lost ? Net::LostNotice(p->name) : Net::LeftNotice(p->name));
                if (m.kind == Net::MessageKind::Lost) NetNoticeKill(m.killer, NetSlotOf(sender));
                if (m.kind == Net::MessageKind::Lost) NetScoreDeath(NetSlotOf(sender), m.killer);
                NetScoreOut(p->slot);
                if (m.kind == Net::MessageKind::Abandon) NetMarkLeft(p->slot);
            }
            return;
        case Net::MessageKind::TimeUp:
            if (!host && sender == n.session.HostId() && n.match) NetApplyTimeUp(m.places);
            return;
        case Net::MessageKind::WorldData:
            if (!host && sender == n.session.HostId()) NetReceiveWorldData(m);
            return;
        case Net::MessageKind::Rejoin:
            if (host || sender != n.session.HostId()) return;
            if (n.session.HasLocalGamer() && m.target == n.session.LocalId())
            {
                // We are back in the match we dropped from: as it is now.
                if (n.match || n.screen != NetGame::Screen::Lobby) return;
                n.rejoin = m;
                n.lobby.state.lives = m.lives;
                n.lobby.state.minutes = m.minutes;
                n.lobby.state.customHash = m.customHash;
                StartNetMatch(m.world);
                n.rejoin.reset();
            }
            else if (n.match)
                NetPeerBack(m.rejoinSlot, m.target);
            return;
        case Net::MessageKind::Snapshot:
            if (!host && sender == n.session.HostId() && n.match && n.session.HasLocalGamer() && m.target == n.session.LocalId())
                n.incoming.insert(n.incoming.end(), m.events.begin(), m.events.end());
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
            if (n.match)
            {
                NetScoreOut(p.slot >= 0 ? p.slot : slot);
                NetMarkLeft(p.slot >= 0 ? p.slot : slot);
            }
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
        n.message = TrFormat("The network game ended: {0}.", {Tr(reason)});
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
            l.title = std::string(Tr(Net::kTextMultiplayer));
            if (n.typing && n.typingServer)
            {
                line(TrFormat("Server:  {0}", {n.draft + "_"}), left, 268.0f, 0.5f);
                line(std::string(Tr("The gamer services server for internet games: its name or address, :port if not 47831.")), left,
                     300.0f, 0.42f, true);
                line(std::string(Tr("Empty: none. OK keeps it; the game uses it from its next start.")), left, 322.0f, 0.42f, true);
                return l;
            }
            if (n.typing)
            {
                line(TrFormat("Host address:  {0}", {n.draft + "_"}), left, 268.0f, 0.5f);
                line(std::string(Tr("Its IP address or name (192.168.1.20, a ZeroTier address, a router's address).")), left, 300.0f,
                     0.42f, true);
                line(std::string(Tr("Empty: the local network only. OK keeps it.")), left, 322.0f, 0.42f, true);
                return l;
            }
            line(TrFormat(n.signedIn ? "You play as {0}." : "Signing in as {0} ...", {n.name}), left, 62.0f);
            if (NetSession::ServerSupport())
            {
                // Internet play (Q-022): the server to set (it starts with the game), or with one the
                // internet's games or the local network's.
                PanelButton server;
                server.rect = rect(20.0f, 80.0f, 320.0f, 108.0f);
                server.label = !n.session.Online() ? (n.server.empty() ? std::string(Tr("Server: none")) : TrFormat("Server: {0}", {n.server}))
                               : std::string(!n.signedIn ? Tr("Sign in at the server")
                                             : n.session.Internet() ? Tr("Internet games (local: here)")
                                                                    : Tr("Local games (internet: here)"));
                server.size = 0.42f;
                server.enabled = !n.session.Active();
                l.buttons.push_back(server);
                l.actions.push_back(n.session.Online() ? kInternet : kServer);
            }
            if (NetSession::AddressSearch() && !n.session.Internet())
            {
                PanelButton address;
                address.rect = rect(330.0f, 80.0f, 620.0f, 108.0f);
                address.label = n.address.empty() ? std::string(Tr("Host address: none")) : TrFormat("Host address: {0}", {n.address});
                address.size = 0.42f;
                address.enabled = !n.session.Active();
                l.buttons.push_back(address);
                l.actions.push_back(kAddress);
            }
            line(std::string(n.session.Internet() ? Tr("Available games on the internet :") : Tr(Net::kTextAvailableGames)),
                 left, 120.0f);
            const auto& found = n.session.Found();
            for (int i = 0; i < kListRows && i < static_cast<int>(found.size()); ++i)
            {
                const auto& game = found[static_cast<std::size_t>(i)];
                PanelButton row;
                row.rect = rect(20.0f, 130.0f + 40.0f * static_cast<float>(i), 620.0f, 164.0f + 40.0f * static_cast<float>(i));
                row.label = game.host + "   -   " + std::to_string(game.players) + " / " +
                            std::to_string(Net::kMaxPlayers) + "   -   " + Net::MissionTitle(game.world);
                row.size = 0.5f;
                row.row = true;
                row.held = n.selected == i;
                l.buttons.push_back(row);
                l.actions.push_back(kRow0 + i);
            }
            if (found.empty())
                line(n.signedIn ? std::string(Tr("No game found yet. Make a new one, or wait for one.")) : std::string(), centre,
                     200.0f, 0.45f, true, true);
            if (!n.message.empty()) line(n.message, centre, 384.0f, 0.45f, false, true);
            button(kJoin, 20.0f, 215.0f, std::string(Tr(Net::kTextJoin)), n.selected >= 0 && n.signedIn);
            button(kNewGame, 225.0f, 375.0f, std::string(Tr(Net::kTextNewGame)), n.signedIn && !n.worlds.empty());
            button(kUpdate, 385.0f, 530.0f, std::string(Tr(Net::kTextUpdateList)), n.signedIn);
            button(kBack, 540.0f, 620.0f, std::string(Tr("Back")));
            return l;
        }

        // The lobby.
        const bool host = n.session.IsHost();
        const std::uint8_t me = n.session.LocalId();
        const int mySlot = n.session.HasLocalGamer() ? n.lobby.SlotOf(me) : -1;
        const bool ready = mySlot >= 0 && n.lobby.state.slots[static_cast<std::size_t>(mySlot)].ready;
        l.title = n.lobby.state.customHash != 0 ? TrFormat("World: {0}", {n.lobby.state.customName})
                                                : Net::MissionTitle(n.lobby.state.world);
        if (n.lobby.state.customHash != 0 && !host && !NetHasLobbyWorld())
        {
            std::size_t got = 0;
            for (const bool piece : n.customGot) got += piece ? 1 : 0;
            const std::size_t pieces = std::max<std::size_t>(n.customGot.size(), 1);
            line(TrFormat("Receiving the host's world ... {0} %", {std::to_string(100 * got / pieces)}), centre, 172.0f, 0.42f, true,
                 true);
        }
        if (n.typing)
        {
            line(std::string(Tr(Net::kTextSendMessage)) + ":  " + n.draft + "_", left, 268.0f, 0.5f);
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
        line(std::string(Tr(Net::kTextPlayerList)), left, 66.0f);
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
            if (i == 0) who += Tr("  (host)");
            if (i == mySlot) who += Tr("  (you)");
            line(who, left + 10.0f * s, y);
            line(std::string(Tr(Net::kTeamNames[slot.team % Net::kTeams])), centre + 40.0f * s, y);
            if (slot.playing) line(std::string(Tr("playing")), centre + 170.0f * s, y, 0.45f, true);
            else line(slot.ready ? std::string(Tr("Ready")) : std::string("..."), centre + 170.0f * s, y, 0.45f, !slot.ready);
            // The last match (Q-021): his place, kills / deaths.
            if (const auto& score = n.scores[static_cast<std::size_t>(i)]; n.scored && score.played && score.name == slot.name)
            {
                static const char* const kPlaces[] = {"-", TrMark("1st"), TrMark("2nd"), TrMark("3rd"), TrMark("4th")};
                const int place = NetPlaceOf(i);
                line(std::string(Tr(kPlaces[place >= 0 && place <= 4 ? place : 0])) + " " + std::to_string(score.kills) + "/" +
                         std::to_string(score.deaths),
                     centre + 235.0f * s, y, 0.42f, true);
            }
        }
        // The match's settings (Galaxy's, Q-022): the host's to change, everyone's to see.
        const std::string livesText = TrFormat("Lives: {0}", {std::to_string(n.lobby.state.lives)});
        const std::string timeText = Net::TimeLimitText(n.lobby.state.minutes);
        if (host)
        {
            for (const auto& [action, x0, x1, label] :
                 {std::tuple<int, float, float, std::string>{kLives, 20.0f, 170.0f, livesText},
                  std::tuple<int, float, float, std::string>{kTimeLimit, 180.0f, 400.0f, timeText}})
            {
                PanelButton b;
                b.rect = rect(x0, 188.0f, x1, 212.0f);
                b.label = label;
                b.size = 0.42f;
                b.enabled = !ready;
                l.buttons.push_back(b);
                l.actions.push_back(action);
            }
        }
        else line(livesText + "      " + timeText, left, 204.0f, 0.42f, true);
        line(std::string(Tr(Net::kTextChatZone)), left, 236.0f);
        for (std::size_t j = 0; j < n.chat.size(); ++j)
            line(n.chat[j], left + 10.0f * s, 258.0f + 20.0f * static_cast<float>(j), 0.42f);
        if (!n.message.empty()) line(n.message, centre, 384.0f, 0.45f, false, true);
        else if (mySlot < 0) line(std::string(Tr("Waiting for the host ...")), centre, 384.0f, 0.45f, true, true);
        else if (host && !n.lobby.CanStart())
            line(std::string(Tr("Start when every player is ready (two at least).")), centre, 384.0f, 0.42f, true, true);
        button(kColour, 20.0f, 130.0f, std::string(Tr("Colour")), mySlot >= 0 && !ready);
        button(kReady, 140.0f, 260.0f, std::string(ready ? Tr("Ready") : Tr(Net::kTextReady)),
               mySlot >= 0 && (ready || NetHasLobbyWorld()), ready);
        button(kChat, 270.0f, 420.0f, std::string(Tr(Net::kTextSendMessage)), mySlot >= 0);
        if (host) button(kStart, 430.0f, 520.0f, std::string(Tr("Start")), n.lobby.CanStart());
        button(kLeave, 530.0f, 620.0f, std::string(Tr("Leave")));
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
            n.message = TrFormat("Not an address: {0} ({1}).", {address, n.session.LastError()});
            return;
        }
        n.address = address;
        n.findSeconds = kFindEverySeconds; // searched again at once
        if (save)
        {
            n.message = address.empty() ? std::string(Tr("Games are searched on the local network."))
                                        : TrFormat("Games are searched at {0} too.", {address});
            NetSaveAddresses();
        }
    }

    void GalaxyEggbertGame::NetFinishTyping(bool send)
    {
        auto& n = *net_;
        n.typing = false;
        n.typedKeys.clear();
        // Over a match's play: the Escape or Enter that ended the line is still down; it does not
        // pause nor confirm.
        if (n.match)
        {
            pauseKeyWasDown_ = true;
            menuConfirmWasDown_ = true;
        }
        if (n.typingAddress)
        {
            n.typingAddress = false;
            if (send && n.typingServer) NetSetServer(n.draft);
            else if (send) NetSetAddress(n.draft, true);
            n.typingServer = false;
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
        if (n.match) NetNotice(chat.text);
        n.session.Send(chat);
    }

    void GalaxyEggbertGame::NetTypedKey(Microsoft::Xna::Framework::Input::Keys key, bool shift)
    {
        if (net_ && net_->typing && net_->typedKeys.size() < 64) net_->typedKeys.emplace_back(key, shift);
    }

    bool GalaxyEggbertGame::NetWantsText() const noexcept
    {
        return net_ && net_->typing && (net_->screen != NetGame::Screen::None || net_->match);
    }

    bool GalaxyEggbertGame::NetTypingInMatch() const noexcept { return NetMatchActive() && net_->typing; }

    void GalaxyEggbertGame::NetStartMatchTyping()
    {
        // A chat line during the match (Galaxy's, Q-022; SB2's chat was the lobby's): typed over the
        // play, the game going on; Enter sends it, Escape gives it up.
        auto& n = *net_;
        n.typing = true;
        n.typingAddress = false;
        n.draft.clear();
        n.typedKeys.clear();
    }

    void GalaxyEggbertGame::NetNoticeKill(std::uint8_t killer, int victimSlot)
    {
        auto& n = *net_;
        if (killer >= Net::kMaxPlayers || victimSlot < 0 || victimSlot >= Net::kMaxPlayers || killer == victimSlot) return;
        const auto& who = n.scores[killer];
        const auto& whom = n.scores[static_cast<std::size_t>(victimSlot)];
        if (!who.played || !whom.played) return;
        // "You" and "you" whole sentences of their own: a language puts them where its grammar wants.
        NetNotice(killer == n.slot       ? TrFormat("You got {0} !", {whom.name})
                  : victimSlot == n.slot ? TrFormat("{0} got you !", {who.name})
                                         : Net::KillNotice(who.name, whom.name));
    }

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
        case kServer:
            n.message.clear();
            NetStartTyping(w, h, true);
            n.typingServer = true;
            n.draft = n.server;
            n.keyboard.Begin(n.draft, false);
            return;
        case kInternet:
            if (!n.signedIn)
            {
                n.session.RetrySignIn(); // the Guide again
                return;
            }
            // Internet games (the server's) or the local network's; the list searched again.
            n.session.SetInternet(!n.session.Internet());
            n.selected = -1;
            n.findSeconds = kFindEverySeconds;
            n.message = n.session.Internet() ? Tr("Games on the internet, through the server.") : Tr("Games on the local network.");
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
        case kLives:
            // The original's ten, or fewer (Q-022): 10, 1, 2, ... 10.
            if (host && !n.match)
            {
                n.lobby.state.lives = static_cast<std::uint8_t>(n.lobby.state.lives % Net::kMultiplayerLives + 1);
                NetBroadcastLobby();
            }
            return;
        case kTimeLimit:
            if (host && !n.match)
            {
                const auto* const begin = std::begin(Net::kTimeLimits);
                const auto* const end = std::end(Net::kTimeLimits);
                const auto* it = std::find(begin, end, static_cast<int>(n.lobby.state.minutes));
                it = it == end || std::next(it) == end ? begin : std::next(it);
                n.lobby.state.minutes = static_cast<std::uint8_t>(*it);
                NetBroadcastLobby();
            }
            return;
        case kWorldPrev:
        case kWorldNext:
            if (host && !n.worlds.empty())
            {
                // The multiplayer worlds, then the host's of the editor (Q-022).
                const std::size_t count = n.worlds.size() + n.customWorlds.size();
                std::size_t index = 0;
                if (n.lobby.state.customHash != 0)
                {
                    for (std::size_t k = 0; k < n.customWorlds.size(); ++k)
                        if (n.customWorlds[k].name == n.lobby.state.customName) index = n.worlds.size() + k;
                }
                else if (auto it = std::find(n.worlds.begin(), n.worlds.end(), static_cast<int>(n.lobby.state.world));
                         it != n.worlds.end())
                    index = static_cast<std::size_t>(it - n.worlds.begin());
                index = action == kWorldNext ? (index + 1) % count : (index + count - 1) % count;
                NetChooseWorld(static_cast<int>(index));
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
            n.message = n.session.LastError().empty() ? std::string(Tr("Could not join that game.")) : n.session.LastError();
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
        if (ready && !NetHasLobbyWorld()) return; // the host's editor world not here yet
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
        start.lives = n.lobby.state.lives;
        start.minutes = n.lobby.state.minutes;
        start.customHash = n.lobby.state.customHash;
        n.session.Send(start);
        n.session.StartGame();
        StartNetMatch(n.lobby.state.world);
    }

    void GalaxyEggbertGame::NetTestStep()
    {
        // GALAXY_NET_TEST=host|join (tools/net_two_players.sh): the screens gone through without a
        // pointer -- the host makes a game, the other joins the first found, both are ready, the
        // host starts once two are there (GALAXY_NET_PLAYERS: more); the match is then the --autoplay
        // script's.
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
        // GALAXY_NET_PLAYERS: how many the test's host waits for (2-4, tools/net_players.sh), else two.
        static const int players = [] {
            const char* forced = std::getenv("GALAXY_NET_PLAYERS");
            return forced != nullptr && *forced != 0 ? std::clamp(std::atoi(forced), 2, Net::kMaxPlayers) : 2;
        }();
        // GALAXY_NET_CUSTOM: the host's test plays that world (its editor's, sent to the others).
        if (host && n.session.IsHost() && !n.customWorlds.empty() && n.lobby.state.customHash == 0 &&
            std::getenv("GALAXY_NET_CUSTOM") != nullptr)
            NetChooseWorld(static_cast<int>(n.worlds.size() + n.customWorlds.size() - 1));
        if (host && n.lobby.Present() < players) return;
        if (!NetHasLobbyWorld()) return;
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
        // A multiplayer world, or the host's of the editor (Q-022), whole here.
        const bool custom = n.lobby.state.customHash != 0;
        const auto bytes = custom ? (NetHasLobbyWorld() ? std::optional<std::string>(std::string(n.customBytes.begin(), n.customBytes.end()))
                                                        : std::optional<std::string>())
                                  : ReadWorldBytes(MissionWorldPath(worldNumber));
        std::istringstream in(bytes ? *bytes : std::string{}, std::ios::in | std::ios::binary);
        if (slot < 0 || !bytes || !worldRuntime_.LoadFromVwrStream(in))
        {
            n.message = custom ? TrFormat("The world {0} could not be started.", {n.lobby.state.customName})
                               : TrFormat("Mission #{0} could not be started.", {std::to_string(worldNumber)});
            std::cout << "GalaxyEggbertCNA: multiplayer: " << n.message << std::endl;
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
            peer.slot = i;
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
        // The host's lives (the original's ten unless changed, Q-022); GALAXY_NET_LIVES for a test's
        // short match (tools/net_players.sh). The time limit likewise (GALAXY_NET_SECONDS: in seconds).
        int lives = std::clamp(static_cast<int>(n.lobby.state.lives), 0, Net::kMultiplayerLives);
        if (const char* forced = std::getenv("GALAXY_NET_LIVES")) lives = std::clamp(std::atoi(forced), 0, Net::kMultiplayerLives);
        n.limitSeconds = 60.0f * static_cast<float>(n.lobby.state.minutes);
        if (const char* forced = std::getenv("GALAXY_NET_SECONDS")) n.limitSeconds = std::max(0.0f, static_cast<float>(std::atof(forced)));
        n.matchSeconds = 0.0f;
        n.timeUp = false;
        n.finalPlaces = {};
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
        simulationChaseCamera_.Reset(blupi_.GetYaw());
        cameraSmoothedInitialized_ = false; simulationCameraInitialized_ = false;
        missionStartPending_ = false;
        recordingResume_ = false;

        // Back after a drop (Q-022): his lives and the match's statistics and clock as the host has
        // them; his new objects numbered past those he made before.
        std::uint32_t firstSpawn = 1;
        if (n.rejoin)
        {
            const auto& r = *n.rejoin;
            interaction_.SetLives(r.lives);
            n.matchSeconds = r.seconds;
            firstSpawn = r.firstSpawn;
            n.outCount = 0;
            for (int i = 0; i < Net::kMaxPlayers; ++i)
            {
                auto& score = n.scores[static_cast<std::size_t>(i)];
                const auto& wire = r.scores[static_cast<std::size_t>(i)];
                if (!wire.played) continue;
                score.played = true;
                score.name = wire.name;
                score.team = wire.team;
                score.deaths = wire.deaths;
                score.kills = wire.kills;
                score.out = wire.out;
                score.left = wire.left;
                n.outCount = std::max(n.outCount, static_cast<int>(wire.out));
                for (auto& peer : n.peers)
                    if (peer.slot == i && (wire.out > 0)) peer.present = false;
            }
            std::cout << "GalaxyEggbertCNA: multiplayer: back in the match (" << static_cast<int>(r.lives) << " lives left)."
                      << std::endl;
            NetNotice(std::string(Tr("Back in the match !")));
        }
        n.replicator.Begin(objects, slot, firstSpawn);
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
        std::cout << "GalaxyEggbertCNA: multiplayer: "
                  << (custom ? "the world " + n.lobby.state.customName : "mission " + std::to_string(worldNumber)) << " starts, "
                  << n.name << " in "
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
            // Its type as it was (its slot already holds the pop of its end).
            int killer = n.replicator.OwnerOf(static_cast<std::size_t>(index));
            if (killer < 0)
            {
                const int type = static_cast<int>(interaction_.DeathObjectType());
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
        NetSendLiftSyncs();
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
        if (!Net::LastSurvivor(present) || n.timeUp) return;
        NetDeclareWin();
    }

    void GalaxyEggbertGame::NetDeclareWin()
    {
        auto& n = *net_;
        // decblupi.cpp:4097-4125: sound 14, ACTION_WIN, and at its end the win screen.
        n.won = true;
        n.scores[static_cast<std::size_t>(n.slot)].won = true;
        NetLogResults();
        if (!n.timeUp) NetNotice(std::string(Tr(Net::kTextWon)));
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

    void GalaxyEggbertGame::NetTimeUp()
    {
        // The host's ranks when the time limit is reached (Galaxy's, Q-022; SB2 has no time limit):
        // those still in by the lives they have left, then their kills, then the fewer deaths, the
        // same shared; then those already out, the last out first.
        auto& n = *net_;
        struct In
        {
            int slot, lives, kills, deaths;
        };
        std::vector<In> in;
        std::vector<std::pair<int, int>> out; // (the order he went out in, slot)
        for (int i = 0; i < Net::kMaxPlayers; ++i)
        {
            const auto& score = n.scores[static_cast<std::size_t>(i)];
            if (!score.played) continue;
            if (score.out > 0 || (i == n.slot && n.lost))
            {
                out.emplace_back(score.out, i);
                continue;
            }
            int lives = 0;
            if (i == n.slot) lives = interaction_.Lives();
            else
                for (const auto& peer : n.peers)
                    if (NetSlotOf(peer.id) == i) lives = peer.lives;
            in.push_back({i, lives, score.kills, score.deaths});
        }
        std::stable_sort(in.begin(), in.end(), [](const In& a, const In& b) {
            if (a.lives != b.lives) return a.lives > b.lives;
            if (a.kills != b.kills) return a.kills > b.kills;
            return a.deaths < b.deaths;
        });
        std::array<std::uint8_t, Net::kMaxPlayers> places{};
        for (std::size_t k = 0; k < in.size(); ++k)
        {
            const bool tied = k > 0 && in[k].lives == in[k - 1].lives && in[k].kills == in[k - 1].kills &&
                              in[k].deaths == in[k - 1].deaths;
            places[static_cast<std::size_t>(in[k].slot)] =
                tied ? places[static_cast<std::size_t>(in[k - 1].slot)] : static_cast<std::uint8_t>(k + 1);
        }
        std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
        std::uint8_t next = static_cast<std::uint8_t>(in.size() + 1);
        for (const auto& [order, slot] : out) places[static_cast<std::size_t>(slot)] = next++;
        Net::Message m;
        m.kind = Net::MessageKind::TimeUp;
        m.places = places;
        n.session.Send(m);
        NetApplyTimeUp(places);
    }

    void GalaxyEggbertGame::NetApplyTimeUp(const std::array<std::uint8_t, Net::kMaxPlayers>& places)
    {
        auto& n = *net_;
        if (n.timeUp || !n.match) return;
        n.timeUp = true;
        n.finalPlaces = places;
        for (int i = 0; i < Net::kMaxPlayers; ++i)
            if (places[static_cast<std::size_t>(i)] == 1 && n.scores[static_cast<std::size_t>(i)].played)
                n.scores[static_cast<std::size_t>(i)].won = true;
        NetNotice(std::string(Tr(Net::kTextTimeUp)));
        std::cout << "GalaxyEggbertCNA: multiplayer: the time is up." << std::endl;
        if (n.lost || n.won) // already out (his results now final) or won
        {
            NetLogResults();
            return;
        }
        if (places[static_cast<std::size_t>(n.slot)] == 1)
        {
            NetDeclareWin();
            return;
        }
        // Not first: the match is over for him as for one knocked out (nothing sent: all know).
        n.lost = true;
        NetLogResults();
        if (blupi_.IsInVehicle()) blupi_.TriggerDismount();
        SetPhase(GalaxyEggbert::Def::GamePhase::Lost, true);
    }

    void GalaxyEggbertGame::NetSendDie(int livesLeft)
    {
        if (!NetMatchActive()) return;
        Net::Message m;
        m.kind = Net::MessageKind::Die;
        m.lives = static_cast<std::uint8_t>(std::clamp(livesLeft, 0, Net::kMultiplayerLives));
        m.killer = net_->killer;
        net_->killer = Net::kNoKiller;
        NetNoticeKill(m.killer, net_->slot); // "Ann got you !"
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
        NetNoticeKill(m.killer, net_->slot);
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
        static const char* const kPlaces[] = {"", TrMark("1st"), TrMark("2nd"), TrMark("3rd"), TrMark("4th")};
        for (const auto& [place, i] : order)
        {
            const auto& score = n.scores[static_cast<std::size_t>(i)];
            std::string line(place > 0 && place <= 4 ? Tr(kPlaces[place]) : Tr("playing"));
            line += "  " + score.name + " (" + std::string(Tr(Net::kTeamNames[score.team % Net::kTeams])) + ")  " +
                    TrCount("{0} kill, ", "{0} kills, ", score.kills, {std::to_string(score.kills)}) +
                    TrCount("{0} death", "{0} deaths", score.deaths, {std::to_string(score.deaths)});
            lines.push_back(line);
        }
        return lines;
    }

    int GalaxyEggbertGame::NetPlaceOf(int slot) const
    {
        // The winner 1; who went out last 2, ...; 0 still playing (or no match).
        if (!net_ || slot < 0 || slot >= Net::kMaxPlayers || !net_->scores[static_cast<std::size_t>(slot)].played) return 0;
        const auto& n = *net_;
        if (n.timeUp) return n.finalPlaces[static_cast<std::size_t>(slot)]; // the host's ranks
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

    int GalaxyEggbertGame::NetOthersPresent() const noexcept
    {
        if (!NetMatchActive()) return 0;
        int count = 0;
        for (const auto& peer : net_->peers) count += peer.present ? 1 : 0;
        return count;
    }

    void GalaxyEggbertGame::NetLogWorldHash() const
    {
        // What every machine of a match must agree on once play has stopped (the long match's test,
        // Q-022): every block, and the objects that stand still -- where, of what type -- as a set
        // (a bomb laid takes the first free place, which may differ from machine to machine). The
        // lifts, enemies and shots move on each machine's own clock and are left out. One knocked
        // out no longer follows the world (his step stopped): no hash.
        if (!NetMatchActive() || net_->lost || phase_ != GalaxyEggbert::Def::GamePhase::Play)
        {
            std::cout << "GalaxyEggbertCNA: multiplayer: no world hash (not playing)." << std::endl;
            return;
        }
        std::uint64_t hash = 1469598103934665603ULL;
        const auto mix = [&hash](std::uint64_t v) {
            for (int i = 0; i < 8; ++i)
            {
                hash ^= (v >> (8 * i)) & 0xff;
                hash *= 1099511628211ULL;
            }
        };
        const auto& world = worldRuntime_.GetWorld();
        const int size = world.blocksPerAxis();
        int solid = 0;
        for (int z = 0; z < size; ++z)
            for (int y = 0; y < size; ++y)
                for (int x = 0; x < size; ++x)
                {
                    const auto raw = world.getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                                    static_cast<std::uint16_t>(z)).rawValue();
                    solid += raw != 0;
                    mix(raw);
                }
        std::vector<std::array<int, 4>> standing;
        for (const auto& o : worldRuntime_.GetMobileObjects())
        {
            if (!o.active) continue;
            const auto kind = GetObjectDefinition(o.type).semanticKind;
            if (kind == ObjectSemanticKind::Lift || kind == ObjectSemanticKind::Enemy || kind == ObjectSemanticKind::Projectile)
                continue;
            standing.push_back({static_cast<int>(o.type), static_cast<int>(std::lround(o.currentX * 4.0f)),
                                static_cast<int>(std::lround(o.currentY * 4.0f)), static_cast<int>(std::lround(o.currentZ * 4.0f))});
        }
        std::sort(standing.begin(), standing.end());
        for (const auto& o : standing)
            for (const int v : o) mix(static_cast<std::uint64_t>(static_cast<std::int64_t>(v)));
        char text[64];
        std::snprintf(text, sizeof(text), "%016llx", static_cast<unsigned long long>(hash));
        std::cout << "GalaxyEggbertCNA: multiplayer: world hash " << text << " (" << solid << " blocks, " << standing.size()
                  << " objects standing)." << std::endl;
    }

    bool GalaxyEggbertGame::NetHostHoldsClock() const noexcept
    {
        // The host knocked out keeps the time limit's clock while another still plays: he goes back to
        // the lobby once the time is up (the ranks are his to send, Q-022).
        if (!NetMatchActive() || !net_->session.IsHost() || net_->limitSeconds <= 0.0f || net_->timeUp) return false;
        return NetOthersPresent() > 0;
    }

    void GalaxyEggbertGame::NetSay(const std::string& text)
    {
        if (!net_ || !net_->typing) return;
        net_->draft = text;
        NetFinishTyping(true);
    }

    void GalaxyEggbertGame::NetSendLiftSyncs()
    {
        auto& n = *net_;
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
    }

    bool GalaxyEggbertGame::NetSpectating() const noexcept
    {
        return NetMatchActive() && net_->spectating && phase_ == GalaxyEggbert::Def::GamePhase::Lost;
    }

    void GalaxyEggbertGame::NetWatch(int step)
    {
        // The players still in the match whose Blupi has been seen, in their lobby order.
        auto& n = *net_;
        if (!NetMatchActive() || n.timeUp || NetOthersPresent() <= 1) return; // decided: nothing to watch
        std::vector<int> playing;
        for (std::size_t i = 0; i < n.peers.size(); ++i)
            if (n.peers[i].present && n.peers[i].hasPose) playing.push_back(static_cast<int>(i));
        if (playing.empty()) return;
        auto at = std::find(playing.begin(), playing.end(), n.watched);
        if (!n.spectating || at == playing.end())
        {
            // From the results: the world's backlog applied, its sounds not (they were long ago).
            n.spectating = true;
            n.soundsIn.clear();
            n.watched = at == playing.end() ? playing.front() : *at;
            cameraSmoothedInitialized_ = false; simulationCameraInitialized_ = false;
        }
        else
        {
            const int count = static_cast<int>(playing.size());
            const int index = static_cast<int>(at - playing.begin());
            n.watched = playing[static_cast<std::size_t>(((index + step) % count + count) % count)];
        }
        std::cout << "GalaxyEggbertCNA: multiplayer: watching " << n.peers[static_cast<std::size_t>(n.watched)].name << "."
                  << std::endl;
    }

    void GalaxyEggbertGame::NetStopWatching()
    {
        if (!net_ || !net_->spectating) return;
        net_->spectating = false;
        std::cout << "GalaxyEggbertCNA: multiplayer: back to the results." << std::endl;
    }

    void GalaxyEggbertGame::NetSpectatorStep(float dt)
    {
        auto& n = *net_;
        // The match decided (one left, the winner, or the time up), or the one watched gone: the next,
        // or the results.
        if (n.timeUp || NetOthersPresent() <= 1)
        {
            NetStopWatching();
            return;
        }
        if (n.watched < 0 || static_cast<std::size_t>(n.watched) >= n.peers.size() ||
            !n.peers[static_cast<std::size_t>(n.watched)].present)
        {
            n.spectating = false;
            NetWatch(+1);
            if (!n.spectating) return;
        }
        NetAgePeers(dt);
        if (!NetRemotePaused())
        {
            // The world goes on (the others' changes, the lifts, the shots); nothing of his is sent.
            NetBeforeStep();
            worldRuntime_.Update(dt * GameSpeedFactor(gameSpeed_));
            sound_.SetOutbox(nullptr);
            n.soundsOut.clear();
            if (n.session.IsHost()) NetSendLiftSyncs();
        }
        // The camera behind the one watched, as the chase camera is behind Blupi.
        const auto& peer = n.peers[static_cast<std::size_t>(n.watched)];
        const float t = NetPoseBlend(n, static_cast<std::size_t>(n.watched));
        const auto lerp = [t](float a, float b) { return a + (b - a) * t; };
        const Microsoft::Xna::Framework::Vector3 target(lerp(peer.previous.x, peer.pose.x), lerp(peer.previous.y, peer.pose.y) + 1.0f,
                                                        lerp(peer.previous.z, peer.pose.z));
        // Behind him: his model faces its +Z (the matrix's third row), so behind is the opposite way.
        float bx = -peer.pose.world[6], bz = -peer.pose.world[8];
        const float length = std::sqrt(bx * bx + bz * bz);
        if (length > 1e-4f)
        {
            bx /= length;
            bz /= length;
        }
        else
        {
            bx = 0.0f;
            bz = 1.0f;
        }
        constexpr float kDistance = 4.0f, kElevation = 0.35f;
        const Microsoft::Xna::Framework::Vector3 direction(bx * std::cos(kElevation), std::sin(kElevation), bz * std::cos(kElevation));
        const float room = std::max(0.8f, CameraWallDistance(worldRuntime_.GetWorld(), target, direction, kDistance) - 0.2f);
        const Microsoft::Xna::Framework::Vector3 eye = target + direction * std::min(kDistance, room);
        if (!cameraSmoothedInitialized_)
        {
            cameraEyeSmoothed_ = eye;
            cameraTargetSmoothed_ = target;
            cameraSmoothedInitialized_ = true;
        }
        else
        {
            const float a = 1.0f - std::exp(-6.0f * dt);
            cameraEyeSmoothed_ = cameraEyeSmoothed_ + (eye - cameraEyeSmoothed_) * a;
            cameraTargetSmoothed_ = cameraTargetSmoothed_ + (target - cameraTargetSmoothed_) * a;
        }
        if (cameraEyeSmoothed_ != cameraTargetSmoothed_)
        {
            camera_.SetLookAt(cameraEyeSmoothed_, cameraTargetSmoothed_);
            cameraTarget_ = cameraTargetSmoothed_;
        }
    }

    bool GalaxyEggbertGame::NetHasLobbyWorld() const noexcept
    {
        if (!net_) return false;
        return net_->lobby.state.customHash == 0 || net_->customHave == net_->lobby.state.customHash;
    }

    void GalaxyEggbertGame::NetChooseWorld(int index)
    {
        // The host's world for the match: a multiplayer world (SB2's numbers), or past them one of his
        // editor (Q-022: its bytes sent to the others, who are ready once they have it all).
        auto& n = *net_;
        if (!n.session.IsHost() || index < 0) return;
        if (static_cast<std::size_t>(index) < n.worlds.size())
        {
            n.lobby.state.world = static_cast<std::uint16_t>(n.worlds[static_cast<std::size_t>(index)]);
            n.lobby.state.customHash = 0;
            n.lobby.state.customName.clear();
            NetBroadcastLobby();
            return;
        }
        const auto k = static_cast<std::size_t>(index) - n.worlds.size();
        if (k >= n.customWorlds.size()) return;
        const auto bytes = ReadWorldBytes(n.customWorlds[k].path.string());
        if (!bytes || bytes->empty() || bytes->size() > Net::kMaxWorldBytes)
        {
            n.message = TrFormat("{0} could not be read.", {n.customWorlds[k].name});
            return;
        }
        n.customBytes.assign(bytes->begin(), bytes->end());
        n.customHave = Net::HashBytes(*bytes);
        if (n.customHave == 0) n.customHave = 1;
        n.lobby.state.customHash = n.customHave;
        n.lobby.state.customName = Net::CleanText(n.customWorlds[k].name, Net::kMaxWorldName);
        // A changed world: who was ready says so again with it.
        for (auto& slot : n.lobby.state.slots)
            if (slot.present && slot.gamerId != n.session.LocalId()) slot.ready = false;
        NetBroadcastLobby();
        NetSendCustomWorld();
        std::cout << "GalaxyEggbertCNA: multiplayer: the host's world " << n.lobby.state.customName << " ("
                  << n.customBytes.size() << " bytes) chosen and sent." << std::endl;
    }

    void GalaxyEggbertGame::NetSendCustomWorld()
    {
        auto& n = *net_;
        if (!n.session.IsHost() || n.lobby.state.customHash == 0 || n.customHave != n.lobby.state.customHash) return;
        for (std::size_t offset = 0; offset < n.customBytes.size(); offset += Net::kWorldChunk)
        {
            Net::Message m;
            m.kind = Net::MessageKind::WorldData;
            m.customHash = n.customHave;
            m.worldSize = static_cast<std::uint32_t>(n.customBytes.size());
            m.worldOffset = static_cast<std::uint32_t>(offset);
            const std::size_t end = std::min(n.customBytes.size(), offset + Net::kWorldChunk);
            m.chunk.assign(n.customBytes.begin() + static_cast<long>(offset), n.customBytes.begin() + static_cast<long>(end));
            n.session.Send(m);
        }
    }

    void GalaxyEggbertGame::NetReceiveWorldData(const Net::Message& m)
    {
        auto& n = *net_;
        if (m.customHash == n.customHave) return; // already here whole
        const std::size_t pieces = (m.worldSize + Net::kWorldChunk - 1) / Net::kWorldChunk;
        if (m.customHash != n.customAssembling || n.customBytes.size() != m.worldSize)
        {
            n.customAssembling = m.customHash;
            n.customBytes.assign(m.worldSize, 0);
            n.customGot.assign(pieces, false);
        }
        if (m.worldOffset % Net::kWorldChunk != 0) return;
        const std::size_t piece = m.worldOffset / Net::kWorldChunk;
        const std::size_t expected = std::min<std::size_t>(Net::kWorldChunk, m.worldSize - m.worldOffset);
        if (piece >= pieces || m.chunk.size() != expected) return;
        std::copy(m.chunk.begin(), m.chunk.end(), n.customBytes.begin() + static_cast<long>(m.worldOffset));
        n.customGot[piece] = true;
        if (std::find(n.customGot.begin(), n.customGot.end(), false) != n.customGot.end()) return;
        // Whole: its hash is the one announced, else it is asked for again by the next Hello's resend.
        std::uint64_t hash = Net::HashBytes(std::string_view(reinterpret_cast<const char*>(n.customBytes.data()), n.customBytes.size()));
        if (hash == 0) hash = 1;
        if (hash != m.customHash)
        {
            std::cout << "GalaxyEggbertCNA: multiplayer: the host's world arrived damaged; dropped." << std::endl;
            n.customAssembling = 0;
            n.customGot.assign(pieces, false);
            return;
        }
        n.customHave = hash;
        std::cout << "GalaxyEggbertCNA: multiplayer: the host's world received (" << n.customBytes.size() << " bytes)."
                  << std::endl;
    }

    void GalaxyEggbertGame::NetMarkLeft(int slot)
    {
        if (net_ && slot >= 0 && slot < Net::kMaxPlayers && net_->scores[static_cast<std::size_t>(slot)].played)
            net_->scores[static_cast<std::size_t>(slot)].left = true;
    }

    void GalaxyEggbertGame::NetPeerBack(int slot, std::uint8_t gamerId)
    {
        // His place's peer again present under his new gamer; his going out undone (the later ones
        // moved up a place).
        auto& n = *net_;
        if (slot < 0 || slot >= Net::kMaxPlayers) return;
        for (auto& peer : n.peers)
        {
            if (peer.slot != slot) continue;
            peer.id = gamerId;
            peer.present = true;
            peer.hasPose = false;
            peer.silence = 0.0f;
            peer.paused = false;
            peer.age = 0.0f;
            NetNotice(TrFormat("{0} is back !", {peer.name}));
        }
        auto& score = n.scores[static_cast<std::size_t>(slot)];
        if (score.out > 0)
        {
            for (auto& other : n.scores)
                if (other.out > score.out) --other.out;
            score.out = 0;
            n.outCount = std::max(0, n.outCount - 1);
        }
        score.left = false;
    }

    void GalaxyEggbertGame::NetTryRejoin(std::uint8_t gamerId, const Net::Message& hello)
    {
        auto& n = *net_;
        if (hello.contentHash != n.contentHash || hello.simulationRevision != Net::kProtocolVersion) return;
        // One who dropped from this match, by his name (another is not taken while it is played).
        int slot = -1;
        for (int i = 0; i < Net::kMaxPlayers; ++i)
        {
            const auto& score = n.scores[static_cast<std::size_t>(i)];
            if (score.played && score.left && score.name == Net::CleanText(hello.name, Net::kMaxName)) slot = i;
        }
        if (slot < 0)
        {
            std::cout << "GalaxyEggbertCNA: multiplayer: " << hello.name << " asks to join the match in progress; not taken."
                      << std::endl;
            return;
        }
        auto& place = n.lobby.state.slots[static_cast<std::size_t>(slot)];
        if (place.present && place.gamerId != gamerId) n.lobby.Remove(place.gamerId);
        place = Net::LobbySlot{};
        place.present = true;
        place.playing = true;
        place.gamerId = gamerId;
        place.team = static_cast<std::uint8_t>(n.scores[static_cast<std::size_t>(slot)].team);
        place.name = n.scores[static_cast<std::size_t>(slot)].name;
        NetBroadcastLobby();
        if (n.lobby.state.customHash != 0) NetSendCustomWorld();
        int lives = n.lobby.state.lives;
        for (const auto& peer : n.peers)
            if (peer.slot == slot) lives = peer.lives;
        NetPeerBack(slot, gamerId);

        // The world as the match began, to send only what changed since (the host's own world is
        // brought up to date with what came in first).
        auto& world = worldRuntime_.GetWorldMutable();
        auto& objects = worldRuntime_.GetMobileObjectsMutable();
        for (const auto& event : n.incoming) n.replicator.Apply(event, world, objects);
        n.incoming.clear();
        n.replicator.Snapshot(world, objects);
        const bool custom = n.lobby.state.customHash != 0;
        const auto bytes = custom ? std::optional<std::string>(std::string(n.customBytes.begin(), n.customBytes.end()))
                                  : ReadWorldBytes(MissionWorldPath(n.world));
        GalaxyEggbert::Game::WorldRuntime start;
        std::istringstream in(bytes ? *bytes : std::string{}, std::ios::in | std::ios::binary);
        if (!bytes || !start.LoadFromVwrStream(in))
        {
            std::cout << "GalaxyEggbertCNA: multiplayer: the world could not be read again for " << hello.name << "." << std::endl;
            return;
        }

        Net::Message m;
        m.kind = Net::MessageKind::Rejoin;
        m.target = gamerId;
        m.rejoinSlot = static_cast<std::uint8_t>(slot);
        m.world = static_cast<std::uint16_t>(std::clamp(n.world, 1, 999));
        m.customHash = n.lobby.state.customHash;
        m.lives = static_cast<std::uint8_t>(std::clamp(lives, 0, Net::kMultiplayerLives));
        m.minutes = n.lobby.state.minutes;
        m.seconds = std::max(0.0f, n.matchSeconds);
        m.firstSpawn = n.replicator.NextSpawnFor(slot);
        for (int i = 0; i < Net::kMaxPlayers; ++i)
        {
            const auto& score = n.scores[static_cast<std::size_t>(i)];
            auto& wire = m.scores[static_cast<std::size_t>(i)];
            wire.played = score.played;
            wire.left = score.left;
            wire.deaths = static_cast<std::uint8_t>(std::clamp(score.deaths, 0, 255));
            wire.kills = static_cast<std::uint8_t>(std::clamp(score.kills, 0, 255));
            wire.out = static_cast<std::uint8_t>(std::clamp(score.out, 0, Net::kMaxPlayers));
            wire.team = static_cast<std::uint8_t>(score.team % Net::kTeams);
            wire.name = score.name;
        }
        n.session.Send(m);
        const auto events = n.replicator.Rejoining(start.GetWorld(), world, objects);
        for (std::size_t first = 0; first < events.size(); first += Net::kMaxEvents)
        {
            Net::Message snapshot;
            snapshot.kind = Net::MessageKind::Snapshot;
            snapshot.target = gamerId;
            snapshot.events.assign(events.begin() + static_cast<long>(first),
                                   events.begin() + static_cast<long>(std::min(events.size(), first + Net::kMaxEvents)));
            n.session.Send(snapshot);
        }
        std::cout << "GalaxyEggbertCNA: multiplayer: " << hello.name << " is back at place " << slot << " (" << lives
                  << " lives, " << events.size() << " changes of the world sent)." << std::endl;
    }

    void GalaxyEggbertGame::NetSaveAddresses() const
    {
        std::ofstream out(GalaxyEggbert::Game::UserPath(kAddressFile), std::ios::trunc);
        out << net_->address << '\n' << net_->server << '\n';
    }

    void GalaxyEggbertGame::NetSetServer(const std::string& typed)
    {
        auto& n = *net_;
        std::string server;
        for (const char c : typed)
            if (c != ' ' && c >= 0x21 && c < 0x7f && server.size() < kAddressInput) server += c;
        n.server = server;
        NetSaveAddresses();
        n.message = server.empty() ? std::string(Tr("No server: the local network only (from the next start)."))
                                   : TrFormat("The server {0} is used from the game's next start.", {server});
    }

    void GalaxyEggbertGame::NetStartPauseTyping(int w, int h)
    {
        auto& n = *net_;
        NetStartMatchTyping();
        n.keyboard.Layout(InputPad::PanelRect(20.0f, 64.0f, 620.0f, 254.0f, w, h), false);
        n.keyboard.Begin(n.draft, true);
    }

    void GalaxyEggbertGame::NetUpdateMatchKeyboard(const Microsoft::Xna::Framework::Input::MouseState& mouse, int, int)
    {
        auto& n = *net_;
        const auto input = inputPad_.UpdatePanel(mouse, {}, &n.keyboard);
        if (input.keyboardKey >= 0)
        {
            const auto result = n.keyboard.Press(input.keyboardKey, n.draft, kChatInput);
            if (result == ScreenKeyboard::Result::Done) NetFinishTyping(true);
            else if (result == ScreenKeyboard::Result::Cancel) NetFinishTyping(false);
        }
        else if (input.outsideClick)
        {
            NetFinishTyping(false);
        }
    }

    void GalaxyEggbertGame::DrawNetMatchKeyboard(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int w, int h)
    {
        // As the lobby's Send message: the line, the last chat lines, the screen's keyboard.
        const auto& n = *net_;
        const float s = static_cast<float>(h) / 480.0f;
        const float left = static_cast<float>(w) * 0.5f - 290.0f * s;
        std::vector<PanelLine> lines;
        lines.push_back(PanelLine{std::string(Tr(Net::kTextSendMessage)) + ":  " + n.draft + "_", left, 268.0f * s, 0.5f, false, false});
        const int shown = static_cast<int>(std::min<std::size_t>(n.chat.size(), 5));
        for (int j = 0; j < shown; ++j)
            lines.push_back(PanelLine{n.chat[n.chat.size() - static_cast<std::size_t>(shown - j)], left,
                                      (296.0f + 20.0f * static_cast<float>(j)) * s, 0.42f, false, false});
        inputPad_.DrawPanel(device, w, h, std::string(Tr("Message")), {}, lines, &net_->keyboard, focusHighlight_);
    }
}
