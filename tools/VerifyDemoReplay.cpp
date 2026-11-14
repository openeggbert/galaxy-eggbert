#include <GalaxyEggbert/Game/DemoReplay.hpp>
#include <GalaxyEggbert/Game/ResumeLog.hpp>

#include <cstdio>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

// The main menu's demos (DemoReplay.hpp): the file format and its compatibility rules, the idle
// timer, the input that stops a demo, and the order the demos play in. With --shipped (run from the
// repository root): every Content/demos/demoNNN.gdemo parses with the current simulation revision
// and names its world's current bytes. Replaying them in the game is tools/verify_demos.sh.
namespace
{
    using namespace GalaxyEggbert::Game;

    bool allOk = true;
    void check(bool condition, const char* what)
    {
        std::cout << (condition ? "  ok   " : "  FAIL ") << what << std::endl;
        allOk = allOk && condition;
    }

    std::optional<std::string> ReadFile(const std::string& path)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in) return std::nullopt;
        return std::string((std::istreambuf_iterator<char>(in)), {});
    }

    DemoRecording SampleDemo()
    {
        DemoRecording demo;
        demo.title = "World 1, \"mission\" 1";
        demo.gameVersion = "0.4.1";
        demo.recordedAt = "2026-11-14T09:30:00Z";
        demo.log.start.build = "recording build";
        demo.log.start.worldPath = "worlds3d/world011.vwr";
        demo.log.start.worldHash = ResumeLog::HashBytes("world bytes");
        demo.log.start.mission = 11;
        demo.log.start.seed = 123456789u;
        demo.log.start.x = 1.25f;
        demo.log.start.yaw = -1.5707963f;
        demo.log.start.saveData = "selectedGamer=1\ngamer1.doorsUnlocked=0101\n";
        ResumeInput input;
        input.keys = {37, 162};
        input.viewportWidth = 1280;
        input.viewportHeight = 720;
        demo.log.Append(input);
        demo.log.Append(input);
        input.keys = {38};
        demo.log.Append(input);
        demo.endSignature = HashDemoSignature({{"Blupi", "1 2 3"}, {"counters", "0 0 3"}});
        return demo;
    }

    std::string Reseal(const std::string& text, const std::string& from, const std::string& to)
    {
        auto body = text.substr(0, text.rfind("\nEND ") + 1);
        body.replace(body.find(from), from.size(), to);
        return ResumeLog::AppendChecksum(body);
    }

    int VerifyShipped()
    {
        int count = 0;
        while (count < kDemoMaxCount && ReadFile(DemoPath(count + 1))) ++count;
        check(count == kDemoSlotCount, "a demo ships for each of the ten positions (Content/demos/demo001-010)");
        for (int number = 1; number <= count; ++number)
        {
            const auto path = DemoPath(number);
            DemoRecording demo;
            const auto status = ParseDemo(*ReadFile(path), kDemoSimulationRevision, demo);
            std::cout << "  " << path << ": " << DemoLoadStatusText(status);
            if (status == DemoLoadStatus::Ok)
                std::cout << ", \"" << demo.title << "\", mission " << demo.log.start.mission << ", "
                          << demo.log.TickCount() << " ticks";
            std::cout << std::endl;
            check(status == DemoLoadStatus::Ok, "the demo parses with the current simulation revision");
            if (status != DemoLoadStatus::Ok) continue;
            char world[64];
            std::snprintf(world, sizeof(world), "worlds3d/world%03d.vwr", demo.log.start.mission);
            check(DemoWorldStatus(demo, world, ReadFile(world)) == DemoLoadStatus::Ok,
                  "its world is the shipped world, byte for byte");
            check(!demo.title.empty() && !demo.endSignature.empty(), "it has a title and an end signature");
        }
        // The game plays the ten positions: none may stand beyond them, or beyond a gap.
        bool beyond = false;
        for (int number = std::min(count, kDemoSlotCount) + 1; number <= kDemoMaxCount; ++number)
            beyond = beyond || ReadFile(DemoPath(number)).has_value();
        check(!beyond, "the demos are numbered without gaps, none beyond the ten positions");
        std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
        return allOk ? 0 : 1;
    }
}

