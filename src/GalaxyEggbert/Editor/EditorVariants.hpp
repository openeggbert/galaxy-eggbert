#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <random>
#include <vector>

namespace GalaxyEggbert::Editor
{
    // The original editor's variant families (ED-0001, plans/audit/editor_parity.md). Each palette
    // entry of the Speedy Blupi editors places one icon of a table, remembered per family, and a
    // click on a cell that already holds the family's current variant advances it to the next
    // (CDecor::PlaceDecor with bMix, blupi_recon games/sb2/src/decdesign.cpp:82-97; the same rule
    // in Speedy Blupi 1, port/games/sb1/game/editor_wide.cpp). Ctrl places without cycling: a
    // random variant for the families marked random, the current one for the others.
    //
    // Galaxy's 3D reading (MR-0003, owner 2026-11-01): a click on the block placed by the previous
    // click cycles it; any other click places the family's current variant in front of the face
    // aimed at, as before.
    //
    // The tables are the decompiled Speedy Blupi 2's (games/sb2/src/decblock.cpp,
    // design_tables.h; names in the comments): the decor cells' families, and the BigDecor
    // families of the first menu (trees, palms, houses, ...). Ported under the owner's standing
    // approval for small fidelity tables (2026-10-25).
    struct VariantFamily
    {
        const char* source;
        bool random;
        std::vector<std::uint16_t> icons;
    };

    [[nodiscard]] inline const std::vector<VariantFamily>& VariantFamilies()
    {
        static const std::vector<VariantFamily> families = {
            {"sb2_design_37668 (support)", false, {76, 77, 199, 198, 200, 201, 235, 236, 375, 376, 377}},
            {"sb2_design_37698 (spider's web)", false, {403, 401, 402}},
            {"table_364a0 (square tech-blocks)", true, {10, 2, 3, 4, 5, 6, 7, 8, 9, 1, 11, 12, 13, 14}},
            {"table_364b0 (special tech-blocks)", true, {19, 20, 21, 23, 24, 25, 26, 27, 28, 29}},
            {"table_364d0 (triangular tech-blocks)", false, {15, 16, 17, 18}},
            {"table_364e8 (secret tech-block)", false, {73, 22}},
            {"table_364f8 (mechanical square)", true, {78, 79, 80, 81, 82, 83, 84}},
            {"table_36510 (special mechanical)", false, {87, 88, 89, 90}},
            {"table_36520 (secret mechanical)", false, {85, 86}},
            {"table_36458", false, {154, 155}},
            {"table_design_marine", false, {91, 92}},
            {"sb2_design_374f0", false, {396, 388, 389, 390, 391}},
            {"sb2_design_37508", false, {398, 399, 400}},
            {"sb2_design_375e8", true, {186, 187, 188, 189, 190, 191, 192}},
            {"table_365d0", true, {193, 194, 195, 196, 197}},
            {"sb2_design_375d0", true, {139, 140, 141, 142, 143}},
            {"table_36538", false, {41, 42, 43, 44, 45, 46, 47}},
            {"table_36550", true, {215, 216, 217, 218, 219, 220, 221, 222, 230, 231, 233, 234}},
            {"table_36570", true, {223, 224, 225, 226, 227, 228, 229, 232}},
            {"table_design_fan", false, {126, 129, 132, 135}},
            {"table_design_wind", false, {110, 114, 118, 122}},
            {"sb2_design_37830 (saw)", false, {378, 384}},
            {"sb2_design_377c8 (doors)", false, {334, 335, 336}},
            {"sb2_design_377d8 (treasure doors)", false,
             {421, 422, 423, 424, 425, 426, 427, 428, 429, 430, 431, 432, 433, 434, 435, 436, 437, 438, 439, 440}},
            {"sb2_design_37a40 (teleporters)", false, {330, 331, 332, 333}},
            {"table_design_key", false, {182, 183}},
            {"sb2_design_37758 (signs)", false,
             {158, 159, 160, 161, 162, 163, 164, 165, 309, 411, 412, 413, 414, 415, 174, 175, 176, 177, 178, 179,
              180, 181, 184}},
        };
        return families;
    }

