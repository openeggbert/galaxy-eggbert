#include "NetSession.hpp"

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

#include <cstdlib>
#include <exception>
#include <iostream>
#include <utility>

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
            case NetworkSessionEndReason::HostEndedSession: return "the host left";
            case NetworkSessionEndReason::RemovedByHost: return "removed by the host";
            case NetworkSessionEndReason::ClientSignedOut: return "signed out";
            default: return "the connection was lost";
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

    bool NetSession::SignIn(const std::string& name)
    {
        if (!servicesStarted_)
        {
            // CNA's local offline profile, signed in when the services start (LocalProfiles.hpp).
            const std::string profile = Net::CleanText(name, Net::kMaxName);
#if defined(_WIN32)
            _putenv_s("CNA_GAMER_SERVICES_AUTO_SIGN_IN", profile.c_str());
#else
            setenv("CNA_GAMER_SERVICES_AUTO_SIGN_IN", profile.c_str(), 1);
#endif
            try
            {
                // The services' own notices (signed in ...) in a corner, off the screens' buttons.
                GS::Guide::setNotificationPositionProperty(GS::NotificationPosition::TopRight);
                if (!GS::GamerServicesDispatcher::getIsInitializedProperty())
                    GS::GamerServicesDispatcher::Initialize(game_.getServicesProperty());
            }
            catch (const std::exception& failure)
            {
                error_ = std::string("the network could not start: ") + failure.what();
                return false;
            }
            servicesStarted_ = true;
        }
        GS::GamerServicesDispatcher::Update();
        return GS::Gamer::getSignedInGamersProperty()->getCountProperty() > 0;
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
            findResult_ = NetworkSession::BeginFind(NetworkSessionType::SystemLink, 1, Advertised(contentHash, 0), nullptr, {});
        }
        catch (const std::exception& failure)
        {
            error_ = std::string("no search: ") + failure.what();
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
            error_ = std::string("search failed: ") + failure.what();
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

    bool NetSession::Host(std::uint64_t contentHash, int world)
    {
        Leave();
        try
        {
            session_ = NetworkSession::Create(NetworkSessionType::SystemLink, 1, Net::kMaxPlayers, 0,
                                              Advertised(contentHash, world));
        }
        catch (const std::exception& failure)
        {
            error_ = std::string("the game could not be created: ") + failure.what();
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
        try
        {
            const auto& sessions = *collection;
            session_ = NetworkSession::Join(&sessions[foundIndex]);
        }
        catch (const std::exception& failure)
        {
            error_ = std::string("could not join: ") + failure.what();
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
            if (session_->getSessionStateProperty() == NetworkSessionState::Lobby) session_->StartGame();
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
            endReason_ = "the connection was lost";
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
        error_ = "this build has no network game";
        return false;
    }
    std::string NetSession::LocalName() const { return {}; }
    void NetSession::BeginFind(std::uint64_t) {}
    void NetSession::FinishFind() {}
    bool NetSession::Host(std::uint64_t, int) { return false; }
    bool NetSession::Join(int) { return false; }
    void NetSession::Subscribe() {}
    void NetSession::Leave() {}
    void NetSession::StartGame() {}
    bool NetSession::IsHost() const { return false; }
    bool NetSession::HasLocalGamer() const { return false; }
    std::uint8_t NetSession::LocalId() const { return 0; }
    std::string NetSession::NameOf(std::uint8_t) const { return {}; }
    void NetSession::Update() {}
    bool NetSession::Send(const Net::Message&) { return false; }
    std::vector<NetSession::Received> NetSession::TakeReceived() { return {}; }
    std::vector<std::pair<std::uint8_t, std::string>> NetSession::TakeJoined() { return {}; }
    std::vector<std::uint8_t> NetSession::TakeLeft() { return {}; }
    bool NetSession::TakeEnded(std::string&) { return false; }
}

#endif
