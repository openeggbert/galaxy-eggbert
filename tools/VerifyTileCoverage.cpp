// The terrain's tile coverage (src/GalaxyEggbert/Game/TileCoverage.hpp): which object-m.png
// tiles are drawn opaque, cut out (alpha test) or blended, read from the sheet's alpha.
#include <GalaxyEggbert/Game/BrickEdges.hpp>
#include <GalaxyEggbert/Game/TileCoverage.hpp>

#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

namespace
{
    using GalaxyEggbert::Game::TileCoverage;

    constexpr int kTile = GalaxyEggbert::BlockTypes::kTileSize;
    constexpr int kGap = GalaxyEggbert::BlockTypes::kSheetGap;
    constexpr int kPitch = kTile + kGap;

    // A sheet of `columns` x `rows` icons laid out as object-m.png, every texel opaque, the gaps
    // clear.
    struct Sheet
    {
        int width;
        int height;
        std::vector<std::uint8_t> alpha;

        Sheet(int columns, int rows)
            : width(kGap + columns * kPitch), height(kGap + rows * kPitch),
              alpha(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0)
        {
            for (int icon = 0; icon < columns * rows; ++icon)
            {
                Fill(icon, columns, 0, kTile, 255);
            }
        }

        // Sets the alpha of icon's texel rows [rowFrom, rowTo).
        void Fill(int icon, int columns, int rowFrom, int rowTo, std::uint8_t value)
        {
            const int x0 = kGap + (icon % columns) * kPitch;
            const int y0 = kGap + (icon / columns) * kPitch;
            for (int y = y0 + rowFrom; y < y0 + rowTo; ++y)
            {
                for (int x = x0; x < x0 + kTile; ++x)
                {
                    alpha[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)] =
                        value;
                }
            }
        }
    };
}

