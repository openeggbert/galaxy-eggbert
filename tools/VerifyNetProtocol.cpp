// The network game's messages and lobby (src/GalaxyEggbert/Net/): every message survives a round
// trip, a damaged or foreign packet is refused whole, and the lobby follows the original's rules.
#include <GalaxyEggbert/Net/NetLobby.hpp>
#include <GalaxyEggbert/Net/NetProtocol.hpp>

#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>

using namespace GalaxyEggbert::Net;

int main()
{
    bool allOk = true;
    const auto check = [&allOk](bool cond, const char* what) {
        std::cout << (cond ? "PASS: " : "FAIL: ") << what << std::endl;
        allOk = allOk && cond;
    };

    // Round trips.
    Message hello;
    hello.kind = MessageKind::Hello;
    hello.simulationRevision = 7;
    hello.contentHash = 0x0123456789abcdefULL;
    hello.name = "Player A";
    const auto helloBack = Decode(Encode(hello));
    check(helloBack && helloBack->kind == MessageKind::Hello && helloBack->simulationRevision == 7 &&
              helloBack->contentHash == hello.contentHash && helloBack->name == "Player A",
          "Hello: revision, content hash and name come back");

    Message lobby;
    lobby.kind = MessageKind::Lobby;
    lobby.lobby.world = 3;
    lobby.lobby.slots[0] = {true, true, 2, 17, "Ann"};
    lobby.lobby.slots[2] = {true, false, 0, 5, "Ben Two"};
    const auto lobbyBack = Decode(Encode(lobby));
    check(lobbyBack && lobbyBack->lobby.world == 3 && lobbyBack->lobby.slots[0].ready &&
              lobbyBack->lobby.slots[0].team == 2 && lobbyBack->lobby.slots[0].gamerId == 17 &&
              lobbyBack->lobby.slots[2].name == "Ben Two" && !lobbyBack->lobby.slots[1].present,
          "Lobby: the world and the four places come back");

    Message pose;
    pose.kind = MessageKind::Pose;
    pose.pose = {123456u, -12.5f, 3.25f, 40.0f, 1.5f, 9, 0.75f, 77, kPoseShield, 6};
    const auto poseBack = Decode(Encode(pose));
    check(poseBack && poseBack->pose.tick == 123456u && poseBack->pose.x == -12.5f && poseBack->pose.clip == 9 &&
              poseBack->pose.clipSeconds == 0.75f && poseBack->pose.alpha == 77 && poseBack->pose.lives == 6 &&
              poseBack->pose.flags == kPoseShield && !IsReliable(MessageKind::Pose),
          "Pose: position, clip, opacity, flags and lives come back; poses may be lost");

    Message events;
    events.kind = MessageKind::Events;
    events.events.push_back({EventKind::ObjectRetired, 42});
    events.events.push_back({EventKind::BlockChanged, 0, 10, 2, 30, 0x1234});
    events.events.push_back({EventKind::ObjectSpawned, (1 << 24) | 5, 201, 0, 0, 0, 1.5f, 2.0f, -3.0f});
    const auto eventsBack = Decode(Encode(events));
    check(eventsBack && eventsBack->events.size() == 3 && eventsBack->events[1].d == 0x1234 &&
              eventsBack->events[2].a == 201 && eventsBack->events[2].z == -3.0f && IsReliable(MessageKind::Events),
          "Events: retired objects, changed blocks and spawned objects come back; they must arrive");

    Message chat;
    chat.kind = MessageKind::Chat;
    chat.text = std::string(150, 'x') + "\x01";
    const auto chatBack = Decode(Encode(chat));
    check(chatBack && chatBack->text.size() == kMaxChat, "Chat: a long line is cut to 99 characters, control bytes dropped");

    // Damaged and foreign packets.
    auto bytes = Encode(hello);
    bool truncatedRefused = true;
    for (std::size_t n = 0; n < bytes.size(); ++n) truncatedRefused = truncatedRefused && !Decode(bytes.data(), n);
    check(truncatedRefused, "every truncated Hello is refused");
    auto longer = bytes;
    longer.push_back(0);
    check(!Decode(longer), "a byte too many is refused");
    auto foreign = bytes;
    foreign[0] ^= 1;
    check(!Decode(foreign), "another game's protocol id is refused");
    auto newer = bytes;
    newer[2] = kProtocolVersion + 1;
    check(!Decode(newer), "another protocol version is refused");
    Message badTeam;
    badTeam.kind = MessageKind::Team;
    auto teamBytes = Encode(badTeam);
    teamBytes.back() = 4;
    check(!Decode(teamBytes), "a colour out of 0-3 is refused");
    auto nanPose = Encode(pose);
    const float nan = std::numeric_limits<float>::quiet_NaN();
    std::uint32_t bits = 0;
    std::memcpy(&bits, &nan, 4);
    for (int i = 0; i < 4; ++i) nanPose[8 + i] = static_cast<std::uint8_t>(bits >> (8 * i));
    check(!Decode(nanPose), "a pose with a NaN coordinate is refused");
    auto badEvent = Encode(events);
    badEvent[5] = 9; // the first event's kind
    check(!Decode(badEvent), "an unknown event kind is refused (the whole packet)");
    Message empty;
    empty.kind = MessageKind::Events;
    check(!Decode(Encode(empty)), "an Events packet without events is refused");

    // The lobby.
    Lobby host;
    check(host.Add(1, "Ann") == 0 && host.Add(2, "Ben") == 1 && host.Add(1, "Ann") == 0 && host.Present() == 2,
          "players take the first free places; the same player keeps his");
    check(host.state.slots[1].team == 1, "each place starts with its own colour");
    check(!host.CanStart(), "nobody ready: no start");
    host.SetReady(1, true);
    check(!host.CanStart(), "one of two ready: no start");
    host.SetReady(2, true);
    host.SetTeam(2, 0);
    check(host.CanStart() && host.state.slots[1].team == 0, "two ready: start; colours may be the same (as the original)");
    host.Add(3, "Cid");
    check(!host.CanStart(), "a third player not ready yet holds the start");
    host.Remove(3);
    check(host.CanStart() && host.Add(4, "Dee") == 2, "he left: start again; his place is free");
    host.Add(5, "Eve");
    check(host.Add(6, "Fay") == -1, "a fifth player finds no place");
    host.ClearReady();
    check(!host.CanStart(), "after a match nobody is ready");

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
