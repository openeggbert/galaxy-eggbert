// The network game's robustness against broken and hostile packets (owner_questions.md Q-022):
// random bytes and damaged copies of every valid message go through the decoder, and the world
// changes it lets through go into a replicated world. Nothing may crash (run it under ASan/UBSan
// too: tools/fuzz_net_sanitized.sh), the decoder takes only exact messages of ours (what it takes
// encodes back to the same bytes, every value in its range), and the world takes only what fits it.
//
//   FuzzNetProtocol [packets] [seed]     # 200000 and 1 by default (CTest: the defaults)
#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
#include <GalaxyEggbert/Net/NetProtocol.hpp>
#include <GalaxyEggbert/Net/NetReplication.hpp>
#include <GalaxyEggbert/Net/NetRules.hpp>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace GalaxyEggbert::Net;

namespace
{
    std::vector<Message> Corpus()
    {
        std::vector<Message> corpus;
        Message m;
        m.kind = MessageKind::Hello;
        m.simulationRevision = kProtocolVersion;
        m.contentHash = 0x0123456789abcdefULL;
        m.name = "Player A";
        corpus.push_back(m);
        m = {};
        m.kind = MessageKind::Lobby;
        m.lobby.world = 205;
        for (int i = 0; i < kMaxPlayers; ++i)
        {
            auto& s = m.lobby.slots[static_cast<std::size_t>(i)];
            s.present = i < 3;
            s.ready = i == 1;
            s.playing = i == 2;
            s.team = static_cast<std::uint8_t>(i);
            s.gamerId = static_cast<std::uint8_t>(10 + i);
            s.name = i < 3 ? "Player " + std::string(1, static_cast<char>('A' + i)) : "";
        }
        corpus.push_back(m);
        m = {};
        m.kind = MessageKind::Team;
        m.team = 2;
        corpus.push_back(m);
        m = {};
        m.kind = MessageKind::Ready;
        m.ready = true;
        corpus.push_back(m);
        m = {};
        m.kind = MessageKind::Chat;
        m.text = "hello, everyone: 1 2 3";
        corpus.push_back(m);
        m = {};
        m.kind = MessageKind::Start;
        m.world = 212;
        m.worldHash = 0xfeedfacecafebeefULL;
        corpus.push_back(m);
        m = {};
        m.kind = MessageKind::Pose;
        m.pose.tick = 1234;
        m.pose.x = 1.5f;
        m.pose.y = 10.0f;
        m.pose.z = -3.25f;
        for (std::size_t i = 0; i < m.pose.world.size(); ++i) m.pose.world[i] = static_cast<float>(i) * 0.5f;
        m.pose.clip = 7;
        m.pose.clipSeconds = 0.75f;
        m.pose.alpha = 128;
        m.pose.flags = kPoseShield | kPoseGlued;
        m.pose.lives = 9;
        m.pose.aura = 3;
        corpus.push_back(m);
        m = {};
        m.kind = MessageKind::Events;
        m.events.push_back({EventKind::ObjectRetired, 3});
        m.events.push_back({EventKind::BlockChanged, 0, 10, 2, 30, 0x0012});
        m.events.push_back({EventKind::ObjectSpawned, (1 << 24) | 5, 200, 0, 0, 0, 1.5f, 2.0f, -3.0f});
        m.events.push_back({EventKind::ObjectSpawned, (1 << 24) | 6, 23, 512, 256, -768, 1.0f, 1.0f, 1.0f});
        m.events.push_back({EventKind::SpawnRetired, (1 << 24) | 5});
        corpus.push_back(m);
        m = {};
        m.kind = MessageKind::Die;
        m.lives = 4;
        m.killer = 1;
        corpus.push_back(m);
        m = {};
        m.kind = MessageKind::Lost;
        m.killer = kNoKiller;
        corpus.push_back(m);
        m = {};
        m.kind = MessageKind::Abandon;
        corpus.push_back(m);
        m = {};
        m.kind = MessageKind::Pause;
        m.ready = true;
        corpus.push_back(m);
        m = {};
        m.kind = MessageKind::Sounds;
        m.sounds.push_back({10, false, 1.0f, 2.0f, 3.0f});
        m.sounds.push_back({92, true, -1.0f, 0.0f, 5.0f});
        corpus.push_back(m);
        m = {};
        m.kind = MessageKind::Lifts;
        m.lifts.push_back({4, 1});
        m.lifts.push_back({17, 3});
        corpus.push_back(m);
        m = {};
        m.kind = MessageKind::StopCloud;
        m.target = 12;
        corpus.push_back(m);
        return corpus;
    }