int main()
{
    using GalaxyEggbert::Game::ClassifyTileSheet;
    using GalaxyEggbert::Game::CoverageOf;

    bool allOk = true;
    const auto check = [&allOk](bool condition, const char* what)
    {
        std::cout << (condition ? "PASS" : "FAIL") << ": " << what << std::endl;
        allOk = allOk && condition;
    };

    constexpr int kColumns = 3;
    Sheet sheet(kColumns, 2);
    sheet.Fill(1, kColumns, 0, 13, 0);              // ~20 % clear: a stone wall's gaps
    sheet.Fill(2, kColumns, 0, 20, 0);              // ~31 % clear: a door on clear texels
    sheet.Fill(3, kColumns, 0, 64, 0);              // a cobweb: clear, its strands faint
    sheet.Fill(3, kColumns, 0, 10, 60);
    sheet.Fill(3, kColumns, 10, 12, 255);
    sheet.Fill(4, kColumns, 0, 40, 0);              // a pillar: faint edges round a solid shape
    sheet.Fill(4, kColumns, 40, 44, 60);
    sheet.Fill(5, kColumns, 0, 64, 127);            // all just under the reference: clear
    const auto coverage = ClassifyTileSheet(sheet.alpha.data(), sheet.width, sheet.height);

    check(coverage.size() == 6, "Every icon of the sheet is classified");
    check(CoverageOf(coverage, 0) == TileCoverage::Opaque, "A fully opaque tile stays opaque");
    check(CoverageOf(coverage, 1) == TileCoverage::Opaque,
          "Under a quarter clear stays opaque (a wall's gaps read as mortar, never as holes)");
    check(CoverageOf(coverage, 2) == TileCoverage::Cutout, "A sprite on clear texels is cut out");
    check(CoverageOf(coverage, 3) == TileCoverage::Translucent,
          "A tile mostly of faint texels is blended (a cut would lose it)");
    check(CoverageOf(coverage, 4) == TileCoverage::Cutout, "Faint edges round a solid sprite are still a cutout");
    check(CoverageOf(coverage, 5) == TileCoverage::Translucent,
          "Alpha below the reference counts as clear, and faint");
    check(CoverageOf(coverage, -1) == TileCoverage::Opaque && CoverageOf(coverage, 6) == TileCoverage::Opaque &&
              CoverageOf({}, 2) == TileCoverage::Opaque,
          "An icon the table does not have is opaque");
    check(ClassifyTileSheet(nullptr, 10, 10).empty(), "No sheet, no table");
    {
        // The 2x sheet (Content/icons2x/object-m.png): every texel of the 1x sheet twice each way, gaps
        // included; classified with scale 2 it gives the 1x sheet's table.
        const int w2 = sheet.width * 2, h2 = sheet.height * 2;
        std::vector<std::uint8_t> alpha2(static_cast<std::size_t>(w2) * static_cast<std::size_t>(h2));
        for (int y = 0; y < h2; ++y)
        {
            for (int x = 0; x < w2; ++x)
            {
                alpha2[static_cast<std::size_t>(y) * static_cast<std::size_t>(w2) + static_cast<std::size_t>(x)] =
                    sheet.alpha[static_cast<std::size_t>(y / 2) * static_cast<std::size_t>(sheet.width) +
                                static_cast<std::size_t>(x / 2)];
            }
        }
        check(ClassifyTileSheet(alpha2.data(), w2, h2, 2) == coverage, "A 2x sheet classifies as its 1x sheet");
    }

    // The real sheet's layout: 1301 x 1431 texels hold 20 x 22 icons.
    const std::vector<std::uint8_t> real(1301u * 1431u, 255);
    check(ClassifyTileSheet(real.data(), 1301, 1431).size() == 440, "object-m.png's 1301 x 1431 hold 440 icons");

    // The clear texels of an opaque tile take the colour round them, solid; other tiles keep theirs.
    {
        constexpr int kW = 2 * 65 + 1, kH = 65 + 1;
        std::vector<std::uint8_t> rgba(static_cast<std::size_t>(kW) * kH * 4, 0);
        const auto texel = [&](int icon, int x, int y) { return &rgba[(static_cast<std::size_t>(1 + y) * kW + 1 + icon * 65 + x) * 4]; };
        for (int icon = 0; icon < 2; ++icon)
        {
            for (int y = 0; y < 64; ++y)
            {
                for (int x = 0; x < 64; ++x)
                {
                    std::uint8_t* t = texel(icon, x, y);
                    const bool hole = x < 8; // a clear strip at the left: black, alpha 0
                    t[0] = hole ? 0 : 40;
                    t[1] = hole ? 0 : 160;
                    t[2] = hole ? 0 : 40;
                    t[3] = hole ? 0 : 255;
                }
            }
        }
        const std::vector<TileCoverage> table{TileCoverage::Opaque, TileCoverage::Cutout};
        FillOpaqueTilesClearTexels(rgba.data(), kW, kH, table);
        const std::uint8_t* edge = texel(0, 0, 30);
        const std::uint8_t* cut = texel(1, 0, 30);
        check(edge[0] == 40 && edge[1] == 160 && edge[2] == 40 && edge[3] == 255,
              "An opaque tile's clear strip takes the green round it and becomes solid");
        check(cut[0] == 0 && cut[1] == 0 && cut[2] == 0, "A cutout tile's clear texels are left as they are");
    }

    // The cheese with holes is a cutout whatever its count (Q-010: see-through holes).
    {
        std::vector<TileCoverage> table(440, TileCoverage::Opaque);
        MarkSeeThroughTiles(table);
        check(table[339] == TileCoverage::Cutout && table[338] == TileCoverage::Opaque,
              "the cheese with holes 339 is see-through, its neighbours not");
    }

    // The brick edge pieces' clear texels take the whole brick's (Q-011: whole bricks elsewhere).
    {
        constexpr int kW = 1 + 20 * 65, kH = 1 + 15 * 65;
        std::vector<std::uint8_t> rgba(static_cast<std::size_t>(kW) * kH * 4, 0);
        const auto at = [&](int icon, int x, int y) {
            const int x0 = 1 + (icon % 20) * 65, y0 = 1 + (icon / 20) * 65;
            return rgba.data() + (static_cast<std::size_t>(y0 + y) * kW + (x0 + x)) * 4;
        };
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x)
            {
                std::uint8_t* whole = at(261, x, y);
                whole[0] = static_cast<std::uint8_t>(x); whole[1] = static_cast<std::uint8_t>(y); whole[2] = 7; whole[3] = 255;
                for (const int icon : {262, 263, 283, 264})
                {
                    std::uint8_t* t = at(icon, x, y);
                    const bool clear = x < 5;
                    t[0] = 200; t[1] = 200; t[2] = 200; t[3] = clear ? 0 : 255;
                }
            }
        GalaxyEggbert::Game::FillBrickEdgesFromWholeBrick(rgba.data(), kW, kH);
        bool copied = true, kept = true;
        for (const int icon : {262, 263, 283})
            copied = copied && at(icon, 3, 40)[0] == 3 && at(icon, 3, 40)[1] == 40 && at(icon, 3, 40)[3] == 255 &&
                     at(icon, 10, 40)[0] == 200;
        kept = at(264, 3, 40)[3] == 0;
        check(copied && kept, "the brick edge pieces' clear texels are the whole brick's at the same place, no other tile's");
    }

    // The brick edge pieces, ragged towards the open (Q-011, BrickEdges.hpp).
    {
        namespace B = GalaxyEggbert::Game::BrickEdges;
        using N = B::Neighbour;
        const auto around = [](N pz, N nz, N px, N nx) {
            return [=](int side) { return side == B::PositiveZ ? pz : side == B::NegativeZ ? nz : side == B::PositiveX ? px : nx; };
        };
        // A wall along X, a room on both sides of it (Z), ending at -X.
        const auto end262 = B::ShapeFor(262, around(N::Air, N::Air, N::Solid, N::Air));
        check(end262.ragged && end262.raggedSides == (1u << B::NegativeX) &&
                  end262.inset[0][B::NegativeX] == 5.0f / 64.0f && end262.inset[2][B::NegativeX] == 8.0f / 64.0f &&
                  end262.inset[1][B::NegativeX] == 0.0f && end262.inset[0][B::NegativeZ] == 0.0f,
              "262 at a wall's end (-X air, the wall going on at +X): its courses end short at -X, not at its faces");
        const auto face262 = B::ShapeFor(262, around(N::Solid, N::Air, N::Air, N::Air));
        check(face262.ragged && face262.raggedSides == (1u << B::NegativeZ),
              "262 of a wall along Z, ending at -Z: ragged at -Z only");
        // Scattered in a thick wall's face (the room at -Z, the wall on along X and behind): whole.
        check(!B::ShapeFor(262, around(N::Solid, N::Air, N::Solid, N::Solid)).ragged &&
                  !B::ShapeFor(263, around(N::Solid, N::Air, N::Solid, N::Solid)).ragged &&
                  !B::ShapeFor(283, around(N::Solid, N::Air, N::Solid, N::Solid)).ragged,
              "an edge piece in the middle of a wall's face stays whole (the worlds scatter them for variety)");
        // The outer corner of a thick wall's end: its end and its face both open.
        const auto corner = B::ShapeFor(262, around(N::Solid, N::Air, N::Solid, N::Air));
        check(corner.raggedSides == ((1u << B::NegativeZ) | (1u << B::NegativeX)),
              "262 at a thick wall's outer corner is ragged on both open sides");
        check(!B::ShapeFor(262, around(N::Air, N::Air, N::Air, N::Air)).ragged &&
                  !B::ShapeFor(262, around(N::Air, N::Air, N::Solid, N::Solid)).ragged &&
                  !B::ShapeFor(262, around(N::Air, N::Air, N::Outside, N::Air)).ragged,
              "262 stays whole with no wall behind its left, with no air on its left, or at the world's edge");
        const auto end263 = B::ShapeFor(263, around(N::Air, N::Air, N::Air, N::Solid));
        check(end263.raggedSides == (1u << B::PositiveX) && end263.inset[2][B::PositiveX] == 10.0f / 64.0f &&
                  !B::ShapeFor(263, around(N::Air, N::Air, N::Solid, N::Air)).ragged,
              "263 is ragged at +X where the wall ends there, never on its left");
        const auto column = B::ShapeFor(283, around(N::Air, N::Air, N::Air, N::Solid));
        check(column.raggedSides == ((1u << B::PositiveZ) | (1u << B::NegativeZ) | (1u << B::PositiveX)) &&
                  column.inset[0][B::NegativeZ] == 7.0f / 64.0f && column.inset[0][B::PositiveX] == 5.0f / 64.0f,
              "283, the column, is ragged on every side open to the air, its left widths at -X/-Z, right at +X/+Z");
        check(!B::ShapeFor(261, around(N::Air, N::Air, N::Air, N::Air)).ragged && B::IsBrickEdgeIcon(283) &&
                  !B::IsBrickEdgeIcon(261),
              "the whole brick 261 is never ragged");

        const auto rect = B::CourseRect(end262, 2);
        const auto rest = B::Subtract(B::Rect{}, rect);
        const auto none = B::Subtract(rect, B::Rect{});
        const auto disjoint = B::Subtract(B::Rect{0.0f, 0.0f, 0.2f, 1.0f}, B::Rect{0.5f, 0.0f, 1.0f, 1.0f});
        const auto frame = B::Subtract(B::Rect{}, B::Rect{0.25f, 0.25f, 0.75f, 0.75f});
        float area = 0.0f;
        for (const auto& r : frame) area += (r.x1 - r.x0) * (r.z1 - r.z0);
        check(rect.x0 == 8.0f / 64.0f && rect.x1 == 1.0f && rest.size() == 1 && rest[0].x1 == 8.0f / 64.0f &&
                  none.empty() && disjoint.size() == 1 && frame.size() == 4 && area > 0.7499f && area < 0.7501f,
              "a course's section, and what of a section another does not cover");

        std::vector<std::uint8_t> alpha(64 * 64, 255);
        for (int row = 0; row < 64; ++row)
            for (int x = 0; x < (row < 17 ? 5 : row < 33 ? 0 : row < 48 ? 8 : 2); ++x) alpha[row * 64 + x] = 0;
        alpha[0] = 0; alpha[1] = 0; alpha[2] = 0; // a stray clear texel in a row does not change the course's run
        alpha[17 * 64 + 0] = 0;
        const auto left = B::MeasureCourseInsets(alpha.data(), 64, true, GalaxyEggbert::Game::kCutoutReferenceAlpha);
        const auto right = B::MeasureCourseInsets(alpha.data(), 64, false, GalaxyEggbert::Game::kCutoutReferenceAlpha);
        check(left == B::kTiles[0].left && right == std::array<int, 4>{0, 0, 0, 0},
              "the courses' insets are each course's most common clear run from the edge");
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
