// The network game's messages and lobby (src/GalaxyEggbert/Net/): every message survives a round
// trip, a damaged or foreign packet is refused whole, and the lobby follows the original's rules.
#include <GalaxyEggbert/Net/NetLobby.hpp>
#include <GalaxyEggbert/Net/NetProtocol.hpp>
#include <GalaxyEggbert/Net/NetReplication.hpp>
#include <GalaxyEggbert/Net/NetRules.hpp>

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
    lobby.lobby.world = 201;
    lobby.lobby.slots[0] = {true, true, 2, 17, "Ann"};
    lobby.lobby.slots[2] = {true, false, 0, 5, "Ben Two"};
    const auto lobbyBack = Decode(Encode(lobby));
    check(lobbyBack && lobbyBack->lobby.world == 201 && lobbyBack->lobby.slots[0].ready &&
              lobbyBack->lobby.slots[0].team == 2 && lobbyBack->lobby.slots[0].gamerId == 17 &&
              lobbyBack->lobby.slots[2].name == "Ben Two" && !lobbyBack->lobby.slots[1].present,
          "Lobby: the world (SB2's 201-212) and the four places come back");
    Message start;
    start.kind = MessageKind::Start;
    start.world = 212;
    start.worldHash = 99;
    const auto startBack = Decode(Encode(start));
    check(startBack && startBack->world == 212 && startBack->worldHash == 99, "Start: the multiplayer world comes back");
    start.world = 1000;
    check(!Decode(Encode(start)), "a world number past 999 is refused");

    Message pose;
    pose.kind = MessageKind::Pose;
    pose.pose.tick = 123456u;
    pose.pose.x = -12.5f;
    pose.pose.y = 3.25f;
    pose.pose.z = 40.0f;
    for (int i = 0; i < 12; ++i) pose.pose.world[static_cast<std::size_t>(i)] = 0.5f * static_cast<float>(i) - 2.0f;
    pose.pose.clip = 9;
    pose.pose.clipSeconds = 0.75f;
    pose.pose.alpha = 77;
    pose.pose.flags = kPoseShield;
    pose.pose.lives = 6;
    const auto poseBack = Decode(Encode(pose));
    check(poseBack && poseBack->pose.tick == 123456u && poseBack->pose.x == -12.5f && poseBack->pose.clip == 9 &&
              poseBack->pose.world == pose.pose.world &&
              poseBack->pose.clipSeconds == 0.75f && poseBack->pose.alpha == 77 && poseBack->pose.lives == 6 &&
              poseBack->pose.flags == kPoseShield && !IsReliable(MessageKind::Pose),
          "Pose: position, the model's matrix, clip, opacity, flags and lives come back; poses may be lost");

    Message pause;
    pause.kind = MessageKind::Pause;
    pause.ready = true;
    const auto pauseBack = Decode(Encode(pause));
    check(pauseBack && pauseBack->kind == MessageKind::Pause && pauseBack->ready && IsReliable(MessageKind::Pause),
          "Pause: paused comes back, and must arrive");

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

    // Speedy Blupi 2's rules.
    check(RemovedInMultiplayer(7) && RemovedInMultiplayer(12) && RemovedInMultiplayer(21) && RemovedInMultiplayer(32) &&
              RemovedInMultiplayer(33) && !RemovedInMultiplayer(13) && !RemovedInMultiplayer(28) &&
              !RemovedInMultiplayer(49) && !RemovedInMultiplayer(200),
          "a network game takes out crates, goals and enemy Blupis only (decblock.cpp:1430)");
    check(LastSurvivor(1) && !LastSurvivor(2) && LastSurvivor(0), "fewer than two present: the one left wins");
    check(IsReplicatedSpawn(28) && IsPersistentSpawn(13) && IsReplicatedSpawn(23) && !IsPersistentSpawn(23) &&
              IsReplicatedSpawn(56) && !IsPersistentSpawn(56) && !IsReplicatedSpawn(5),
          "what a player makes: bombs and vehicles left stay until taken; a fuse and a shot run out");
    check(DieNotice("Ann", 3) == "Ann has 3 lives left !" && DieNotice("Ann", 1) == "Ann has only one more life left !" &&
              DieNotice("Ann", 0) == "Ann has no life left !" && LeftNotice("Ben") == "Ben has left the game ..." &&
              LostNotice("Cid") == "Cid has been just knocked out ...",
          "the original's notices (sb2-eggbert-en.rc 257-261)");
    check(HashBytes("a") != HashBytes("b") && HashBytes("ab") == HashBytes("b", HashBytes("a")),
          "the content hash chains over the files");

    // Replication: one machine's step, the other machine's world.
    {
        using GalaxyEggbert::Game::MobileObjSpec;
        using GalaxyEggbert::Def::ObjectType;
        const auto object = [](ObjectType type, float x) {
            MobileObjSpec o;
            o.type = type;
            o.currentX = o.posStartX = o.posEndX = x;
            o.currentY = o.posStartY = o.posEndY = 1.0f;
            o.active = true;
            return o;
        };
        const std::vector<MobileObjSpec> loaded{object(ObjectType::ObjectType5, 1.0f), object(ObjectType::ObjectType29, 2.0f),
                                                object(ObjectType::ObjectType201, 3.0f)};
        GalaxyEggbert::Worlds::World worldA, worldB;
        auto objectsA = loaded, objectsB = loaded;
        Replicator a, b;
        a.Begin(objectsA, 0);
        b.Begin(objectsB, 1);
        a.Snapshot(worldA, objectsA);
        // A's step: a pickup taken, a wall blown, a bomb laid in the taken one's slot, a shot fired.
        objectsA[0].active = false;
        worldA.setBlock(10, 2, 20, GalaxyEggbert::Worlds::Block::make(10));
        objectsA[0] = object(ObjectType::ObjectType200, 4.0f);
        auto shot = object(ObjectType::ObjectType23, 5.0f);
        shot.posEndX = 9.0f;
        shot.firedByBlupi = true;
        objectsA.push_back(shot);
        auto enemyShot = object(ObjectType::ObjectType23, 6.0f);
        objectsA.push_back(enemyShot);
        const auto events = a.Diff(worldA, objectsA);
        int retired = 0, blocks = 0, spawned = 0;
        for (const auto& e : events)
        {
            retired += e.kind == EventKind::ObjectRetired;
            blocks += e.kind == EventKind::BlockChanged;
            spawned += e.kind == EventKind::ObjectSpawned;
        }
        check(retired == 1 && blocks == 1 && spawned == 2,
              "A's step: the pickup taken, the block changed, his bomb and his own shot (not another's)");
        // Through the wire, into B.
        Message m;
        m.kind = MessageKind::Events;
        m.events = events;
        const auto wire = Decode(Encode(m));
        check(wire && wire->events.size() == events.size(), "the events survive the wire");
        for (const auto& e : wire->events) b.Apply(e, worldB, objectsB);
        b.Snapshot(worldB, objectsB);
        bool bomb = false, shotThere = false;
        for (const auto& o : objectsB)
        {
            bomb = bomb || (o.active && o.type == ObjectType::ObjectType200 && o.currentX == 4.0f);
            shotThere = shotThere || (o.active && o.type == ObjectType::ObjectType23 && !o.firedByBlupi && o.posEndX == 9.0f);
        }
        check(!objectsB[0].active || objectsB[0].type != ObjectType::ObjectType5, "B: the pickup is gone");
        check(worldB.getBlock(10, 2, 20).type() == 10, "B: the block changed");
        check(bomb && shotThere, "B: A's bomb is there, and A's shot flies at B's Blupi");
        check(b.Diff(worldB, objectsB).empty(), "B sends nothing back of what it applied");
        // B's Blupi touches A's bomb: it goes, and A hears it.
        for (auto& o : objectsB)
            if (o.type == ObjectType::ObjectType200) o.active = false;
        const auto back = b.Diff(worldB, objectsB);
        check(back.size() == 1 && back[0].kind == EventKind::SpawnRetired, "B: A's bomb blown, sent as SpawnRetired");
        a.Snapshot(worldA, objectsA);
        for (const auto& e : back) a.Apply(e, worldA, objectsA);
        check(!objectsA[0].active, "A: his bomb is gone too");
        // A late retirement of the world's object whose slot now holds something else is refused.
        WorldEvent late;
        late.kind = EventKind::ObjectRetired;
        late.id = 0;
        auto again = objectsA;
        again[0] = object(ObjectType::ObjectType39, 7.0f);
        const int refusedBefore = a.Refused();
        a.Apply(late, worldA, again);
        check(again[0].active && a.Refused() == refusedBefore + 1, "a stale retirement does not remove what took its slot");
        WorldEvent outside;
        outside.kind = EventKind::BlockChanged;
        outside.a = 500;
        a.Apply(outside, worldA, objectsA);
        check(a.Refused() == refusedBefore + 2, "a block outside the world is refused");
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