    // The BigDecor families of the first menu (PlaceDecor1 entries 0-8, all random), the same
    // decompiled tables (ED-0006).
    [[nodiscard]] inline const std::vector<VariantFamily>& BigDecorVariantFamilies()
    {
        static const std::vector<VariantFamily> families = {
            {"table_366b0 (tree)", true, {20, 21, 22}},
            {"table_366c0 (palm tree)", true, {16, 17, 18, 19}},
            {"table_366d8 (decorative plant)", true, {23, 24, 25}},
            {"table_366e8 (house)", true, {26, 27}},
            {"table_366f8 (mechanical background)", true, {28, 29, 30, 31}},
            {"table_36710 (kid's stuff background)", true, {45, 46, 47, 36, 37, 38, 39, 40, 41, 42, 43, 44}},
            {"sb2_design_37740 (green slime)", true, {66, 67, 68, 69}},
            {"sb2_design_37518 (palace element)", true, {87, 88, 89}},
        };
        return families;
    }

    // The family of `families` whose table holds `icon`, or -1 (a single icon: nothing to cycle).
    [[nodiscard]] inline int FindVariantFamily(std::uint16_t icon,
                                               const std::vector<VariantFamily>& families = VariantFamilies())
    {
        for (std::size_t f = 0; f < families.size(); ++f)
        {
            for (const auto variant : families[f].icons)
            {
                if (variant == icon)
                {
                    return static_cast<int>(f);
                }
            }
        }
        return -1;
    }

    // The editor's state for the rule above: each family's remembered variant (m_lastDecorIndexes,
    // for the editing session) and the cell the previous click placed into.
    class VariantCycler
    {
    public:
        explicit VariantCycler(const std::vector<VariantFamily>& families = VariantFamilies()) : families_(&families)
        {
        }

        // The icon a click places for the palette's `selected` icon: its family's current variant
        // (the entry's own icon until it has been cycled); with Ctrl a random one of a random
        // family.
        [[nodiscard]] std::uint16_t IconToPlace(std::uint16_t selected, bool ctrl)
        {
            const int family = FindVariantFamily(selected, *families_);
            if (family < 0)
            {
                return selected;
            }
            const auto& icons = (*families_)[static_cast<std::size_t>(family)].icons;
            if (ctrl && (*families_)[static_cast<std::size_t>(family)].random)
            {
                return icons[std::uniform_int_distribution<std::size_t>(0, icons.size() - 1)(rng_)];
            }
            return icons[CurrentIndex(family, selected)];
        }

        // A click on (x, y, z) holding `iconThere` with `selected` chosen: when the previous click
        // placed that cell and it still holds the family's current variant, the family advances
        // (wrapping) and the new icon for the cell is returned.
        [[nodiscard]] std::optional<std::uint16_t> Cycle(std::uint16_t x, std::uint16_t y, std::uint16_t z,
                                                         std::uint16_t iconThere, std::uint16_t selected)
        {
            if (!lastPlaced_ || (*lastPlaced_)[0] != x || (*lastPlaced_)[1] != y || (*lastPlaced_)[2] != z)
            {
                return std::nullopt;
            }
            const int family = FindVariantFamily(selected, *families_);
            if (family < 0)
            {
                return std::nullopt;
            }
            const auto& icons = (*families_)[static_cast<std::size_t>(family)].icons;
            const std::size_t current = CurrentIndex(family, selected);
            if (icons.size() < 2 || icons[current] != iconThere)
            {
                return std::nullopt;
            }
            const std::size_t next = (current + 1) % icons.size();
            index_[static_cast<std::size_t>(family)] = static_cast<int>(next);
            return icons[next];
        }

        // The previous click placed into (x, y, z) (or cycled it).
        void NotePlaced(std::uint16_t x, std::uint16_t y, std::uint16_t z) { lastPlaced_ = {{x, y, z}}; }
        // Anything else that edits the world ends the chance to cycle.
        void Forget() noexcept { lastPlaced_.reset(); }

    private:
        std::size_t CurrentIndex(int family, std::uint16_t selected)
        {
            if (index_.size() != families_->size())
            {
                index_.assign(families_->size(), -1);
            }
            int& index = index_[static_cast<std::size_t>(family)];
            if (index < 0)
            {
                // Until cycled, the palette entry's own icon.
                const auto& icons = (*families_)[static_cast<std::size_t>(family)].icons;
                for (std::size_t i = 0; i < icons.size(); ++i)
                {
                    if (icons[i] == selected)
                    {
                        index = static_cast<int>(i);
                    }
                }
            }
            return static_cast<std::size_t>(index);
        }

        const std::vector<VariantFamily>* families_;
        std::vector<int> index_;
        std::optional<std::array<std::uint16_t, 3>> lastPlaced_;
        std::mt19937 rng_{20261101u};
    };
}
