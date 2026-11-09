#pragma once

#include <GalaxyEggbert/BlockTypes.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace GalaxyEggbert::Game::BrickEdges
{
    // The brick wall's edge pieces (Q-011, the owner 2026-11-09: "ragged only towards the open").
    // 262 (left), 263 (right) and 283 (a column: both sides) end their four courses of bricks short
    // of the tile on their clear side; in the original's 2D the backdrop shows there, the ragged end
    // of a wall. The auto-tiling (ED-0007) puts 262 where the wall goes on to the right (+X or +Z)
    // and not to the left (-X and -Z), 263 the other way round, 283 where it goes on to neither.
    //
    // In 3D a cell of one of them is drawn as its four courses, each a box ending where its bricks end
    // (the toothing of a wall's end), on each of its clear sides that is the end of a wall: the side
    // borders air, so does a side across it (the wall's face: a cell in the middle of a face has the
    // wall on both sides along it), and for 262 and 263 the wall goes on opposite. The worlds also
    // scatter the three in brick masses for variety (tools/worlds station.py, village.py); in a face
    // they stay whole. Every other side, and every cell without such a side, is a whole cube (its
    // clear texels filled, TileCoverage.hpp).

    // A course is the texel rows [kCourseRows[k], kCourseRows[k + 1]) of the 64-texel tile, top first:
    // the mortar lines between the four courses of bricks.
    inline constexpr int kTileTexels = BlockTypes::kTileSize;
    inline constexpr int kCourseCount = 4;
    inline constexpr std::array<int, kCourseCount + 1> kCourseRows{0, 17, 33, 48, 64};

    // How far each course stops short of the tile's edge, in texels, on its left and right side:
    // the most common clear run of the course's rows, measured from Content/icons/object-m.png's
    // alpha (MeasureCourseInsets(); the game measures the sheet it loads again and says so if it
    // differs). Zero where a side is not ragged.
    struct TileInsets
    {
        int icon;
        std::array<int, kCourseCount> left;
        std::array<int, kCourseCount> right;
    };
    inline constexpr std::array<TileInsets, 3> kTiles{{
        {262, {5, 0, 8, 2}, {0, 0, 0, 0}},
        {263, {0, 0, 0, 0}, {0, 1, 10, 3}},
        {283, {7, 3, 0, 5}, {5, 1, 3, 0}},
    }};

    [[nodiscard]] inline const TileInsets* FindTile(int icon) noexcept
    {
        for (const auto& tile : kTiles)
            if (tile.icon == icon) return &tile;
        return nullptr;
    }

    [[nodiscard]] inline bool IsBrickEdgeIcon(int icon) noexcept { return FindTile(icon) != nullptr; }

    // The four horizontal sides, in CnaEngine::Graphics::BoxFace's order: +Z, -Z, +X, -X.
    enum Side : int
    {
        PositiveZ = 0,
        NegativeZ = 1,
        PositiveX = 2,
        NegativeX = 3,
    };
    inline constexpr std::array<int, 4> kSideDx{0, 0, 1, -1};
    inline constexpr std::array<int, 4> kSideDz{1, -1, 0, 0};
    [[nodiscard]] constexpr int Opposite(int side) noexcept { return side ^ 1; }
    // The texture's left side is the negative one of each axis (-X, -Z) on every face (the faces
    // are mapped by position, so a course's ragged end meets the bricks' ends in the picture).
    [[nodiscard]] constexpr bool IsLeftSide(int side) noexcept { return side == NegativeZ || side == NegativeX; }

    enum class Neighbour : std::uint8_t
    {
        Air,
        Solid,   // a block of the world (not air)
        Outside, // beyond the world's edge
    };

    struct Shape
    {
        bool ragged = false;
        std::uint8_t raggedSides = 0;                               // bit per Side
        std::array<std::array<float, 4>, kCourseCount> inset{};     // [course][side], in cell units
    };

    // The shape of a cell of @p icon whose horizontal neighbours are @p neighbour(side).
    template <class NeighbourAt>
    [[nodiscard]] Shape ShapeFor(int icon, NeighbourAt&& neighbour)
    {
        Shape shape;
        const TileInsets* tile = FindTile(icon);
        if (tile == nullptr) return shape;
        for (int side = 0; side < 4; ++side)
        {
            const bool left = IsLeftSide(side);
            const bool clearSide = icon == 283 || (icon == 262) == left;
            if (!clearSide || neighbour(side) != Neighbour::Air) continue;
            // A wall's end, not its face: open across too.
            const int across = side < 2 ? PositiveX : PositiveZ;
            if (neighbour(across) != Neighbour::Air && neighbour(Opposite(across)) != Neighbour::Air) continue;
            // 262 and 263: where the wall goes on behind.
            if (icon != 283 && neighbour(Opposite(side)) != Neighbour::Solid) continue;
            const auto& texels = left ? tile->left : tile->right;
            bool any = false;
            for (int course = 0; course < kCourseCount; ++course)
            {
                shape.inset[static_cast<std::size_t>(course)][static_cast<std::size_t>(side)] =
                    static_cast<float>(texels[static_cast<std::size_t>(course)]) / static_cast<float>(kTileTexels);
                any = any || texels[static_cast<std::size_t>(course)] > 0;
            }
            if (!any) continue;
            shape.raggedSides = static_cast<std::uint8_t>(shape.raggedSides | (1u << side));
        }
        if (shape.raggedSides == 0) shape.inset = {};
        shape.ragged = shape.raggedSides != 0;
        return shape;
    }

    // A rectangle of the cell's horizontal section, in cell units (0..1 on X and Z).
    struct Rect
    {
        float x0 = 0.0f, z0 = 0.0f, x1 = 1.0f, z1 = 1.0f;
    };
    inline constexpr float kSliver = 1e-4f;

    [[nodiscard]] inline Rect CourseRect(const Shape& shape, int course) noexcept
    {
        const auto& in = shape.inset[static_cast<std::size_t>(course)];
        return {in[NegativeX], in[NegativeZ], 1.0f - in[PositiveX], 1.0f - in[PositiveZ]};
    }

    // @p a without @p b: up to four rectangles (the strips beside b on X, then before and behind it
    // on Z within b's X range); slivers dropped.
    [[nodiscard]] inline std::vector<Rect> Subtract(const Rect& a, const Rect& b)
    {
        std::vector<Rect> out;
        const auto keep = [&out](Rect r) {
            if (r.x1 - r.x0 > kSliver && r.z1 - r.z0 > kSliver) out.push_back(r);
        };
        const float ox0 = std::max(a.x0, b.x0), ox1 = std::min(a.x1, b.x1);
        const float oz0 = std::max(a.z0, b.z0), oz1 = std::min(a.z1, b.z1);
        if (ox1 - ox0 <= kSliver || oz1 - oz0 <= kSliver)
        {
            keep(a);
            return out;
        }
        keep({a.x0, a.z0, ox0, a.z1});
        keep({ox1, a.z0, a.x1, a.z1});
        keep({ox0, a.z0, ox1, oz0});
        keep({ox0, oz1, ox1, a.z1});
        return out;
    }

    // The 1D counterpart for the caps on a side: [0, 1] without [from, to].
    [[nodiscard]] inline std::vector<std::pair<float, float>> Outside(float from, float to)
    {
        std::vector<std::pair<float, float>> out;
        if (from > kSliver) out.emplace_back(0.0f, from);
        if (to < 1.0f - kSliver) out.emplace_back(to, 1.0f);
        return out;
    }

    // Each course's most common clear run from the tile's left (or right) edge, in texels. @p alpha:
    // the tile's kTileTexels x kTileTexels alpha bytes, row by row, @p stride bytes apart; clear is
    // below @p threshold.
    [[nodiscard]] inline std::array<int, kCourseCount> MeasureCourseInsets(const std::uint8_t* alpha, int stride,
                                                                           bool left, int threshold)
    {
        std::array<int, kCourseCount> insets{};
        for (int course = 0; course < kCourseCount; ++course)
        {
            std::array<int, kTileTexels + 1> votes{};
            for (int row = kCourseRows[static_cast<std::size_t>(course)];
                 row < kCourseRows[static_cast<std::size_t>(course) + 1]; ++row)
            {
                int run = 0;
                while (run < kTileTexels)
                {
                    const int column = left ? run : kTileTexels - 1 - run;
                    if (alpha[static_cast<std::size_t>(row) * static_cast<std::size_t>(stride) +
                              static_cast<std::size_t>(column)] >= threshold)
                        break;
                    ++run;
                }
                ++votes[static_cast<std::size_t>(run)];
            }
            insets[static_cast<std::size_t>(course)] =
                static_cast<int>(std::max_element(votes.begin(), votes.end()) - votes.begin());
        }
        return insets;
    }
}