int main(int argc, char** argv)
{
    if (argc > 1 && std::strcmp(argv[1], "--shipped") == 0) return VerifyShipped();

    // ---- the file format and its compatibility ----
    {
        const DemoRecording demo = SampleDemo();
        const std::string text = demo.Serialize();
        check(text.starts_with("GALAXY_DEMO 2\n"), "a demo writes its own header, format 2");
        DemoRecording parsed;
        check(ParseDemo(text, kDemoSimulationRevision, parsed) == DemoLoadStatus::Ok && parsed.title == demo.title &&
                  parsed.simulationRevision == kDemoSimulationRevision && parsed.endSignature == demo.endSignature &&
                  parsed.log.start == demo.log.start && parsed.log.runs == demo.log.runs &&
                  parsed.gameVersion == demo.gameVersion && parsed.recordedAt == demo.recordedAt && !parsed.worldBytes,
              "a demo round-trips its title, revision, end signature, start state and inputs");
        check(parsed.log.start.build == "recording build",
              "the recording build is kept for information, and no build has to match it");

        DemoRecording untouched = SampleDemo();
        untouched.title = "unchanged";
        check(ParseDemo(text, kDemoSimulationRevision + 1, untouched) == DemoLoadStatus::IncompatibleRevision &&
                  untouched.title == "unchanged",
              "another simulation revision is refused, transactionally");
        check(ParseDemo(Reseal(text, "GALAXY_DEMO 2", "GALAXY_DEMO 3"), kDemoSimulationRevision, untouched) ==
                  DemoLoadStatus::IncompatibleFormat, "another demo format is refused");
        bool truncated = true;
        for (std::size_t length = 0; length < text.size(); ++length)
            truncated = truncated && ParseDemo(text.substr(0, length), kDemoSimulationRevision, untouched) ==
                                         DemoLoadStatus::Corrupt;
        check(truncated && untouched.title == "unchanged", "every truncated demo is refused as damaged");
        bool flipped = true;
        for (std::size_t i = 0; i < text.size(); ++i)
        {
            auto damaged = text;
            damaged[i] ^= 1;
            flipped = flipped && ParseDemo(damaged, kDemoSimulationRevision, untouched) == DemoLoadStatus::Corrupt;
        }
        check(flipped && untouched.title == "unchanged", "a flipped byte anywhere is rejected transactionally");
        check(ParseDemo(text + "x", kDemoSimulationRevision, untouched) == DemoLoadStatus::Corrupt,
              "trailing bytes are refused");
        check(ParseDemo("", kDemoSimulationRevision, untouched) == DemoLoadStatus::Corrupt, "an empty file is refused");

        // Resume files and demos are distinct: neither is accepted as the other.
        ResumeLog resume = demo.log;
        check(ParseDemo(resume.Serialize(), kDemoSimulationRevision, untouched) == DemoLoadStatus::Corrupt,
              "a resume file is not a demo");
        ResumeLog notResume;
        check(!ResumeLog::Parse(text, "recording build", notResume), "a demo is not a resume file");
        check(ResumeLog::Parse(resume.Serialize(), "recording build", notResume) &&
                  !ResumeLog::Parse(resume.Serialize(), "another build", notResume),
              "resume files still require the identical build");

        DemoRecording fast = SampleDemo();
        fast.log.start.speed = 2;
        check(ParseDemo(fast.Serialize(), kDemoSimulationRevision, untouched) == DemoLoadStatus::Corrupt,
              "a demo recorded at another game speed is refused (demos play at the normal speed)");
        DemoRecording empty = SampleDemo();
        empty.log.runs.clear();
        check(ParseDemo(empty.Serialize(), kDemoSimulationRevision, untouched) == DemoLoadStatus::Corrupt,
              "a demo without input is refused");
        DemoRecording longDemo = SampleDemo();
        longDemo.log.runs[0].count = static_cast<std::uint32_t>(kDemoMaxTicks);
        check(ParseDemo(longDemo.Serialize(), kDemoSimulationRevision, untouched) == DemoLoadStatus::Corrupt,
              "a demo longer than ten minutes is refused");
        DemoRecording twice = SampleDemo();
        twice.endSignature.push_back(twice.endSignature.front());
        check(ParseDemo(twice.Serialize(), kDemoSimulationRevision, untouched) == DemoLoadStatus::Corrupt,
              "a signature naming an entry twice is refused");
        DemoRecording badInput = SampleDemo();
        badInput.log.runs[0].input.keys = {300};
        check(ParseDemo(badInput.Serialize(), kDemoSimulationRevision, untouched) == DemoLoadStatus::Corrupt,
              "out-of-range input is refused as in a resume file");

        check(DemoWorldStatus(demo, "worlds3d/world011.vwr", std::string("world bytes")) == DemoLoadStatus::Ok,
              "the recorded world's bytes are accepted");
        check(DemoWorldStatus(demo, "worlds3d/world011.vwr", std::string("world bytes!")) ==
                  DemoLoadStatus::WorldChanged, "a changed world is refused");
        check(DemoWorldStatus(demo, "worlds3d/world012.vwr", std::string("world bytes")) ==
                  DemoLoadStatus::WorldChanged, "another world's path is refused");
        check(DemoWorldStatus(demo, "worlds3d/world011.vwr", std::nullopt) == DemoLoadStatus::WorldMissing,
              "a missing world is reported");
        check(DemoPath(1) == "Content/demos/demo001.gdemo" && DemoPath(12) == "Content/demos/demo012.gdemo",
              "demos are numbered files in Content/demos/");
        check(UserDemoPath(3) == UserDemoDir() + "/demo003.gdemo" && UserDemoDir() == "demos" && kDemoSlotCount == 10,
              "the player's recording of position 3 is demos/demo003.gdemo beside savedata.txt (BD-0116)");
    }

    // ---- format 2 snapshots and the explicit revision opt-in ----
    {
        auto demo = SampleDemo();
        DemoRecording parsed;
        check(ParseDemo(demo.Serialize(), kDemoSimulationRevision + 1, parsed, true) == DemoLoadStatus::Ok &&
                  parsed.simulationRevision == demo.simulationRevision,
              "another revision can be explicitly accepted, with its original revision retained");
        auto legacyBody = *ResumeLog::VerifiedBody(demo.Serialize());
        legacyBody.replace(0, std::string("GALAXY_DEMO 2").size(), "GALAXY_DEMO 1");
        const auto metadata = legacyBody.find("VERSION ");
        legacyBody.erase(metadata, legacyBody.find("WORLD 0\n", metadata) + 8 - metadata);
        check(ParseDemo(ResumeLog::AppendChecksum(legacyBody), kDemoSimulationRevision, parsed) == DemoLoadStatus::Ok &&
                  parsed.gameVersion.empty() && parsed.recordedAt.empty() && !parsed.worldBytes &&
                  parsed.log.start == demo.log.start && parsed.log.runs == demo.log.runs,
              "format 1 still parses with absent metadata and snapshot");
        std::string binary;
        for (int i = 0; i < 256; ++i) binary += static_cast<char>(i);
        bool roundTrips = true;
        for (const auto& [bytes, encoded] : std::vector<std::pair<std::string, std::string>>{
                 {"", ""}, {"f", "Zg=="}, {"fo", "Zm8="}, {"foo", "Zm9v"},
                 {"foob", "Zm9vYg=="}, {"fooba", "Zm9vYmE="}, {"foobar", "Zm9vYmFy"}, {binary, ""}})
        {
            demo.worldBytes = bytes;
            demo.log.start.worldHash = ResumeLog::HashBytes(bytes);
            const auto text = demo.Serialize();
            roundTrips = roundTrips && ParseDemo(text, kDemoSimulationRevision, parsed) == DemoLoadStatus::Ok &&
                parsed.worldBytes == demo.worldBytes && parsed.gameVersion == demo.gameVersion &&
                parsed.recordedAt == demo.recordedAt && parsed.log.start == demo.log.start &&
                parsed.log.runs == demo.log.runs && parsed.endSignature == demo.endSignature &&
                (bytes == binary || text.find("WORLD 1 " + std::to_string(bytes.size()) + " \"" + encoded + "\"\n") != std::string::npos);
        }
        check(roundTrips, "base64 round trips empty, 1-3 byte tails and every binary byte; known encodings match");
        demo.worldBytes = "f";
        demo.log.start.worldHash = ResumeLog::HashBytes("f");
        const auto text = demo.Serialize();
        parsed.title = "unchanged";
        bool flipped = true;
        for (std::size_t i = 0; i < text.size(); ++i)
        {
            auto damaged = text;
            damaged[i] ^= 1;
            flipped = flipped && ParseDemo(damaged, kDemoSimulationRevision, parsed) == DemoLoadStatus::Corrupt;
        }
        check(flipped && parsed.title == "unchanged", "the footer covers every byte of a demo with an embedded world");
        for (const auto& invalid : {"=g==", "Zh==", "Zg=A", "Zg?=", "Zg", "Zg======"})
            check(ParseDemo(Reseal(text, "Zg==", invalid), kDemoSimulationRevision, parsed) == DemoLoadStatus::Corrupt &&
                      parsed.title == "unchanged", "malformed base64 is refused, even with a valid footer");
        check(ParseDemo(Reseal(text, "WORLD 1 1", "WORLD 1 2"), kDemoSimulationRevision, parsed) == DemoLoadStatus::Corrupt,
              "a wrong decoded length is refused");
        demo.worldBytes = "different";
        check(ParseDemo(demo.Serialize(), kDemoSimulationRevision, parsed) == DemoLoadStatus::Corrupt &&
                  ParseDemo(demo.Serialize(), kDemoSimulationRevision + 1, parsed, true) == DemoLoadStatus::Corrupt &&
                  parsed.title == "unchanged",
              "embedded bytes must match the world's hash, including with revision opt-in");
        demo.worldBytes = "f";
        check(DemoWorldStatus(demo, demo.log.start.worldPath, std::nullopt) == DemoLoadStatus::OwnWorld &&
                  DemoWorldStatus(demo, demo.log.start.worldPath, std::string("changed")) == DemoLoadStatus::OwnWorld &&
                  DemoWorldStatus(demo, demo.log.start.worldPath, std::string("f")) == DemoLoadStatus::Ok,
              "a missing or changed world uses its own copy; a matching original stays preferred");

        const auto home = std::filesystem::current_path();
        const auto here = home / "build-probe" / "verify-demo-cache";
        std::filesystem::remove_all(here);
        std::filesystem::create_directories(here);
        std::filesystem::current_path(here);
        const auto cached = CacheDemoWorld(demo);
        const auto expected = UserDemoDir() + "/worlds/" + std::to_string(demo.log.start.worldHash) + ".vwr";
        check(cached == expected && ReadFile(expected) == demo.worldBytes,
              "the embedded world is cached under demos/worlds/<hash>.vwr");
        if (cached)
        {
            const auto stamp = std::filesystem::file_time_type::clock::now() - std::chrono::hours(24);
            std::filesystem::last_write_time(*cached, stamp);
            check(CacheDemoWorld(demo) == cached && std::filesystem::last_write_time(*cached) == stamp,
                  "a matching cache file is reused without rewriting");
            { std::ofstream file(*cached, std::ios::binary); file << "damaged"; }
            check(CacheDemoWorld(demo) == cached && ReadFile(*cached) == demo.worldBytes &&
                      !std::filesystem::exists(*cached + ".tmp"), "a damaged cache is atomically replaced");
        }
        demo.worldBytes = "wrong hash";
        check(!CacheDemoWorld(demo) && ReadFile(expected) == std::optional<std::string>("f"),
              "invalid embedded bytes cannot overwrite the cache");
        demo.worldBytes.reset();
        check(!CacheDemoWorld(demo), "a recording without a world creates no cache file");
        std::filesystem::remove_all(UserDemoDir());
        { std::ofstream file(UserDemoDir()); file << "not a directory"; }
        demo.worldBytes = "f";
        check(!CacheDemoWorld(demo), "a cache write failure is reported without claiming a playable path");
        std::filesystem::current_path(home);
        std::filesystem::remove_all(here);
    }

    // ---- sharing recordings: export into demos/shared/, import into a position ----
    {
        // In a folder of its own: the build folder's demos/ may hold someone's recordings.
        const auto home = std::filesystem::current_path();
        const auto here = home / "verify-run" / "demo-share";
        std::error_code error;
        std::filesystem::remove_all(here, error);
        std::filesystem::create_directories(here, error);
        std::filesystem::current_path(here, error);
        auto recorded = SampleDemo();
        recorded.title = "Recorded by player A: mission 1"; // the automatic title: exported by player and position
        recorded.worldBytes = "world bytes";
        const std::string text = recorded.Serialize();
        std::filesystem::create_directories(UserDemoDir(), error);
        { std::ofstream out(UserDemoPath(4), std::ios::binary); out << text; }
        const auto exported = ExportDemo(UserDemoPath(4), 0, 4);
        const auto none = ExportDemo(UserDemoPath(5), 0, 5);
        std::ifstream copy(exported, std::ios::binary);
        const std::string copied((std::istreambuf_iterator<char>(copy)), {});
        check(SharedDemoDir() == "demos/shared" && DemoExportName(1, 7) == "B-demo-7.gdemo" &&
                  exported == "demos/shared/A-demo-4.gdemo" && copied == text && none.empty() &&
                  ListSharedDemos() == std::vector<std::string>{exported},
              "a position's demo exports as demos/shared/A-demo-4.gdemo; a position without a file exports nothing");
        const auto second = ExportDemo(UserDemoPath(4), 0, 4);
        check(second == "demos/shared/A-demo-4-2.gdemo" && std::filesystem::exists(exported) && ListSharedDemos().size() == 2,
              "a second export of position 4 is numbered (A-demo-4-2.gdemo), the first stays");
        std::filesystem::remove(second, error);
        // A demo named by the player (2026-11-10): its title in the file, its export by the name.
        check(SanitizeDemoName("  Fast:  Run!! 2 ") == "Fast Run 2" && IsAutomaticDemoTitle("Recorded by player B: mission 3") &&
                  !IsAutomaticDemoTitle("Fast Run") && DemoTitle(UserDemoPath(4)) == recorded.title,
              "demo names: letters, digits, spaces and '-' kept; the automatic title told apart");
        const bool renamed = RenameDemo(UserDemoPath(4), "Fast Run");
        DemoRecording reread;
        std::ifstream renamedFile(UserDemoPath(4), std::ios::binary);
        const std::string renamedText((std::istreambuf_iterator<char>(renamedFile)), {});
        check(renamed && !RenameDemo(UserDemoPath(4), "  ") && !RenameDemo(UserDemoPath(9), "Gone") &&
                  ParseDemo(renamedText, kDemoSimulationRevision, reread) == DemoLoadStatus::Ok &&
                  reread.title == "Fast Run" && reread.log.TickCount() == recorded.log.TickCount() &&
                  reread.endSignature == recorded.endSignature && reread.gameVersion == recorded.gameVersion &&
                  reread.recordedAt == recorded.recordedAt && reread.worldBytes == recorded.worldBytes,
              "RenameDemo writes the name as the title, the rest as it was (an empty name or no file: nothing)");
        const auto named = ExportDemo(UserDemoPath(4), 0, 4);
        check(named == "demos/shared/Fast-Run.gdemo", "a named demo exports as its name: Fast-Run.gdemo");
        std::filesystem::remove(named, error);
        { std::ofstream out(SharedDemoDir() + "/broken.gdemo", std::ios::binary); out << text.substr(0, text.size() / 2); }
        const auto refused = ImportDemo(SharedDemoDir() + "/broken.gdemo", 6);
        const auto missing = ImportDemo(SharedDemoDir() + "/nothing.gdemo", 6);
        const bool untouched = !std::filesystem::exists(UserDemoPath(6));
        const auto imported = ImportDemo(exported, 6);
        std::ifstream six(UserDemoPath(6), std::ios::binary);
        const std::string sixth((std::istreambuf_iterator<char>(six)), {});
        check(refused == DemoLoadStatus::Corrupt && missing == DemoLoadStatus::Missing && untouched &&
                  imported == DemoLoadStatus::Ok && sixth == text,
              "an import checks the file first: a damaged or missing one writes nothing, a good one becomes position 6");
        // Keep the serialized body byte-for-byte except the title, even for an older revision.
        auto olderDemo = recorded;
        olderDemo.simulationRevision = kDemoSimulationRevision + 7;
        const auto olderText = olderDemo.Serialize();
        const std::string olderPath = SharedDemoDir() + "/old.gdemo";
        { std::ofstream out(olderPath, std::ios::binary); out << olderText; }
        { std::ofstream out(SharedDemoDir() + "/My-run.gdemo", std::ios::binary); out << text; }
        { std::ofstream out(SharedDemoDir() + "/My-run-2.gdemo", std::ios::binary); out << text; }
        const auto renamedPath = RenameDemoFile(olderPath, "My run");
        const auto renamedBytes = ReadFile(renamedPath);
        auto expectedBody = *ResumeLog::VerifiedBody(olderText);
        const auto titleStart = expectedBody.find('"');
        expectedBody.replace(titleStart, recorded.title.size() + 2, "\"My run\"");
        check(renamedPath == SharedDemoDir() + "/My-run-3.gdemo" && !std::filesystem::exists(olderPath) &&
                  renamedBytes && *renamedBytes == ResumeLog::AppendChecksum(expectedBody) &&
                  ParseDemo(*renamedBytes, kDemoSimulationRevision, reread, true) == DemoLoadStatus::Ok &&
                  reread.title == "My run" && reread.simulationRevision == olderDemo.simulationRevision,
              "collection rename numbers collisions, changes only title/footer, validates checksum and keeps revision");
        check(RenameDemoFile(renamedPath, "My run").ends_with("My-run-4.gdemo"),
              "renaming again does not replace an existing same-title file");
        const auto samePath = SharedDemoDir() + "/Same.gdemo";
        { std::ofstream out(samePath, std::ios::binary); out << text; }
        check(RenameDemoFile(samePath, "Same") == samePath && RenameDemoFile(samePath, "!!!").empty(),
              "unchanged filename is allowed; empty sanitized name leaves the recording alone");
        const auto now = std::filesystem::file_time_type::clock::now();
        std::filesystem::last_write_time(exported, now + std::chrono::seconds(20));
        std::filesystem::last_write_time(samePath, now + std::chrono::seconds(10));
        const auto collection = ListDemoCollection([](DemoRecording& demo) { return demo.log.start.worldPath; });
        check(collection.size() >= 2 && collection[0].path == exported && collection[1].path == samePath &&
                  collection[0].name == "A-demo-4.gdemo" && collection[1].name == "Same" &&
                  collection[0].status == DemoLoadStatus::OwnWorld && !collection[0].metadata.empty(),
              "collection sorts newest first, uses filename for automatic titles, keeps version/date and own-world state");
        const auto oldEntry = std::find_if(collection.begin(), collection.end(), [](const auto& entry) { return entry.olderRevision; });
        check(oldEntry != collection.end() && oldEntry->Playable() && oldEntry->status == DemoLoadStatus::OwnWorld,
              "collection lists older revisions as playable with a warning");
        check(DeleteDemoFile(samePath) && !DeleteDemoFile(samePath) && !std::filesystem::exists(samePath),
              "collection delete removes the file and reports an absent file");
        const auto afterDelete = ListSharedDemos();
        check(std::find(afterDelete.begin(), afterDelete.end(), samePath) == afterDelete.end(),
              "deleted recording disappears on refresh");
        // Pause > Record's recording kept (replays 3/3): the name as its title and file, numbered when
        // taken, the fallback when nothing of the name is left; whole and readable, nothing temporary.
        const auto keptFirst = SaveDemoToCollection(recorded, "Garden run", "Mission 11");
        const auto keptSecond = SaveDemoToCollection(recorded, "Garden run", "Mission 11");
        const auto keptFallback = SaveDemoToCollection(recorded, "!!!", "Mission 11");
        std::string keptText;
        {
            std::ifstream in(keptFirst, std::ios::binary);
            keptText.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        }
        DemoRecording kept;
        const bool keptParsed = ParseDemo(keptText, kDemoSimulationRevision, kept) == DemoLoadStatus::Ok;
        bool temporaryLeft = false;
        for (const auto& entry : std::filesystem::directory_iterator(SharedDemoDir(), error))
            temporaryLeft = temporaryLeft || entry.path().extension() == ".tmp";
        check(keptFirst == SharedDemoDir() + "/Garden-run.gdemo" && keptSecond == SharedDemoDir() + "/Garden-run-2.gdemo" &&
                  keptFallback == SharedDemoDir() + "/Mission-11.gdemo" && DemoTitle(keptFirst) == "Garden run" &&
                  DemoTitle(keptFallback) == "Mission 11" && keptParsed && kept.worldBytes == recorded.worldBytes &&
                  kept.log.TickCount() == recorded.log.TickCount() && kept.gameVersion == recorded.gameVersion &&
                  !temporaryLeft,
              "a recording saved into My recordings: named, numbered, fallback name, whole, no temporary file");
        std::filesystem::current_path(home, error);
        std::filesystem::remove_all(here, error);
    }

    // ---- the end signature ----
    {
        const auto a = HashDemoSignature({{"Blupi", "1 2 3"}, {"camera", "4 5"}, {"objects", ""}});
        check(DemoSignatureDifferences(a, a).empty(), "an equal end state has no differences");
        const auto b = HashDemoSignature({{"Blupi", "1 2 3.5"}, {"camera", "4 5"}, {"objects", ""}});
        check(DemoSignatureDifferences(a, b) == std::vector<std::string>{"Blupi"}, "a changed entry is named");
        const auto c = HashDemoSignature({{"Blupi", "1 2 3"}, {"camera", "4 5"}, {"clocks", "0"}});
        check(DemoSignatureDifferences(a, c) == (std::vector<std::string>{"objects", "clocks"}),
              "missing and added entries are named");
    }

    // ---- the order the demos play in (DemoStep: the next one, the first after the last) ----
    {
        check(DemoOrderAfter(0, 3) == (std::vector<int>{1, 2, 3}), "a new session starts with the first demo");
        check(DemoOrderAfter(1, 3) == (std::vector<int>{2, 3, 1}), "after a demo the next one plays");
        check(DemoOrderAfter(3, 3) == (std::vector<int>{1, 2, 3}), "after the last the first plays again");
        check(DemoOrderAfter(1, 1) == (std::vector<int>{1}), "a single demo repeats");
        check(DemoOrderAfter(5, 3) == (std::vector<int>{1, 2, 3}), "an unknown current demo starts from the first");
        check(DemoOrderAfter(0, 0).empty(), "no demos, nothing to play");
    }

    // ---- the idle timer ----
    {
        DemoIdleTimer timer;
        bool fired = false;
        for (int i = 0; i < 159; ++i) fired = fired || timer.Update(0.25f, true, false);
        check(!fired, "no demo before 40 seconds of the idle menu");
        check(timer.Update(0.25f, true, false), "a demo starts after 40 seconds");
        check(timer.Elapsed() == 0.0f && !timer.Update(0.25f, true, false), "it fires once, then counts again");

        timer.Reset();
        for (int i = 0; i < 30; ++i) timer.Update(1.0f, true, false);
        timer.Update(0.016f, true, true);
        check(timer.Elapsed() == 0.0f, "activity starts the 40 seconds again");
        for (int i = 0; i < 39; ++i) fired = timer.Update(1.0f, true, false);
        check(!fired && timer.Update(1.0f, true, false), "the full 40 seconds count again from the activity");

        timer.Reset();
        for (int i = 0; i < 39; ++i) timer.Update(1.0f, true, false);
        check(!timer.Update(5.0f, false, false) && timer.Elapsed() == 0.0f,
              "outside the settled main menu (play, setup, editor, inactive window) nothing counts");
        check(!timer.Update(-3.0f, true, false) && timer.Elapsed() == 0.0f, "negative time is ignored");
    }

    // ---- live input: what resets the timer and what stops a demo ----
    {
        DemoLiveInput idle;
        idle.mouseX = 100;
        idle.mouseY = 200;
        check(!DemoActivity(idle, idle) && !DemoNewPress(idle, idle) && !DemoInputHeld(idle),
              "nothing done is neither activity nor a press");

        auto moved = idle;
        moved.mouseX = 101;
        check(DemoActivity(idle, moved), "moving the mouse is activity in the menu");
        check(!DemoNewPress(idle, moved), "moving the mouse does not stop a demo (TreatEvent ignores WM_MOUSEMOVE)");

        auto key = idle;
        key.keys = {32};
        check(DemoActivity(idle, key) && DemoNewPress(idle, key) && DemoInputHeld(key),
              "a key press is activity and stops a demo");
        check(!DemoNewPress(key, key), "a key already held when the demo started does not stop it");
        auto repeat = key;
        repeat.keyEvents = {32};
        check(!DemoNewPress(key, repeat), "the auto-repeat of a held key does not stop it");
        auto tap = idle;
        tap.keyEvents = {13};
        check(DemoActivity(idle, tap) && DemoNewPress(idle, tap) && !DemoInputHeld(tap),
              "a tap shorter than an update stops it");
        check(!DemoNewPress(key, idle), "releasing a key does not stop it");

        auto click = idle;
        click.mouseButtons = 1;
        check(DemoNewPress(idle, click) && DemoInputHeld(click), "a mouse button press stops it");
        auto right = click;
        right.mouseButtons = 1 | 4;
        check(DemoNewPress(click, right) && !DemoNewPress(right, click), "another button stops it, a release does not");
        auto wheel = idle;
        wheel.wheel = 120;
        check(DemoActivity(idle, wheel) && DemoNewPress(idle, wheel), "the wheel stops it");

        auto pad = idle;
        pad.padButtons = 1u << 10;
        check(DemoActivity(idle, pad) && DemoNewPress(idle, pad) && DemoInputHeld(pad), "a gamepad button stops it");
        check(!DemoNewPress(pad, pad), "a held gamepad button does not");

        auto finger = idle;
        finger.touches = 1;
        finger.touchPresses = 1;
        check(DemoActivity(idle, finger) && DemoNewPress(idle, finger) && DemoInputHeld(finger), "a touch stops it");
        auto held = finger;
        held.touchPresses = 0;
        check(!DemoNewPress(finger, held) && DemoInputHeld(held), "a finger left on the screen does not");
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