    bool Printable(const std::string& text, std::size_t limit)
    {
        if (text.size() > limit) return false;
        for (const char c : text)
            if (c < 0x20 || c > 0x7e) return false;
        return true;
    }

    bool Finite(float v) { return std::isfinite(v) && std::fabs(v) < 1.0e6f; }

    // Every value of a decoded message in its range (what the game then trusts).
    const char* Invalid(const Message& m)
    {
        if (!Printable(m.name, kMaxName)) return "a name out of range";
        if (!Printable(m.text, kMaxChat)) return "a chat line out of range";
        switch (m.kind)
        {
        case MessageKind::Lobby:
            if (m.lobby.world > 999) return "a lobby world past 999";
            for (const auto& s : m.lobby.slots)
                if (s.team >= kTeams || !Printable(s.name, kMaxName)) return "a lobby place out of range";
            break;
        case MessageKind::Team:
            if (m.team >= kTeams) return "a team past 3";
            break;
        case MessageKind::Start:
            if (m.world > 999) return "a world past 999";
            break;
        case MessageKind::Pose:
            if (!Finite(m.pose.x) || !Finite(m.pose.y) || !Finite(m.pose.z) || !Finite(m.pose.clipSeconds))
                return "a pose not finite";
            for (const float v : m.pose.world)
                if (!Finite(v)) return "a pose matrix not finite";
            if (m.pose.flags > kPoseFlags || m.pose.aura > kAuraLimit) return "pose flags out of range";
            break;
        case MessageKind::Events:
            if (m.events.empty() || m.events.size() > kMaxEvents) return "an events count out of range";
            for (const auto& e : m.events)
                if (!Finite(e.x) || !Finite(e.y) || !Finite(e.z)) return "an event's place not finite";
            break;
        case MessageKind::Die:
        case MessageKind::Lost:
            if (m.killer != kNoKiller && m.killer >= kMaxPlayers) return "a killer past 3";
            break;
        case MessageKind::Sounds:
            if (m.sounds.empty() || m.sounds.size() > kMaxSounds) return "a sounds count out of range";
            for (const auto& s : m.sounds)
                if (s.channel >= kSoundChannels || !Finite(s.x) || !Finite(s.y) || !Finite(s.z)) return "a sound out of range";
            break;
        case MessageKind::Lifts:
            if (m.lifts.empty() || m.lifts.size() > kMaxLifts) return "a lifts count out of range";
            for (const auto& l : m.lifts)
                if (l.index < 0 || (l.step != 1 && l.step != 3)) return "a lift out of range";
            break;
        default:
            break;
        }
        return nullptr;
    }
}

