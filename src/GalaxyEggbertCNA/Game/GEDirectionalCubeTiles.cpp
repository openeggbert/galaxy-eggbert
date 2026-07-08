#include "GEDirectionalCubeTiles.hpp"
#include "GESwatchUv.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        using Easy3D::CubeFace;

        // Every icon below answered "krychle, textura na (všech/4) bočních
        // stranách" -- all 4 side faces always show the tile's real texture;
        // only top/bottom vary, and only between "flat fallback color"
        // (TopColor/BottomColor = true, approximated via SwatchUv) and
        // "open" (false -- a real hole, like icon 200's). Source: direct
        // user Q&A in mobile-eggbert-reference/
        // questionnaire-all-remaining-tiles.md (round 2) and
        // questionnaire-unused-tiles.md (round 3), confirmed 2026-07-08 --
        // see those files for each icon's exact original wording.
        struct SymmetricEntry
        {
            int Icon;
            bool TopColor;    // false = open
            bool BottomColor; // false = open
        };

        constexpr SymmetricEntry kSymmetricEntries[] = {
            // shora + zdola both flat color
            {2, true, true}, {22, true, true}, {23, true, true}, {24, true, true},
            {50, true, true}, {52, true, true}, {86, true, true},
            {193, true, true}, {194, true, true}, {195, true, true}, {196, true, true}, {197, true, true},
            {224, true, true}, {225, true, true}, {226, true, true}, {227, true, true}, {228, true, true},
            {232, true, true}, {283, true, true},
            // shora flat color, zdola open
            {25, true, false}, {26, true, false}, {27, true, false}, {29, true, false},
            {44, true, false}, {45, true, false}, {46, true, false}, {47, true, false},
            {51, true, false}, {87, true, false}, {88, true, false}, {89, true, false}, {90, true, false},
            {154, true, false}, {155, true, false},
            {250, true, false}, {251, true, false}, {252, true, false}, {253, true, false},
            {254, true, false}, {255, true, false}, {256, true, false}, {257, true, false},
            {258, true, false}, {259, true, false}, {260, true, false},
            {364, true, false}, {365, true, false}, {366, true, false},
        };

        // The 4 ventilator/fan tiles: 4 side faces always textured; the
        // remaining top/bottom pair is one "základna větráku" (fan base,
        // flat fallback color) and one genuinely open face. Which of
        // top/bottom is the base isn't stated in the questionnaire text
        // itself -- resolved 2026-07-08 by the user looking at the actual
        // crops again: icon 132 (FanUp) shows a visible pedestal touching
        // the BOTTOM of the image, icon 135 (FanDown) shows a mount
        // hanging from the TOP (mirrored), and 126/129 (FanLeft/FanRight,
        // side-mounted on a wall bracket with no clear up/down cue) default
        // to the same "base = bottom" reading as 132.
        struct FanEntry
        {
            int Icon;
            bool BaseIsTop; // false = base is bottom
        };

        constexpr FanEntry kFanEntries[] = {
            {126, false}, {129, false}, {132, false}, {135, true},
        };

        // Icons needing exactly ONE of the 4 side faces textured (the rest
        // some mix of flat color / open), where mobile-eggbert has NO
        // per-placement rotation to read the facing from (confirmed
        // 2026-07-08: mobile-eggbert's Decor cell is just `{ icon; }`, no
        // orientation field -- see NEXT.md §3). Facing was instead
        // determined, icon by icon, from the actual crop image: an image
        // with a genuine asymmetric feature offset toward one edge uses that
        // edge (2D top/bottom -> CubeFace::PosZ/NegZ, 2D left/right ->
        // CubeFace::NegX/PosX -- same convention as the FanLeft/Right/Up/Down
        // icons already confirm mobile-eggbert uses). An image with NO
        // reliable directional cue (the large majority -- most of these are
        // small symmetric decorative icons, not oriented objects) defaults
        // to PosZ; a symmetric icon looks identical on any one face, so this
        // is a harmless tie-break, not a guess about unknown content.
        enum class Pattern
        {
            SingleFaceRestColor,             // 1 side tex; other 5 = flat color
            SingleFaceTopColorRestOpen,      // 1 side tex; top = color; other 4 = open
            SingleFaceTopBottomColorRestOpen,// 1 side tex; top+bottom = color; other 3 sides = open
            SingleFaceRestOpen,              // 1 side tex; other 5 = open (fully passable)
            AxisRestColor,                   // 2 opposite sides tex; other 4 = color
            AxisPlusTopBottomOtherSidesColor,// 2 opposite sides + top + bottom tex; other 2 sides = color
        };

        struct DirectionalEntry
        {
            int Icon;
            Pattern EntryPattern;
            CubeFace Primary; // single-face patterns: the textured face.
                              // Axis patterns: one face of the textured pair
                              // (its opposite is inferred).
        };

        constexpr CubeFace Opposite(CubeFace f)
        {
            switch (f)
            {
                case CubeFace::PosZ: return CubeFace::NegZ;
                case CubeFace::NegZ: return CubeFace::PosZ;
                case CubeFace::PosX: return CubeFace::NegX;
                case CubeFace::NegX: return CubeFace::PosX;
                case CubeFace::PosY: return CubeFace::NegY;
                default:             return CubeFace::PosY;
            }
        }

        constexpr DirectionalEntry kDirectionalEntries[] = {
            // Group A: single face + 5 sides flat color. Confident facing:
            // icon 392 has a light trim strip on the LEFT edge of the crop,
            // icon 393 the matching dark trim on the RIGHT edge (a real
            // matched pair from the "architectural-kit" stone set) -- every
            // other icon in this group is a small symmetric/abstract
            // decoration with no reliable edge cue, so defaults to PosZ.
            {3, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {4, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {5, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {6, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {8, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {9, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {10, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {11, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {12, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {13, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {14, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {48, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {186, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {187, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {188, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {189, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {190, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {191, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {192, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {390, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {391, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {392, Pattern::SingleFaceRestColor, CubeFace::NegX}, // light trim on the crop's left edge
            {393, Pattern::SingleFaceRestColor, CubeFace::PosX}, // dark trim on the crop's right edge
            {394, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {395, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {396, Pattern::SingleFaceRestColor, CubeFace::PosZ},
            {397, Pattern::SingleFaceRestColor, CubeFace::PosZ},

            // Group B: single face + top color + other 4 (3 sides + bottom)
            // open. Icons 74/75's own wording only states the textured side
            // + top color + bottom open, leaving the other 2 sides
            // unstated; treated as open here to match this fully-specified
            // pattern (the closest confirmed analog) rather than left
            // undefined -- flag for re-confirmation if that turns out wrong.
            {19, Pattern::SingleFaceTopColorRestOpen, CubeFace::PosZ},
            {20, Pattern::SingleFaceTopColorRestOpen, CubeFace::PosZ},
            {21, Pattern::SingleFaceTopColorRestOpen, CubeFace::PosZ},
            {28, Pattern::SingleFaceTopColorRestOpen, CubeFace::PosZ},
            {74, Pattern::SingleFaceTopColorRestOpen, CubeFace::PosZ},
            {75, Pattern::SingleFaceTopColorRestOpen, CubeFace::PosZ},

            // Group C: single face + top AND bottom color + other 3 sides open.
            {245, Pattern::SingleFaceTopBottomColorRestOpen, CubeFace::PosZ},

            // Group E: single face + all other 5 faces open (fully passable
            // except through the one textured face).
            {66, Pattern::SingleFaceRestOpen, CubeFace::PosZ},

            // Group G: 2 opposite side faces textured (an axis, not a single
            // face) + all other 4 faces (top/bottom + the other 2 sides)
            // flat color. Icon 400's crop is a symmetric archway with no
            // left/right vs. front/back cue -- defaults to the Z axis.
            {400, Pattern::AxisRestColor, CubeFace::PosZ},

            // Group H: 2 opposite sides + top + bottom all textured, other 2
            // sides flat color. Icon 49's crop shows a clear horizontal
            // axle/dumbbell shape spanning left-right -> X axis.
            {49, Pattern::AxisPlusTopBottomOtherSidesColor, CubeFace::PosX},

            // NOT YET ADDED: icons 15/16/17/18 ("2 protilehlé strany + shora
            // barva + zdola průhledné + z zbylých 2 bočních stran jedna
            // průhledná a druhá barva") need BOTH an axis choice AND which
            // of the 2 remaining perpendicular sides is open vs. colored --
            // their crops (diagonal wedge cuts) didn't give a confident read
            // for either part as of 2026-07-08. See NEXT.md §8.
        };
    }

    bool TryGetDirectionalCubeFaces(int icon, const Easy3D::UvRect& tileUv,
                                    Easy3D::DirectionalCubeFace (&outFaces)[6])
    {
        using Easy3D::CubeFace;

        if (icon == GalaxyEggbert::BlockTypes::Platform)
        {
            // Icon 200 ("Platform"): confirmed by round-1 Q&A
            // (questionnaire-unidentified-tiles.md) and re-confirmed in
            // 02-tiles.md's icon 200 entry -- a passable grate/grid, textured
            // on all 4 side faces, top and bottom genuinely open (a real
            // hole, not a fallback color -- see BlockTypes.hpp's Platform
            // comment).
            for (auto& face : outFaces)
            {
                face.Visible = true;
                face.Uv = tileUv;
            }
            outFaces[static_cast<int>(CubeFace::PosY)].Visible = false;
            outFaces[static_cast<int>(CubeFace::NegY)].Visible = false;
            return true;
        }

        for (const auto& entry : kSymmetricEntries)
        {
            if (entry.Icon != icon)
            {
                continue;
            }

            for (auto& face : outFaces)
            {
                face.Visible = true;
                face.Uv = tileUv;
            }

            auto& top = outFaces[static_cast<int>(CubeFace::PosY)];
            top.Visible = entry.TopColor;
            top.Uv = entry.TopColor ? SwatchUv(tileUv, kTopSwatchV) : top.Uv;

            auto& bottom = outFaces[static_cast<int>(CubeFace::NegY)];
            bottom.Visible = entry.BottomColor;
            bottom.Uv = entry.BottomColor ? SwatchUv(tileUv, kBottomSwatchV) : bottom.Uv;

            return true;
        }

        for (const auto& fan : kFanEntries)
        {
            if (fan.Icon != icon)
            {
                continue;
            }

            for (int i = 0; i < 4; ++i)
            {
                outFaces[i].Visible = true;
                outFaces[i].Uv = tileUv;
            }

            const auto baseFace = fan.BaseIsTop ? CubeFace::PosY : CubeFace::NegY;
            const auto openFace = fan.BaseIsTop ? CubeFace::NegY : CubeFace::PosY;
            outFaces[static_cast<int>(baseFace)].Visible = true;
            outFaces[static_cast<int>(baseFace)].Uv =
                SwatchUv(tileUv, fan.BaseIsTop ? kTopSwatchV : kBottomSwatchV);
            outFaces[static_cast<int>(openFace)].Visible = false;
            return true;
        }

        for (const auto& entry : kDirectionalEntries)
        {
            if (entry.Icon != icon)
            {
                continue;
            }

            // Start from "all open" and turn faces on as the pattern needs --
            // simpler than tracking 6 independent flags per pattern.
            for (auto& face : outFaces)
            {
                face.Visible = false;
                face.Uv = tileUv;
            }

            const auto setColor = [&](CubeFace f, float vFrac)
            {
                auto& face = outFaces[static_cast<int>(f)];
                face.Visible = true;
                face.Uv = SwatchUv(tileUv, vFrac);
            };
            const auto setTex = [&](CubeFace f)
            {
                auto& face = outFaces[static_cast<int>(f)];
                face.Visible = true;
                face.Uv = tileUv;
            };
            const auto vFracFor = [](CubeFace f)
            {
                if (f == CubeFace::PosY) return kTopSwatchV;
                if (f == CubeFace::NegY) return kBottomSwatchV;
                return kMidSwatchV;
            };

            switch (entry.EntryPattern)
            {
                case Pattern::SingleFaceRestColor:
                    setTex(entry.Primary);
                    for (int i = 0; i < 6; ++i)
                    {
                        const auto f = static_cast<CubeFace>(i);
                        if (f != entry.Primary)
                        {
                            setColor(f, vFracFor(f));
                        }
                    }
                    break;
                case Pattern::SingleFaceTopColorRestOpen:
                    setTex(entry.Primary);
                    setColor(CubeFace::PosY, kTopSwatchV);
                    break;
                case Pattern::SingleFaceTopBottomColorRestOpen:
                    setTex(entry.Primary);
                    setColor(CubeFace::PosY, kTopSwatchV);
                    setColor(CubeFace::NegY, kBottomSwatchV);
                    break;
                case Pattern::SingleFaceRestOpen:
                    setTex(entry.Primary);
                    break;
                case Pattern::AxisRestColor:
                    setTex(entry.Primary);
                    setTex(Opposite(entry.Primary));
                    for (int i = 0; i < 6; ++i)
                    {
                        const auto f = static_cast<CubeFace>(i);
                        if (f != entry.Primary && f != Opposite(entry.Primary))
                        {
                            setColor(f, vFracFor(f));
                        }
                    }
                    break;
                case Pattern::AxisPlusTopBottomOtherSidesColor:
                    setTex(entry.Primary);
                    setTex(Opposite(entry.Primary));
                    setTex(CubeFace::PosY);
                    setTex(CubeFace::NegY);
                    for (int i = 0; i < 6; ++i)
                    {
                        const auto f = static_cast<CubeFace>(i);
                        if (f != entry.Primary && f != Opposite(entry.Primary) &&
                            f != CubeFace::PosY && f != CubeFace::NegY)
                        {
                            setColor(f, kMidSwatchV);
                        }
                    }
                    break;
            }

            return true;
        }

        return false;
    }
}
