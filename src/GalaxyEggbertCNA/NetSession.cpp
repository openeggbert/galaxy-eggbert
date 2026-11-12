#include "NetSession.hpp"

#include <GalaxyEggbert/Game/Localization.hpp>

#if defined(GALAXY_EGGBERT_NET)
#include <Microsoft/Xna/Framework/GamerServices/Gamer.hpp>
#include <Microsoft/Xna/Framework/GamerServices/GamerServicesDispatcher.hpp>
#include <Microsoft/Xna/Framework/GamerServices/Guide.hpp>
#include <Microsoft/Xna/Framework/GamerServices/SignedInGamer.hpp>
#include <Microsoft/Xna/Framework/GamerServices/SignedInGamerCollection.hpp>
#include <Microsoft/Xna/Framework/Net/AvailableNetworkSession.hpp>
#include <Microsoft/Xna/Framework/Net/AvailableNetworkSessionCollection.hpp>
#include <Microsoft/Xna/Framework/Net/GamerJoinedEventArgs.hpp>
#include <Microsoft/Xna/Framework/Net/GamerLeftEventArgs.hpp>
#include <Microsoft/Xna/Framework/Net/LocalNetworkGamer.hpp>
#include <Microsoft/Xna/Framework/Net/NetworkGamer.hpp>
#include <Microsoft/Xna/Framework/Net/NetworkSession.hpp>
#include <Microsoft/Xna/Framework/Net/NetworkSessionEndedEventArgs.hpp>
#include <Microsoft/Xna/Framework/Net/NetworkSessionProperties.hpp>
#include <Microsoft/Xna/Framework/Net/NetworkSessionState.hpp>
#include <Microsoft/Xna/Framework/Net/NetworkSessionType.hpp>
#include <Microsoft/Xna/Framework/Net/SendDataOptions.hpp>
#include <System/IAsyncResult.hpp>
#if __has_include(<CNA/GamerServices/Configuration.hpp>)
#include <CNA/GamerServices/Configuration.hpp>
#include <Microsoft/Xna/Framework/Input/TextInputEXT.hpp>
#define GALAXY_EGGBERT_NET_SERVER 1
#endif
#if __has_include(<CNA/Net/SystemLinkDiscovery.hpp>)
#include <CNA/Net/SystemLinkDiscovery.hpp>
#define GALAXY_EGGBERT_NET_ADDRESS 1
#endif

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <utility>
#include <chrono>
#include <thread>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        using namespace Microsoft::Xna::Framework::Net;
        namespace GS = Microsoft::Xna::Framework::GamerServices;

        // Version and content in the session's advertised properties: only sessions of this
        // protocol and these multiplayer worlds are found.
        NetworkSessionProperties Advertised(std::uint64_t contentHash, int world)
        {
            NetworkSessionProperties properties;
            properties[0] = static_cast<int>(GalaxyEggbert::Net::kProtocolVersion);
            properties[1] = static_cast<int>(contentHash & 0x7fffffffULL);
            if (world > 0) properties[2] = world;
            return properties;
        }

        const char* EndReasonText(NetworkSessionEndReason reason)
        {
            switch (reason)
            {
            case NetworkSessionEndReason::HostEndedSession: return GalaxyEggbert::Game::TrMark("the host left");
            case NetworkSessionEndReason::RemovedByHost: return GalaxyEggbert::Game::TrMark("removed by the host");
            case NetworkSessionEndReason::ClientSignedOut: return GalaxyEggbert::Game::TrMark("signed out");
            default: return GalaxyEggbert::Game::TrMark("the connection was lost");
            }
        }
    }

    NetSession::NetSession(Microsoft::Xna::Framework::Game& game) : game_(game) {}

    NetSession::~NetSession()
    {
        Leave();
        if (findResult_ != nullptr)
        {
            try
            {
                (void)NetworkSession::EndFind(findResult_);
            }
            catch (...)
            {
            }
            delete findResult_;
            findResult_ = nullptr;
        }
    }

    bool NetSession::ServerSupport() noexcept
    {
#if defined(GALAXY_EGGBERT_NET_SERVER)
        return true;
#else
        return false;
#endif
    }

    bool NetSession::SetServer(const std::string& endpoint)
    {
        if (servicesStarted_) return endpoint == server_; // the services read it once, at their start
#if defined(GALAXY_EGGBERT_NET_SERVER)
        if (!endpoint.empty())
        {
            ::CNA::GamerServices::Configuration configuration;
            configuration.endpoint = endpoint;
            configuration.gameId = "galaxy-eggbert";
            // Plain HTTP only on this machine's numeric loopback (a test's server, CNA's own rule).
            configuration.insecureLoopback = endpoint.rfind("http://127.0.0.1", 0) == 0 || endpoint.rfind("http://[::1]", 0) == 0;
            try
            {
                ::CNA::GamerServices::validateConfiguration(configuration);
            }
            catch (const std::exception& failure)
            {
                error_ = failure.what();
                return false;
            }
        }
        server_ = endpoint;
        return true;
#else
        return endpoint.empty();
#endif
    }

    bool NetSession::SignIn(const std::string& name)
    {
        if (!servicesStarted_)
        {
            // CNA's local offline profile, signed in when the services start (LocalProfiles.hpp);
            // with a server (SetServer) an account, through CNA's Guide.
            // ASCII: the profile name goes through the environment (and a Windows one's code page).
            const std::string profile = Net::AsciiText(name, Net::kMaxName);
#if defined(_WIN32)
            _putenv_s("CNA_GAMER_SERVICES_AUTO_SIGN_IN", server_.empty() ? profile.c_str() : "");
#else
            if (server_.empty()) setenv("CNA_GAMER_SERVICES_AUTO_SIGN_IN", profile.c_str(), 1);
            else unsetenv("CNA_GAMER_SERVICES_AUTO_SIGN_IN");
#endif
#if defined(GALAXY_EGGBERT_NET_SERVER)
            if (!server_.empty())
            {
                ::CNA::GamerServices::Configuration configuration;
                configuration.endpoint = server_;
                configuration.gameId = "galaxy-eggbert";
                configuration.insecureLoopback =
                    server_.rfind("http://127.0.0.1", 0) == 0 || server_.rfind("http://[::1]", 0) == 0;
                ::CNA::GamerServices::setConfigurationOverride(configuration);
                online_ = true;
            }
#endif
            try
            {
                if (!GS::GamerServicesDispatcher::getIsInitializedProperty())
                    GS::GamerServicesDispatcher::Initialize(game_.getServicesProperty());
                // The services' own notices (signed in ...) in a corner, off the screens' buttons. After
                // Initialize: the Guide refuses every call before it, as XNA's does (CNA 643b97d5f).
                GS::Guide::setNotificationPositionProperty(GS::NotificationPosition::TopRight);
            }
            catch (const std::exception& failure)
            {
                error_ = GalaxyEggbert::Game::TrFormat("the network could not start: {0}", {failure.what()});
                return false;
            }
            servicesStarted_ = true;
        }
        GS::GamerServicesDispatcher::Update();
        const bool signedIn = GS::Gamer::getSignedInGamersProperty()->getCountProperty() > 0;
#if defined(GALAXY_EGGBERT_NET_SERVER)
        if (online_ && !signedIn && !signInShown_ && !GS::Guide::getIsVisibleProperty())
        {
            // The account's name and password in CNA's Guide (once; Escape there gives it up).
            signInShown_ = true;
            try
            {
                GS::Guide::ShowSignIn(1, true);
            }
            catch (const std::exception& failure)
            {
                error_ = GalaxyEggbert::Game::TrFormat("no sign-in: {0}", {failure.what()});
            }
            // GALAXY_NET_ACCOUNT=name:password: a test's account typed into the Guide (a test server's
            // disposable accounts only).
            if (const char* account = std::getenv("GALAXY_NET_ACCOUNT"); account != nullptr && std::strchr(account, ':'))
            {
                const std::string text(account);
                const auto colon = text.find(':');
                const auto type = [](const std::string& line) {
                    for (const unsigned char c : line) Microsoft::Xna::Framework::Input::TextInputEXT::INTERNAL_OnTextInput(c);
                    Microsoft::Xna::Framework::Input::TextInputEXT::INTERNAL_OnTextInput(u'\r');
                };
                type(text.substr(0, colon));
                type(text.substr(colon + 1));
            }
        }
#endif
        return signedIn;
    }

    std::string NetSession::LocalName() const
    {
        auto* gamers = GS::Gamer::getSignedInGamersProperty();
        return gamers->getCountProperty() > 0 ? (*gamers)[0]->getGamertagProperty() : std::string();
    }

    void NetSession::BeginFind(std::uint64_t contentHash)
    {
        if (findResult_ != nullptr) return;
        try
        {
            findResult_ = NetworkSession::BeginFind(internet_ ? NetworkSessionType::PlayerMatch : NetworkSessionType::SystemLink, 1,
                                                    Advertised(contentHash, 0), nullptr, {});
        }
        catch (const std::exception& failure)
        {
            error_ = GalaxyEggbert::Game::TrFormat("no search: {0}", {failure.what()});
            std::cout << "GalaxyEggbertCNA: multiplayer: " << error_ << std::endl;
            findResult_ = nullptr;
        }
    }

    void NetSession::FinishFind()
    {
        if (findResult_ == nullptr || !findResult_->getIsCompletedProperty()) return;
        found_.clear();
        try
        {
            auto collection = std::make_shared<AvailableNetworkSessionCollection>(NetworkSession::EndFind(findResult_));
            const auto& sessions = *collection;
            for (int i = 0; i < sessions.getCountProperty(); ++i)
            {
                const auto& session = sessions[i];
                FoundSession found;
                found.host = session.getHostGamertagProperty();
                found.players = session.getCurrentGamerCountProperty();
                const auto& world = session.getSessionPropertiesProperty()[2];
                found.world = world ? *world : 1;
                found_.push_back(found);
            }
            available_ = collection;
        }
        catch (const std::exception& failure)
        {
            error_ = GalaxyEggbert::Game::TrFormat("search failed: {0}", {failure.what()});
            std::cout << "GalaxyEggbertCNA: multiplayer: " << error_ << std::endl;
        }
        if (found_.size() != lastFoundCount_)
        {
            lastFoundCount_ = found_.size();
            std::cout << "GalaxyEggbertCNA: multiplayer: " << found_.size() << " games on the network." << std::endl;
        }
        delete findResult_;
        findResult_ = nullptr;
    }

    void NetSession::AbandonFind()
    {
        // CNA refuses a session while a search is outstanding: it is ended first (a search lasts
        // a fraction of a second).
        if (findResult_ == nullptr) return;
        try
        {
            (void)NetworkSession::EndFind(findResult_);
        }
        catch (...)
        {
        }
        delete findResult_;
        findResult_ = nullptr;
    }

    bool NetSession::Host(std::uint64_t contentHash, int world)
    {
        Leave();
        AbandonFind();
        const auto create = [&] {
            session_ = NetworkSession::Create(internet_ ? NetworkSessionType::PlayerMatch : NetworkSessionType::SystemLink, 1,
                                              Net::kMaxPlayers, 0,
                                              Advertised(contentHash, world));
        };
        try
        {
#if defined(GALAXY_EGGBERT_NET_ADDRESS)
            // The game on a fixed port beside the search's, so that a router can forward both to the
            // host (docs/handbook/09-multiplayer.md); another program on it: a port of the system's.
            try
            {
                // Internet games go through the server's relay: no port of ours to forward.
                ::CNA::Net::SystemLinkDiscovery::setHostPortProperty(internet_ ? 0 : kHostPort);
                create();
            }
            catch (const std::exception& failure)
            {
                std::cout << "GalaxyEggbertCNA: multiplayer: port " << kHostPort << " busy (" << failure.what()
                          << "); another one." << std::endl;
                ::CNA::Net::SystemLinkDiscovery::setHostPortProperty(0);
                create();
            }
#else
            create();
#endif
        }
        catch (const std::exception& failure)
        {
            error_ = GalaxyEggbert::Game::TrFormat("the game could not be created: {0}", {failure.what()});
            std::cout << "GalaxyEggbertCNA: multiplayer: " << error_ << std::endl;
            session_ = nullptr;
            return false;
        }
        Subscribe();
        return true;
    }

    bool NetSession::Join(int foundIndex)
    {
        auto collection = std::static_pointer_cast<AvailableNetworkSessionCollection>(available_);
        if (!collection || foundIndex < 0 || foundIndex >= collection->getCountProperty()) return false;
        Leave();
        AbandonFind();
        try
        {
            const auto& sessions = *collection;
            session_ = NetworkSession::Join(&sessions[foundIndex]);
        }
        catch (const std::exception& failure)
        {
            error_ = GalaxyEggbert::Game::TrFormat("could not join: {0}", {failure.what()});
            std::cout << "GalaxyEggbertCNA: multiplayer: " << error_ << std::endl;
            session_ = nullptr;
            return false;
        }
        Subscribe();
        return true;
    }

    void NetSession::Subscribe()
    {
        if (session_ == nullptr) return;
        ended_ = false;
        // A bad network for the tests (tools/net_two_players.sh NET_LOSS, NET_LATENCY): XNA's
        // simulated loss (of the unreliable packets, the poses) and latency, on what this machine sends.
        try
        {
            if (const char* loss = std::getenv("GALAXY_NET_LOSS"); loss != nullptr && *loss != 0)
                session_->setSimulatedPacketLossProperty(static_cast<float>(std::clamp(std::atof(loss), 0.0, 1.0)));
            if (const char* latency = std::getenv("GALAXY_NET_LATENCY"); latency != nullptr && *latency != 0)
                session_->setSimulatedLatencyProperty(System::TimeSpan::FromMilliseconds(std::clamp(std::atof(latency), 0.0, 2000.0)));
            if (std::getenv("GALAXY_NET_LOSS") != nullptr || std::getenv("GALAXY_NET_LATENCY") != nullptr)
                std::cout << "GalaxyEggbertCNA: multiplayer: a simulated network: loss "
                          << session_->getSimulatedPacketLossProperty() << ", latency "
                          << session_->getSimulatedLatencyProperty().getTotalSecondsProperty() * 1000.0 << " ms." << std::endl;
        }
        catch (const std::exception& failure)
        {
            error_ = failure.what();
        }
        session_->GamerJoined += [this](System::Object*, const GamerJoinedEventArgs& e) {
            if (auto* gamer = e.getGamerProperty()) joined_.emplace_back(gamer->getIdProperty(), gamer->getGamertagProperty());
        };
        session_->GamerLeft += [this](System::Object*, const GamerLeftEventArgs& e) {
            if (auto* gamer = e.getGamerProperty()) left_.push_back(gamer->getIdProperty());
        };
        session_->SessionEnded += [this](System::Object*, const NetworkSessionEndedEventArgs& e) {
            ended_ = true;
            endReason_ = EndReasonText(e.getEndReasonProperty());
        };
    }

    void NetSession::Leave()
    {
        if (session_ == nullptr) return;
        try
        {
            // What was sent last (the last death's PK_LOST, a chat line) goes out before the session
            // closes, as XNA wants (SendData only queues; Update transmits), and has a moment to be
            // acknowledged: the teardown drops what is still unacknowledged (UB-012). 150 ms at most.
            for (int i = 0; i < 15; ++i)
            {
                session_->Update();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            session_->Dispose();
        }
        catch (...)
        {
        }
        delete session_;
        session_ = nullptr;
        found_.clear(); // the list was of before; the next search makes it again
        received_.clear();
        joined_.clear();
        left_.clear();
    }

    void NetSession::StartGame()
    {
        if (session_ == nullptr || !session_->getIsHostProperty()) return;
        try
        {
            // A player whose network dropped may come back to the match (Galaxy's, Q-022): the host
            // takes him at his place, any other is not taken.
            if (session_->getSessionStateProperty() == NetworkSessionState::Lobby)
            {
                session_->setAllowJoinInProgressProperty(true);
                session_->StartGame();
            }
        }
        catch (const std::exception& failure)
        {
            error_ = failure.what();
        }
    }

    bool NetSession::SetSearchAddress(const std::string& address)
    {
#if defined(GALAXY_EGGBERT_NET_ADDRESS)
        try
        {
            ::CNA::Net::SystemLinkDiscovery::setExtraSearchAddressesProperty(
                address.empty() ? std::vector<std::string>{} : std::vector<std::string>{address});
            std::cout << "GalaxyEggbertCNA: multiplayer: games searched "
                      << (address.empty() ? std::string("on the local network only") : "at " + address + " too") << "."
                      << std::endl;
            return true;
        }
        catch (const std::exception& failure)
        {
            error_ = failure.what();
            return false;
        }
#else
        return address.empty();
#endif
    }

    bool NetSession::AddressSearch() noexcept
    {
#if defined(GALAXY_EGGBERT_NET_ADDRESS)
        return true;
#else
        return false;
#endif
    }

    void NetSession::EndGame()
    {
        if (session_ == nullptr || !session_->getIsHostProperty()) return;
        try
        {
            if (session_->getSessionStateProperty() == NetworkSessionState::Playing) session_->EndGame();
        }
        catch (const std::exception& failure)
        {
            error_ = failure.what();
        }
    }

    bool NetSession::IsHost() const { return session_ != nullptr && session_->getIsHostProperty(); }

    bool NetSession::Available() noexcept { return true; }

    bool NetSession::HasLocalGamer() const
    {
        return session_ != nullptr && session_->getLocalGamersProperty().getCountProperty() > 0;
    }

    std::uint8_t NetSession::LocalId() const
    {
        if (session_ == nullptr) return 0;
        const auto& locals = session_->getLocalGamersProperty();
        return locals.getCountProperty() > 0 ? locals[0]->getIdProperty() : 0;
    }

    std::uint8_t NetSession::HostId() const
    {
        if (session_ == nullptr) return 0;
        auto* host = session_->getHostProperty();
        return host != nullptr ? host->getIdProperty() : 0;
    }

    std::string NetSession::NameOf(std::uint8_t id) const
    {
        if (session_ == nullptr) return {};
        auto* gamer = session_->FindGamerById(id);
        return gamer != nullptr ? gamer->getGamertagProperty() : std::string();
    }

    void NetSession::Update()
    {
        if (servicesStarted_) GS::GamerServicesDispatcher::Update();
        FinishFind();
        if (session_ == nullptr) return;
        try
        {
            session_->Update();
        }
        catch (const std::exception& failure)
        {
            error_ = failure.what();
            ended_ = true;
            endReason_ = GalaxyEggbert::Game::TrMark("the connection was lost");
            return;
        }
        if (session_ == nullptr) return;
        const auto& locals = session_->getLocalGamersProperty();
        if (locals.getCountProperty() == 0) return;
        auto* local = locals[0];
        // XNA's ReceiveData fills an array as large as the packet at least (it never grows it):
        // a datagram's largest, ours are a few kilobytes.
        std::vector<SharpRuntime::bytecs> data(65536);
        try
        {
            while (local->getIsDataAvailableProperty())
            {
                NetworkGamer* sender = nullptr;
                const int size = local->ReceiveData(data, sender);
                if (size <= 0 || sender == nullptr || sender->getIsLocalProperty()) continue;
                auto message = Net::Decode(data.data(), static_cast<std::size_t>(size));
                if (!message)
                {
                    ++rejected_;
                    continue;
                }
                received_.push_back({sender->getIdProperty(), std::move(*message)});
            }
        }
        catch (const std::exception& failure)
        {
            error_ = failure.what(); // a packet too large for anything of ours: left, never applied
            ++rejected_;
        }
    }

    bool NetSession::Send(const Net::Message& message)
    {
        if (session_ == nullptr) return false;
        const auto& locals = session_->getLocalGamersProperty();
        if (locals.getCountProperty() == 0) return false;
        const auto bytes = Net::Encode(message);
        const std::vector<SharpRuntime::bytecs> data(bytes.begin(), bytes.end());
        try
        {
            locals[0]->SendData(data, Net::IsReliable(message.kind) ? SendDataOptions::ReliableInOrder : SendDataOptions::None);
        }
        catch (const std::exception& failure)
        {
            error_ = failure.what();
            return false;
        }
        return true;
    }

    std::vector<NetSession::Received> NetSession::TakeReceived() { return std::exchange(received_, {}); }
    std::vector<std::pair<std::uint8_t, std::string>> NetSession::TakeJoined() { return std::exchange(joined_, {}); }
    std::vector<std::uint8_t> NetSession::TakeLeft() { return std::exchange(left_, {}); }

    bool NetSession::TakeEnded(std::string& reason)
    {
        if (!ended_) return false;
        ended_ = false;
        reason = endReason_;
        return true;
    }
}