int main(int argc, char** argv)
{
    const long packets = argc > 1 ? std::atol(argv[1]) : 200000;
    const unsigned seed = argc > 2 ? static_cast<unsigned>(std::atol(argv[2])) : 1u;
    std::mt19937 rng(seed);
    const auto below = [&rng](std::size_t n) { return n == 0 ? std::size_t{0} : static_cast<std::size_t>(rng() % n); };

    const auto corpus = Corpus();
    std::vector<std::vector<std::uint8_t>> valid;
    bool corpusOk = true;
    for (const auto& m : corpus)
    {
        valid.push_back(Encode(m));
        corpusOk = corpusOk && Decode(valid.back()).has_value();
    }

    // A world of the size the game's are, with some objects of its own, for the events let through.
    GalaxyEggbert::Worlds::World world;
    std::vector<GalaxyEggbert::Game::MobileObjSpec> objects(40);
    for (std::size_t i = 0; i < objects.size(); ++i)
    {
        objects[i].type = static_cast<GalaxyEggbert::Def::ObjectType>(i % 2 == 0 ? 5 : 29);
        objects[i].active = true;
    }
    Replicator replicator;
    replicator.Begin(objects, 0);
    const std::size_t baseObjects = objects.size();

    const auto& registry = GalaxyEggbert::BlockDefinitionRegistry::Instance();
    const int n = static_cast<int>(world.blocksPerAxis());
    // A world change applied: a changed cell holds a block the game knows.
    bool blocksKnown = true;
    const auto applyChecked = [&](const WorldEvent& e) {
        replicator.Apply(e, world, objects);
        if (e.kind == EventKind::BlockChanged && e.a >= 0 && e.b >= 0 && e.c >= 0 && e.a < n && e.b < n && e.c < n)
            blocksKnown = blocksKnown && registry.IsSupported(world.getBlock(static_cast<std::uint16_t>(e.a), static_cast<std::uint16_t>(e.b),
                                                                             static_cast<std::uint16_t>(e.c)).type());
    };

    long taken = 0, refused = 0, events = 0;
    long failures = 0;
    std::string firstFailure;
    const auto fail = [&](const std::string& what, const std::vector<std::uint8_t>& bytes) {
        if (failures++ == 0)
        {
            firstFailure = what + " (" + std::to_string(bytes.size()) + " bytes:";
            for (const auto b : bytes) firstFailure += " " + std::to_string(b);
            firstFailure += ")";
        }
    };

    for (long p = 0; p < packets; ++p)
    {
        std::vector<std::uint8_t> bytes;
        const auto how = below(10);
        if (how < 2)
        {
            // Noise; half of it behind our envelope (protocol id, version, a kind), to get past it.
            bytes.resize(below(300));
            for (auto& b : bytes) b = static_cast<std::uint8_t>(rng());
            if (how == 1 && bytes.size() >= 4)
            {
                bytes[0] = static_cast<std::uint8_t>(kProtocolId & 0xff);
                bytes[1] = static_cast<std::uint8_t>(kProtocolId >> 8);
                bytes[2] = kProtocolVersion;
                bytes[3] = static_cast<std::uint8_t>(below(18));
            }
        }
        else
        {
            bytes = valid[below(valid.size())];
            const auto edits = 1 + below(4);
            for (std::size_t k = 0; k < edits && !bytes.empty(); ++k)
            {
                switch (below(7))
                {
                case 0: bytes[below(bytes.size())] ^= static_cast<std::uint8_t>(1u << below(8)); break; // a bit flipped
                case 1: bytes[below(bytes.size())] = static_cast<std::uint8_t>(rng()); break;           // a byte changed
                case 2: bytes[below(bytes.size())] = static_cast<std::uint8_t>(below(2) ? 0xff : 0); break;
                case 3: bytes.resize(below(bytes.size() + 1)); break;                                   // cut short
                case 4: bytes.insert(bytes.begin() + static_cast<long>(below(bytes.size() + 1)), static_cast<std::uint8_t>(rng())); break;
                case 5: bytes.erase(bytes.begin() + static_cast<long>(below(bytes.size()))); break;
                default:
                {
                    // A float's bytes made NaN, infinite or huge somewhere past the envelope.
                    if (bytes.size() < 8) break;
                    const std::uint32_t specials[] = {0x7fc00000u, 0x7f800000u, 0xff800000u, 0x7f7fffffu, 0x80000000u};
                    const std::uint32_t v = specials[below(5)];
                    const auto at = 4 + below(bytes.size() - 7);
                    std::memcpy(&bytes[at], &v, 4);
                    break;
                }
                }
            }
        }

        const auto message = Decode(bytes);
        if (!message)
        {
            ++refused;
            continue;
        }
        ++taken;
        if (const char* why = Invalid(*message)) fail(std::string("taken with ") + why, bytes);
        if (Encode(*message) != bytes) fail("taken, but it does not encode back to the same bytes", bytes);
        if (message->kind == MessageKind::Events)
        {
            replicator.Snapshot(world, objects);
            for (const auto& e : message->events)
            {
                applyChecked(e);
                ++events;
            }
            if (objects.size() > baseObjects + Replicator::kMaxSpawned) fail("the world grew past its spawn limit", bytes);
        }
    }

    // The replication on its own (a caller that did not decode): any event at all, extremes included.
    const std::int32_t extremes[] = {0, 1, -1, 2, 23, 200, 255, 256, 1023, 65535, 65536, 0x7fffffff, -0x7fffffff - 1};
    const auto any = [&]() { return below(3) == 0 ? extremes[below(std::size(extremes))] : static_cast<std::int32_t>(rng()); };
    for (long k = 0; k < packets / 4; ++k)
    {
        WorldEvent e;
        e.kind = static_cast<EventKind>(1 + below(4));
        e.id = any();
        e.a = below(2) ? any() : static_cast<std::int32_t>(below(static_cast<std::size_t>(n) + 2));
        e.b = below(2) ? any() : static_cast<std::int32_t>(below(static_cast<std::size_t>(n) + 2));
        e.c = below(2) ? any() : static_cast<std::int32_t>(below(static_cast<std::size_t>(n) + 2));
        e.d = any();
        e.x = static_cast<float>(any());
        e.y = static_cast<float>(any());
        e.z = static_cast<float>(any());
        if (k % 64 == 0) replicator.Snapshot(world, objects);
        applyChecked(e);
        ++events;
    }
    const bool directHeld = objects.size() <= baseObjects + Replicator::kMaxSpawned;

    // A flood of spawns from one player: the world stops growing at its limit.
    Message flood;
    flood.kind = MessageKind::Events;
    for (std::size_t i = 0; i < kMaxEvents; ++i)
        flood.events.push_back({EventKind::ObjectSpawned, static_cast<std::int32_t>((1u << 24) | (i + 1)), 23, 0, 0, 0, 0.0f, 0.0f, 0.0f});
    for (auto& o : objects) o.active = true;
    for (int round = 0; round < 64; ++round)
    {
        const auto wire = Decode(Encode(flood));
        for (const auto& e : wire->events) replicator.Apply(e, world, objects);
    }
    const bool floodHeld = objects.size() <= baseObjects + Replicator::kMaxSpawned;

    bool allOk = true;
    const auto check = [&allOk](bool cond, const std::string& what) {
        std::cout << (cond ? "PASS: " : "FAIL: ") << what << std::endl;
        allOk = allOk && cond;
    };
    check(corpusOk, "every message of the corpus decodes");
    check(failures == 0, std::to_string(packets) + " random and damaged packets (seed " + std::to_string(seed) + "): " +
                             std::to_string(taken) + " taken, each exact and in range; " + std::to_string(refused) +
                             " refused; " + std::to_string(events) + " world changes applied or refused" +
                             (failures ? "; " + std::to_string(failures) + " wrong, the first " + firstFailure : ""));
    check(taken > 0 && refused > 0, "the damage both passes (values still valid) and is caught");
    check(blocksKnown, "every block a change put in the world is one the game knows");
    check(directHeld, "events given to the replication directly, extremes included, keep the world in bounds");
    check(floodHeld, "a flood of spawned objects stops at the limit (" + std::to_string(objects.size() - baseObjects) + " added)");
    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
