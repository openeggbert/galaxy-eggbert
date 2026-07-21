#pragma once
#include <cstddef>

// Explicit, queryable set of terrain icon IDs whose recommended 3D render
// mode (mobile-eggbert-reference/15-3d-render-mapping-design.md §10.2-§10.5,
// applied per-icon in mobile-eggbert-reference/02-tiles.md's Category column)
// is still only a first-pass agent visual guess, not yet confirmed by direct
// user identification the way 15-3d-render-mapping-design.md §11's 34 icons
// were. INFRA-004 (plan.md §7): this table exists so these ~131 icons can't
// be silently treated as confirmed by a future pass, and so any render-mode
// change touching one of them gets flagged for the same direct-identification
// treatment §11 already gave the other 34, gated by the golden-frame harness
// (INFRA-001/INFRA-002) rather than assumed correct from the first-pass guess.
//
// All 441 icon IDs not listed here are unaffected -- they are either
// confirmed UniformCube (the default, per §4), or already resolved to a
// specific mode by direct user identification (§11: DirectionalCube,
// thin-bar, or the 4 icons corrected to plain Billboard/UniformCube).

namespace GalaxyEggbert
{
namespace UnverifiedRenderMapping
{

enum class RenderModeGuess
{
    Billboard,
    ThinMechanical,
    SpecialSurface,
    ArchitecturalKit,
};

struct Entry
{
    int iconId;
    RenderModeGuess renderMode;
};

// Generated from mobile-eggbert-reference/02-tiles.md's "(§10.N)"-cited rows
// (2026-07-21) -- every row carrying a Billboard/ThinMechanical/
// special-surface/architectural-kit note that is NOT also tagged
// "(user 2026-07-07)" or "CORRECTED" (those are §11-resolved, direct
// identification, not first-pass guesses).
constexpr Entry kEntries[] = {
    {30, RenderModeGuess::Billboard},
    {31, RenderModeGuess::Billboard},
    {48, RenderModeGuess::Billboard},
    {66, RenderModeGuess::ThinMechanical},
    {76, RenderModeGuess::Billboard},
    {77, RenderModeGuess::Billboard},
    {86, RenderModeGuess::ThinMechanical},
    {87, RenderModeGuess::ThinMechanical},
    {88, RenderModeGuess::ThinMechanical},
    {89, RenderModeGuess::ThinMechanical},
    {90, RenderModeGuess::ThinMechanical},
    {91, RenderModeGuess::SpecialSurface},
    {92, RenderModeGuess::SpecialSurface},
    {96, RenderModeGuess::SpecialSurface},
    {110, RenderModeGuess::ThinMechanical},
    {114, RenderModeGuess::ThinMechanical},
    {118, RenderModeGuess::ThinMechanical},
    {122, RenderModeGuess::ThinMechanical},
    {126, RenderModeGuess::ThinMechanical},
    {129, RenderModeGuess::ThinMechanical},
    {132, RenderModeGuess::ThinMechanical},
    {135, RenderModeGuess::ThinMechanical},
    {138, RenderModeGuess::ThinMechanical},
    {158, RenderModeGuess::Billboard},
    {159, RenderModeGuess::Billboard},
    {160, RenderModeGuess::Billboard},
    {161, RenderModeGuess::Billboard},
    {162, RenderModeGuess::Billboard},
    {163, RenderModeGuess::Billboard},
    {164, RenderModeGuess::Billboard},
    {165, RenderModeGuess::Billboard},
    {174, RenderModeGuess::Billboard},
    {175, RenderModeGuess::Billboard},
    {176, RenderModeGuess::Billboard},
    {177, RenderModeGuess::Billboard},
    {178, RenderModeGuess::Billboard},
    {179, RenderModeGuess::Billboard},
    {180, RenderModeGuess::Billboard},
    {181, RenderModeGuess::Billboard},
    {182, RenderModeGuess::Billboard},
    {187, RenderModeGuess::ThinMechanical},
    {188, RenderModeGuess::ThinMechanical},
    {189, RenderModeGuess::ThinMechanical},
    {191, RenderModeGuess::Billboard},
    {192, RenderModeGuess::ThinMechanical},
    {199, RenderModeGuess::ThinMechanical},
    {201, RenderModeGuess::ThinMechanical},
    {203, RenderModeGuess::SpecialSurface},
    {211, RenderModeGuess::ThinMechanical},
    {214, RenderModeGuess::Billboard},
    {215, RenderModeGuess::Billboard},
    {216, RenderModeGuess::Billboard},
    {217, RenderModeGuess::Billboard},
    {218, RenderModeGuess::Billboard},
    {219, RenderModeGuess::Billboard},
    {220, RenderModeGuess::Billboard},
    {221, RenderModeGuess::Billboard},
    {222, RenderModeGuess::Billboard},
    {230, RenderModeGuess::Billboard},
    {231, RenderModeGuess::Billboard},
    {233, RenderModeGuess::Billboard},
    {234, RenderModeGuess::Billboard},
    {235, RenderModeGuess::Billboard},
    {236, RenderModeGuess::Billboard},
    {245, RenderModeGuess::Billboard},
    {250, RenderModeGuess::ThinMechanical},
    {251, RenderModeGuess::ThinMechanical},
    {252, RenderModeGuess::ThinMechanical},
    {253, RenderModeGuess::ThinMechanical},
    {254, RenderModeGuess::ThinMechanical},
    {255, RenderModeGuess::ThinMechanical},
    {256, RenderModeGuess::ThinMechanical},
    {257, RenderModeGuess::ThinMechanical},
    {258, RenderModeGuess::ThinMechanical},
    {259, RenderModeGuess::ThinMechanical},
    {260, RenderModeGuess::ThinMechanical},
    {264, RenderModeGuess::ThinMechanical},
    {265, RenderModeGuess::ThinMechanical},
    {266, RenderModeGuess::ThinMechanical},
    {267, RenderModeGuess::ThinMechanical},
    {268, RenderModeGuess::ThinMechanical},
    {269, RenderModeGuess::ThinMechanical},
    {270, RenderModeGuess::ThinMechanical},
    {271, RenderModeGuess::ThinMechanical},
    {272, RenderModeGuess::ThinMechanical},
    {273, RenderModeGuess::ThinMechanical},
    {304, RenderModeGuess::Billboard},
    {305, RenderModeGuess::ThinMechanical},
    {309, RenderModeGuess::Billboard},
    {317, RenderModeGuess::Billboard},
    {324, RenderModeGuess::ThinMechanical},
    {334, RenderModeGuess::Billboard},
    {335, RenderModeGuess::Billboard},
    {336, RenderModeGuess::Billboard},
    {364, RenderModeGuess::ThinMechanical},
    {373, RenderModeGuess::ThinMechanical},
    {375, RenderModeGuess::Billboard},
    {376, RenderModeGuess::Billboard},
    {377, RenderModeGuess::Billboard},
    {384, RenderModeGuess::ThinMechanical},
    {385, RenderModeGuess::ThinMechanical},
    {391, RenderModeGuess::ArchitecturalKit},
    {392, RenderModeGuess::ArchitecturalKit},
    {393, RenderModeGuess::ArchitecturalKit},
    {394, RenderModeGuess::ArchitecturalKit},
    {395, RenderModeGuess::ArchitecturalKit},
    {397, RenderModeGuess::ArchitecturalKit},
    {398, RenderModeGuess::Billboard},
    {399, RenderModeGuess::Billboard},
    {400, RenderModeGuess::ArchitecturalKit},
    {404, RenderModeGuess::Billboard},
    {410, RenderModeGuess::Billboard},
    {411, RenderModeGuess::Billboard},
    {412, RenderModeGuess::Billboard},
    {413, RenderModeGuess::Billboard},
    {421, RenderModeGuess::Billboard},
    {422, RenderModeGuess::Billboard},
    {423, RenderModeGuess::Billboard},
    {424, RenderModeGuess::Billboard},
    {425, RenderModeGuess::Billboard},
    {426, RenderModeGuess::Billboard},
    {427, RenderModeGuess::Billboard},
    {428, RenderModeGuess::Billboard},
    {429, RenderModeGuess::Billboard},
    {430, RenderModeGuess::Billboard},
    {431, RenderModeGuess::Billboard},
    {432, RenderModeGuess::Billboard},
    {433, RenderModeGuess::Billboard},
    {434, RenderModeGuess::Billboard},
    {435, RenderModeGuess::Billboard},
    {437, RenderModeGuess::Billboard},
};

constexpr std::size_t kEntryCount = sizeof(kEntries) / sizeof(kEntries[0]);

constexpr bool IsUnverified(int iconId) noexcept
{
    for (std::size_t i = 0; i < kEntryCount; ++i)
    {
        if (kEntries[i].iconId == iconId)
        {
            return true;
        }
    }
    return false;
}

} // namespace UnverifiedRenderMapping
} // namespace GalaxyEggbert