#else

// Without CNA's networking (the web build): no network game, the menu has no Multiplayer.
namespace GalaxyEggbert::CNA
{
    NetSession::NetSession(Microsoft::Xna::Framework::Game& game) : game_(game) {}
    NetSession::~NetSession() = default;
    bool NetSession::Available() noexcept { return false; }
    bool NetSession::SignIn(const std::string&)
    {
        error_ = GalaxyEggbert::Game::Tr("this build has no network game");
        return false;
    }
    std::string NetSession::LocalName() const { return {}; }
    void NetSession::BeginFind(std::uint64_t) {}
    void NetSession::FinishFind() {}
    void NetSession::AbandonFind() {}
    bool NetSession::Host(std::uint64_t, int) { return false; }
    bool NetSession::Join(int) { return false; }
    void NetSession::Subscribe() {}
    void NetSession::Leave() {}
    void NetSession::StartGame() {}
    void NetSession::EndGame() {}
    bool NetSession::SetSearchAddress(const std::string& address) { return address.empty(); }
    bool NetSession::AddressSearch() noexcept { return false; }
    bool NetSession::SetServer(const std::string& endpoint) { return endpoint.empty(); }
    bool NetSession::ServerSupport() noexcept { return false; }
    bool NetSession::IsHost() const { return false; }
    bool NetSession::HasLocalGamer() const { return false; }
    std::uint8_t NetSession::LocalId() const { return 0; }
    std::uint8_t NetSession::HostId() const { return 0; }
    std::string NetSession::NameOf(std::uint8_t) const { return {}; }
    void NetSession::Update() {}
    bool NetSession::Send(const Net::Message&) { return false; }
    std::vector<NetSession::Received> NetSession::TakeReceived() { return {}; }
    std::vector<std::pair<std::uint8_t, std::string>> NetSession::TakeJoined() { return {}; }
    std::vector<std::uint8_t> NetSession::TakeLeft() { return {}; }
    bool NetSession::TakeEnded(std::string&) { return false; }
}

#endif
