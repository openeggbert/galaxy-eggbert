// The static terrain in chunks (TerrainRenderer::RebuildChangedChunks, OR-0074's rest): a change
// rebuilds only the chunks within its cells' reach (TerrainRenderer::ChunksReachedBy). That is right
// only if every chunk outside that reach is drawn the same after the change as before it. This checks
// it on real worlds without a GPU: each chunk's fingerprint (all the CPU makes of it, its meshes with
// the occlusion bake their upload gives them) before and after a cell is changed, for cells taken out
// and cells put in, many kinds; a chunk outside the reach whose fingerprint moved is a failure.
//
// Usage: VerifyTerrainChunks [source-dir]
#include <GalaxyEggbert/Game/BlockModels.hpp>
#include <GalaxyEggbert/Game/BrickEdges.hpp>
#include <GalaxyEggbert/Game/DirectionalCubeTiles.hpp>
#include <GalaxyEggbert/Game/TerrainRenderer.hpp>
#include <GalaxyEggbert/Game/TerrainRelief.hpp>
#include <GalaxyEggbert/Game/TileAtlas.hpp>

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace
{
    namespace G = GalaxyEggbert::Game;
    using Chunk = std::array<int, 3>;

    int failures = 0;
    void check(bool ok, const std::string& what)
    {
        if (!ok)
        {
            ++failures;
            std::cout << "FAIL: " << what << std::endl;
        }
    }

    struct Setup
    {
        G::TileAtlas atlas;
        std::vector<G::TileCoverage> coverage;
        std::vector<std::uint16_t> modelIcons;
        G::TerrainReliefTable reliefs;
    };

    // Every chunk's fingerprint.
    std::map<Chunk, std::uint64_t> Fingerprints(const G::TerrainRenderer::ChunkBuildInputs& inputs, int chunks,
                                                const std::set<Chunk>* only = nullptr)
    {
        std::map<Chunk, std::uint64_t> prints;
        for (int cy = 0; cy < chunks; ++cy)
            for (int cz = 0; cz < chunks; ++cz)
                for (int cx = 0; cx < chunks; ++cx)
                    if (only == nullptr || only->count({cx, cy, cz}) != 0)
                        prints[{cx, cy, cz}] = G::TerrainRenderer::ChunkFingerprint(inputs, cx, cy, cz, nullptr);
        return prints;
    }

    void CheckWorld(const Setup& setup, const std::string& path, int edits, int fullChecks, std::mt19937& random)
    {
        std::ifstream in(path, std::ios::binary);
        check(in.good(), path + " opens");
        if (!in.good()) return;
        auto world = GalaxyEggbert::Worlds::World::loadFromStream(in);
        const int n = world.blocksPerAxis();
        const int chunks = (n + G::TerrainRenderer::kStaticChunkCells - 1) / G::TerrainRenderer::kStaticChunkCells;
        const auto plates = G::TerrainRenderer::RotatedPlates(world);
        const G::TerrainRenderer::ChunkBuildInputs inputs{&world, &setup.atlas, &setup.coverage, false, &setup.modelIcons,
                                                         setup.reliefs.Empty() ? nullptr : &setup.reliefs, &plates};

        // The cells to change: solid cells (taken out) and air beside them (a block put in, of a kind the
        // world has), all over the world.
        std::vector<std::array<int, 3>> solid, open;
        std::vector<std::uint16_t> kinds;
        for (int y = 0; y < n; ++y)
            for (int z = 0; z < n; ++z)
                for (int x = 0; x < n; ++x)
                {
                    const auto block = world.getBlock(x, y, z);
                    if (!block.isAir())
                    {
                        solid.push_back({x, y, z});
                        kinds.push_back(block.type());
                        continue;
                    }
                    bool beside = false;
                    for (const auto& d : {std::array<int, 3>{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}})
                    {
                        const int nx = x + d[0], ny = y + d[1], nz = z + d[2];
                        beside = beside || (nx >= 0 && ny >= 0 && nz >= 0 && nx < n && ny < n && nz < n &&
                                            !world.getBlock(nx, ny, nz).isAir());
                    }
                    if (beside) open.push_back({x, y, z});
                }
        // The cells whose drawing looks farther than their neighbours: a brick end (its neighbours'
        // neighbours), a tile textured on one side (the room before each side), a still model (a front
        // model's opening, the floor under it). The changes near them are the ones a reach too short misses.
        std::vector<std::array<int, 3>> far;
        for (const auto& c : solid)
        {
            const int icon = world.getBlock(c[0], c[1], c[2]).type();
            const int room[4] = {8, 8, 8, 8};
            const int k = G::TerrainRenderer::kStaticChunkCells;
            const auto nearFace = [&](int v) { return v % k < 8 || v % k >= k - 8; };
            if ((nearFace(c[0]) || nearFace(c[2])) &&
                (G::BrickEdges::IsBrickEdgeIcon(icon) || G::SingleTexturedSide(icon, 0, room) >= 0 ||
                 std::find(setup.modelIcons.begin(), setup.modelIcons.end(), icon) != setup.modelIcons.end()))
                far.push_back(c);
        }
        std::sort(kinds.begin(), kinds.end());
        kinds.erase(std::unique(kinds.begin(), kinds.end()), kinds.end());
        check(!solid.empty() && !open.empty(), path + " has cells to change");
        if (solid.empty() || open.empty()) return;

        auto prints = Fingerprints(inputs, chunks);
        double reachedSum = 0;
        for (int e = 0; e < edits; ++e)
        {
            const std::uint64_t revision = world.blockRevision();
            std::array<int, 3> cell;
            std::uint16_t raw;
            if (e % 3 == 2 && !far.empty())
            {
                // Across the nearest chunk face along X or Z from one of them, 2 to 9 cells from it in
                // all, up to 4 below or above it: where a reach too short leaves a chunk as it was.
                const auto& f = far[random() % far.size()];
                const int k = G::TerrainRenderer::kStaticChunkCells;
                const int axis = static_cast<int>(random() % 2) * 2; // 0: X, 2: Z
                const int at = f[static_cast<std::size_t>(axis)];
                const int toLow = at % k + 1, toHigh = k - at % k;  // cells to step to leave its chunk
                const bool high = toHigh <= toLow;
                const int distance = std::max(high ? toHigh : toLow, 2) + static_cast<int>(random() % 3);
                cell = f;
                cell[static_cast<std::size_t>(axis)] = std::clamp(at + (high ? distance : -distance), 0, n - 1);
                cell[1] = std::clamp(f[1] + static_cast<int>(random() % 9) - 4, 0, n - 1);
                raw = world.getBlock(cell[0], cell[1], cell[2]).isAir()
                          ? GalaxyEggbert::Worlds::Block::make(kinds[random() % kinds.size()]).rawValue()
                          : GalaxyEggbert::Worlds::Block::make(0).rawValue();
            }
            else if (e % 2 == 0)
            {
                cell = solid[random() % solid.size()];
                raw = GalaxyEggbert::Worlds::Block::make(0).rawValue();
            }
            else
            {
                cell = open[random() % open.size()];
                raw = GalaxyEggbert::Worlds::Block::make(kinds[random() % kinds.size()]).rawValue();
            }
            const auto before = world.getBlock(cell[0], cell[1], cell[2]);
            world.setBlock(cell[0], cell[1], cell[2], GalaxyEggbert::Worlds::Block(raw));
            const auto changes = world.blockChangesSince(revision);
            check(changes.known && changes.cells.size() == 1, path + ": the change is in the journal");
            const auto reachedList = G::TerrainRenderer::ChunksReachedBy(inputs, changes, false);
            const std::set<Chunk> reached(reachedList.begin(), reachedList.end());
            reachedSum += static_cast<double>(reached.size());
            // The first edits check every chunk; the others the chunks around the reach (the ones a reach
            // too short would miss) -- all the farther ones are many times as slow to build.
            std::set<Chunk> look;
            if (e >= fullChecks)
            {
                for (const auto& [cx, cy, cz] : reached)
                    for (int dy = -1; dy <= 1; ++dy)
                        for (int dz = -1; dz <= 1; ++dz)
                            for (int dx = -1; dx <= 1; ++dx)
                            {
                                const Chunk c{cx + dx, cy + dy, cz + dz};
                                if (c[0] >= 0 && c[1] >= 0 && c[2] >= 0 && c[0] < chunks && c[1] < chunks && c[2] < chunks)
                                    look.insert(c);
                            }
            }
            const auto after = Fingerprints(inputs, chunks, e >= fullChecks ? &look : nullptr);
            int moved = 0;
            for (const auto& [chunk, print] : after)
            {
                if (reached.count(chunk) != 0)
                {
                    moved += print != prints[chunk] ? 1 : 0;
                    continue;
                }
                check(print == prints[chunk], path + ": cell (" + std::to_string(cell[0]) + "," + std::to_string(cell[1]) +
                                                  "," + std::to_string(cell[2]) + ") " + std::to_string(before.type()) + " -> " +
                                                  std::to_string(GalaxyEggbert::Worlds::Block(raw).type()) +
                                                  " changed chunk (" + std::to_string(chunk[0]) + "," +
                                                  std::to_string(chunk[1]) + "," + std::to_string(chunk[2]) +
                                                  ") outside its reach");
            }
            // A real change shows within its reach, unless it is between cells not drawn (the flight
            // barrier, invisible in play).
            const auto drawn = [](std::uint16_t type) { return type != 0 && type != GalaxyEggbert::BlockTypes::FlightBarrier; };
            if (before.rawValue() != raw && (drawn(before.type()) || drawn(GalaxyEggbert::Worlds::Block(raw).type())))
                check(moved > 0, path + ": the change of (" + std::to_string(cell[0]) + "," + std::to_string(cell[1]) + "," +
                                     std::to_string(cell[2]) + ") " + std::to_string(before.type()) + " -> " +
                                     std::to_string(GalaxyEggbert::Worlds::Block(raw).type()) + " moves a chunk within its reach");
            for (const auto& [chunk, print] : after) prints[chunk] = print;
        }
        std::cout << path << ": " << far.size() << " far-looking cells, " << edits << " changes, " << reachedSum / edits << " of " << chunks * chunks * chunks
                  << " chunks rebuilt each on average" << std::endl;
    }

    // The far-looking cells set across a chunk face on purpose (random changes in real worlds seldom
    // land where they look): a row of brick ends, a tile textured on one side between two rooms, a still
    // model with its floor, each in chunk 0 near X = 32, and changes in chunk 1 at each distance.
    void CheckFarCells(const Setup& setup)
    {
        GalaxyEggbert::Worlds::World world(7); // 70 cells: chunks of 32, 32 and 6
        const int n = world.blocksPerAxis();
        const auto put = [&](int x, int y, int z, int type) {
            world.setBlock(x, y, z, GalaxyEggbert::Worlds::Block::make(static_cast<std::uint16_t>(type)));
        };
        constexpr int kWall = 1;
        // Brick ends 262/263/283 in a row from x 26 to 39.
        for (int x = 26; x < 40; ++x) put(x, 5, 5, 262 + (x % 3 == 0 ? 0 : x % 3 == 1 ? 1 : 21));
        // A tile textured on one side at x 30 between a room of 7 to -X (a wall at 22) and the open to +X.
        int oneSided = -1;
        for (int icon = 1; icon < 450 && oneSided < 0; ++icon)
        {
            const int room[4] = {8, 8, 8, 8};
            if (G::SingleTexturedSide(icon, 0, room) >= 0) oneSided = icon;
        }
        check(oneSided > 0, "a tile textured on one side exists");
        put(30, 10, 10, oneSided);
        put(22, 10, 10, kWall);
        for (int x = 21; x < 42; ++x) { put(x, 10, 9, kWall); put(x, 10, 11, kWall); }
        // A still model at x 30 on a floor.
        const int model = G::kFrontModelIcons[0]; // a sign: it looks for its opening and its floor
        put(30, 20, 15, model);
        for (int x = 26; x < 42; ++x) put(x, 19, 15, kWall);
        // And one at y 33, over a chunk face below it, on a column of floor.
        put(12, 33, 12, model);
        for (int y = 26; y < 33; ++y) put(12, y, 12, kWall);

        const auto plates = G::TerrainRenderer::RotatedPlates(world);
        const G::TerrainRenderer::ChunkBuildInputs inputs{&world, &setup.atlas, &setup.coverage, false, &setup.modelIcons,
                                                         setup.reliefs.Empty() ? nullptr : &setup.reliefs, &plates};
        const int chunks = (n + G::TerrainRenderer::kStaticChunkCells - 1) / G::TerrainRenderer::kStaticChunkCells;
        auto prints = Fingerprints(inputs, chunks);
        int changes = 0;
        const auto change = [&](int x, int y, int z, int type) {
            const std::uint64_t revision = world.blockRevision();
            put(x, y, z, type);
            const auto journal = world.blockChangesSince(revision);
            if (journal.cells.empty()) return;
            ++changes;
            const auto reachedList = G::TerrainRenderer::ChunksReachedBy(inputs, journal, false);
            const std::set<Chunk> reached(reachedList.begin(), reachedList.end());
            const auto after = Fingerprints(inputs, chunks);
            for (const auto& [chunk, print] : after)
                if (reached.count(chunk) == 0)
                    check(print == prints[chunk], "far cells: (" + std::to_string(x) + "," + std::to_string(y) + "," +
                                                      std::to_string(z) + ") -> " + std::to_string(type) +
                                                      " changed chunk (" + std::to_string(chunk[0]) + "," +
                                                      std::to_string(chunk[1]) + "," + std::to_string(chunk[2]) +
                                                      ") outside its reach");
            prints = after;
        };
        for (int x = 32; x < 42; ++x)
        {
            for (const auto& [y, z, filled] : {std::array<int, 3>{5, 5, 262}, {10, 10, kWall}, {19, 15, kWall},
                                               {20, 15, kWall}, {21, 15, kWall}, {23, 15, kWall}})
            {
                const int old = world.getBlock(x, y, z).type();
                change(x, y, z, old == 0 ? filled : 0);
                change(x, y, z, old);
            }
        }
        for (int y = 27; y < 33; ++y)
        {
            const int old = world.getBlock(12, y, 12).type();
            change(12, y, 12, old == 0 ? kWall : 0);
            change(12, y, 12, old);
        }
        std::cout << "far cells: " << changes << " changes across a chunk face" << std::endl;
    }

    // Timing, for the record: the CPU's part of a whole world against a change's chunks.
    void Timing(const Setup& setup, const std::string& path)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in.good()) return;
        auto world = GalaxyEggbert::Worlds::World::loadFromStream(in);
        const int n = world.blocksPerAxis();
        const int chunks = (n + G::TerrainRenderer::kStaticChunkCells - 1) / G::TerrainRenderer::kStaticChunkCells;
        const auto plates = G::TerrainRenderer::RotatedPlates(world);
        const G::TerrainRenderer::ChunkBuildInputs inputs{&world, &setup.atlas, &setup.coverage, false, &setup.modelIcons,
                                                         setup.reliefs.Empty() ? nullptr : &setup.reliefs, &plates};
        const auto start = std::chrono::steady_clock::now();
        Fingerprints(inputs, chunks);
        const double whole = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        GalaxyEggbert::Worlds::World::BlockChanges one;
        one.known = true;
        one.cells.push_back({n / 2, n / 4, n / 2, 0, 0});
        const auto reached = G::TerrainRenderer::ChunksReachedBy(inputs, one, false);
        const std::set<Chunk> set(reached.begin(), reached.end());
        const auto partStart = std::chrono::steady_clock::now();
        Fingerprints(inputs, chunks, &set);
        const double part = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - partStart).count();
        std::cout << path << ": the whole world " << whole << " ms, a change in its middle (" << reached.size()
                  << " chunks) " << part << " ms (CPU, with the occlusion bake)" << std::endl;
    }
}

