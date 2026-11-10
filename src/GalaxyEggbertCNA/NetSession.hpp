#pragma once

#include <GalaxyEggbert/Net/NetProtocol.hpp>

#include <Microsoft/Xna/Framework/Game.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Microsoft::Xna::Framework::Net
{
    class NetworkSession;
}
namespace System
{
    class IAsyncResult;
}

namespace GalaxyEggbert::CNA
{
    // The network game's link to CNA (plans/plan_gemultiplayer.md §7.1): a SystemLink session (ENet
    // over UDP, found by a broadcast on the local network) of up to four players, each signed in
    // automatically as the selected player ("Player A", owner Q-019 Q6) -- the gamer services start
    // only when the player opens Multiplayer, so a game played alone never starts them. Sessions
    // advertise the protocol version and the multiplayer worlds' content hash, so only compatible
    // ones are listed. Messages are Net::Message, sent to everyone (CNA's host relays).
    class NetSession
    {
    public:
        struct FoundSession
        {
            std::string host;
            int players = 0;
            int world = 1;
        };
        struct Received
        {
            std::uint8_t sender = 0;
            Net::Message message;
        };

        explicit NetSession(Microsoft::Xna::Framework::Game& game);
        ~NetSession();
        NetSession(const NetSession&) = delete;
        NetSession& operator=(const NetSession&) = delete;

        // Starts the gamer services once, signed in as @p name (letters, digits, single spaces);
        // true once a gamer is signed in (a frame or two after the first call).
        bool SignIn(const std::string& name);
        [[nodiscard]] std::string LocalName() const;

        // Looking for sessions: a search in the background, its results when it is done.
        void BeginFind(std::uint64_t contentHash);
        [[nodiscard]] bool Finding() const noexcept { return findResult_ != nullptr; }
        [[nodiscard]] const std::vector<FoundSession>& Found() const noexcept { return found_; }

        bool Host(std::uint64_t contentHash, int world);
        bool Join(int foundIndex);
        void Leave();
        // The host's match begins: the game is no longer listed or joined (XNA's StartGame).
        void StartGame();
        [[nodiscard]] bool Active() const noexcept { return session_ != nullptr; }
        [[nodiscard]] bool IsHost() const;
        [[nodiscard]] std::uint8_t LocalId() const;
        // Whether our gamer is in the session yet (it joins at the Update after Create or Join).
        [[nodiscard]] bool HasLocalGamer() const;
        // Whether CNA's networking is in this build (not on the web).
        [[nodiscard]] static bool Available() noexcept;

        // Once a frame: services the connection and collects what arrived and who came or went.
        void Update();
        // False when it could not go (no session, our gamer not in it yet, a send error).
        bool Send(const Net::Message& message);
        [[nodiscard]] std::vector<Received> TakeReceived();
        // Gamers who joined (id, name) and left since the last call; whether the session ended.
        [[nodiscard]] std::vector<std::pair<std::uint8_t, std::string>> TakeJoined();
        [[nodiscard]] std::vector<std::uint8_t> TakeLeft();
        [[nodiscard]] bool TakeEnded(std::string& reason);
        [[nodiscard]] std::string NameOf(std::uint8_t id) const;

        // Rejected packets (another game's, damaged): counted, never applied.
        [[nodiscard]] int Rejected() const noexcept { return rejected_; }
        [[nodiscard]] const std::string& LastError() const noexcept { return error_; }

    private:
        void Subscribe();
        void FinishFind();
        void AbandonFind();

        Microsoft::Xna::Framework::Game& game_;
        bool servicesStarted_ = false;
        Microsoft::Xna::Framework::Net::NetworkSession* session_ = nullptr;
        System::IAsyncResult* findResult_ = nullptr;
        std::vector<FoundSession> found_;
        std::size_t lastFoundCount_ = 0;
        std::vector<Received> received_;
        std::vector<std::pair<std::uint8_t, std::string>> joined_;
        std::vector<std::uint8_t> left_;
        bool ended_ = false;
        std::string endReason_;
        std::string error_;
        int rejected_ = 0;
        std::shared_ptr<void> available_; // the last search's AvailableNetworkSessionCollection
    };
}