int main(int argc, char** argv)
{
    // The repository (its worlds3d/ and avatars3d/): the argument, else the one this was built from.
    std::filesystem::current_path(argc > 1 ? argv[1] : GE_SOURCE_DIR);
    Setup setup;
    // A coverage that sends tiles down every pass (opaque, cut out, blended): what holds for any sheet.
    setup.coverage.resize(450, G::TileCoverage::Opaque);
    for (std::size_t icon = 0; icon < setup.coverage.size(); ++icon)
    {
        if (icon % 7 == 3) setup.coverage[icon] = G::TileCoverage::Cutout;
        if (icon % 11 == 5) setup.coverage[icon] = G::TileCoverage::Translucent;
    }
    setup.modelIcons.assign(G::kStillModelIcons.begin(), G::kStillModelIcons.end());
    setup.modelIcons.insert(setup.modelIcons.end(), G::kSignModelIcons.begin(), G::kSignModelIcons.end());
    const int reliefs = setup.reliefs.Load("avatars3d/terrain");
    std::cout << "VerifyTerrainChunks: " << reliefs << " terrain reliefs" << std::endl;

    // Reach: one cell in an empty world reaches its own chunk, or the next one too at its side.
    {
        GalaxyEggbert::Worlds::World empty(10); // 100 cells
        const auto plates = G::TerrainRenderer::RotatedPlates(empty);
        const G::TerrainRenderer::ChunkBuildInputs inputs{&empty, &setup.atlas, &setup.coverage, false, &setup.modelIcons,
                                                         nullptr, &plates};
        GalaxyEggbert::Worlds::World::BlockChanges one;
        one.known = true;
        one.cells.push_back({48, 48, 48, 0, 0});
        check(G::TerrainRenderer::ChunksReachedBy(inputs, one, false).size() == 1,
              "a cell in a chunk's middle reaches that chunk alone");
        one.cells[0] = {33, 48, 48, 0, 0};
        check(G::TerrainRenderer::ChunksReachedBy(inputs, one, false).size() == 2,
              "a cell beside a chunk's side reaches the next one too");
        one.cells[0] = {40, 40, 40, 0, 0};
        check(G::TerrainRenderer::ChunksReachedBy(inputs, one, false).size() == 1 &&
                  G::TerrainRenderer::ChunksReachedBy(inputs, one, true).size() == 8,
              "Ultra's light reaches its rays' length (8 cells from a chunk's side, past it)");
    }

    CheckFarCells(setup);
    std::mt19937 random(20261112);
    CheckWorld(setup, "worlds3d/world999.vwr", 90, 4, random);
    CheckWorld(setup, "worlds3d/world065.vwr", 60, 2, random);
    CheckWorld(setup, "worlds3d/world105.vwr", 30, 2, random);
    CheckWorld(setup, "worlds3d/world025.vwr", 30, 2, random);
    Timing(setup, "worlds3d/world065.vwr");

    std::cout << "VerifyTerrainChunks: " << (failures == 0 ? "all passed" : "FAILED") << " (" << failures << " failures)"
              << std::endl;
    return failures == 0 ? 0 : 1;
}
