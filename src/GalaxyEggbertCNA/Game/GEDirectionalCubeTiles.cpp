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
            // icons 30/31: same face pattern as any other "4 sides + top/bottom
            // color" entry -- the confirmed answer's "textura má i průhlednost"
            // is about the SIDE faces' own texture having real per-pixel alpha,
            // not a different face layout. That alpha need is handled
            // separately in GETerrainRenderer.cpp (NeedsAlphaBlend()), which
            // routes these 2 icons to a dedicated semi-transparent draw pass
            // reusing the water render mode's blend state (2026-07-08).
            // Editor-only markers (moveable-object start position) per the
            // questionnaire -- never placed in real gameplay worlds, but
            // wired up for completeness/correctness.
            {30, true, true}, {31, true, true},
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
            // icon 107: shora open (false) here on PURPOSE, not a real hole --
            // its confirmed answer is "shora samostatná textura trávy"
            // (top = a SEPARATE grass texture), rendered by
            // GETerrainRenderer's dedicated grass-top pass
            // (IsGrassTopIcon()/m_grassRenderer, 2026-07-08, §8 task 3), not
            // by this table at all. Setting TopColor=false here just stops
            // this table from drawing a competing/z-fighting top face over
            // the grass plate. zdola (bottom) = hnědá plná barva, and
            // SwatchUv's bottom sample correctly picks up brown here since
            // icon 107's own texture is genuinely grass-on-top-of-dirt
            // (confirmed by looking at its crop).
            {107, false, true},
            // Teleporter pillars (330-333, plan.md E3D-MIG-147, revised
            // 2026-07-11 per direct user Q&A live in this session --
            // supersedes the older questionnaire's "Billboard" answer, see
            // 02-tiles.md's own updated note): 4 sides always show the
            // real texture (confirmed: "modré pozadí by měla být stále
            // renderována na stranu krychle" -- blue background on the
            // cube's side), top/bottom flat fallback color (a "solid
            // pillar" per every prior questionnaire pass, no open-face cue
            // anywhere). The lower ~2/3 of this same texture (the dark
            // cone/spike graphic) is ALSO used for a separate, genuinely
            // 3D pyramid-tip attachment hanging below the block --
            // GETerrainRenderer's own IsPyramidTipIcon()/PyramidTipUv(), an
            // extra AppendPyramidTipMesh call inlined into
            // AppendSpecialGeometry right after the DirectionalCube append,
            // not this table -- so the side faces showing that same cone
            // squished onto their own flat surface (from using the
            // whole-tile UV here, the same convention every other entry in
            // this table uses) is accepted as a minor first-pass cosmetic
            // overlap, not fixed with a custom sub-rect UV here.
            {GalaxyEggbert::BlockTypes::Teleport1, true, true},
            {GalaxyEggbert::BlockTypes::Teleport2, true, true},
            {GalaxyEggbert::BlockTypes::Teleport3, true, true},
            {GalaxyEggbert::BlockTypes::Teleport4, true, true},
        };

        // The 4 ventilator/fan tiles: 4 of the 6 faces always textured; the
        // remaining opposite pair is one "základna větráku" (fan base, flat
        // fallback color) and one genuinely open face (where the air exits).
        // The axis of that pair follows the fan's real blow direction, not
        // always top/bottom: FanUp (132) blows up (base at bottom, open at
        // top) and FanDown (135) blows down (base at top, open at bottom),
        // both confirmed 2026-07-08 from the actual crops (pedestal
        // touching the bottom of 132's image; mount hanging from the top of
        // 135's, mirrored). FanLeft (126)/FanRight (129) blow horizontally,
        // so their base/open pair is the X axis, not Y (axis fixed
        // 2026-07-09; NegX/PosX = left/right matches the same left/right
        // convention documented below for kDirectionalEntries). Which of
        // NegX/PosX is base vs. open was reported backwards live after that
        // first fix and corrected here the same day: base (blue) is now the
        // face AWAY from the blow direction, open (transparent) is the face
        // the fan blows toward. Putting a real texture on PosY/NegY for the
        // first time (previously those 2 faces were only ever used for
        // SwatchUv flat colors, never a directional texture) also exposed
        // that face's UV corner order doesn't read upright there -- reported
        // live as the top face needing a 180 degree turn; RotateTop180
        // below flips PosY's UV rect (U0<->U1, V0<->V1: a true 180 degree
        // turn of the sampled texture, not a mirror) for the 2 fans where
        // PosY is one of the always-textured faces. The base face's
        // flat-color swatch uses kTopSwatchV/kBottomSwatchV when the base is
        // Y-aligned (matching its "shora"/"zdola" wording) and kMidSwatchV
        // for the X-aligned 126/129 base, which has no such wording to
        // anchor to -- confirmed by direct pixel sampling of object-m.png to
        // be fully opaque at that sample point either way, ruling out a
        // swatch-alpha cause for any transparency complaint.
        struct FanEntry
        {
            int Icon;
            CubeFace BaseFace;
            CubeFace OpenFace;
            float BaseSwatchV;
            bool RotateTop180 = false;
        };

        constexpr FanEntry kFanEntries[] = {
            {126, CubeFace::NegX, CubeFace::PosX, kMidSwatchV, true},  // FanLeft: blows toward -X
            {129, CubeFace::PosX, CubeFace::NegX, kMidSwatchV, true},  // FanRight: blows toward +X
            {132, CubeFace::NegY, CubeFace::PosY, kBottomSwatchV, false}, // FanUp: blows up
            {135, CubeFace::PosY, CubeFace::NegY, kTopSwatchV, false},    // FanDown: blows down
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
            AxisTopColorBottomOpenSideColorSideOpen, // 2 opposite sides tex (axis); top = color;
                                              // bottom = open; of the other axis' 2 sides, one =
                                              // color, one = open (icons 15-18).
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

            // Group I: icons 15/16/17/18 ("2 protilehlé strany + shora barva
            // + zdola průhledné + z zbylých 2 bočních stran jedna průhledná
            // a druhá barva"). Their crops (diagonal wedge cuts with a
            // small machine-piece detail) didn't give a confident axis or
            // side-asymmetry read even after direct user review on
            // 2026-07-09 (confirmed only that icons 15/17 are a horizontal
            // mirror pair of each other, which doesn't resolve either
            // question for a SINGLE block's own 6 faces) -- defaults to the
            // Z axis (matching this table's other axis defaults, e.g. icon
            // 400) with NegX = open, PosX = color, per the user's explicit
            // go-ahead to use the same tie-break approach as every other
            // ambiguous icon in this table.
            {15, Pattern::AxisTopColorBottomOpenSideColorSideOpen, CubeFace::PosZ},
            {16, Pattern::AxisTopColorBottomOpenSideColorSideOpen, CubeFace::PosZ},
            {17, Pattern::AxisTopColorBottomOpenSideColorSideOpen, CubeFace::PosZ},
            {18, Pattern::AxisTopColorBottomOpenSideColorSideOpen, CubeFace::PosZ},
        };
    }

    bool TryGetDirectionalCubeFaces(int icon, const Easy3D::UvRect& tileUv,
                                    const Easy3D::UvRect& icon107Uv,
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

            for (int i = 0; i < 6; ++i)
            {
                const auto face = static_cast<CubeFace>(i);
                if (face == fan.BaseFace || face == fan.OpenFace)
                {
                    continue;
                }
                outFaces[i].Visible = true;
                outFaces[i].Uv = tileUv;
            }

            outFaces[static_cast<int>(fan.BaseFace)].Visible = true;
            outFaces[static_cast<int>(fan.BaseFace)].Uv = SwatchUv(tileUv, fan.BaseSwatchV);
            outFaces[static_cast<int>(fan.OpenFace)].Visible = false;

            if (fan.RotateTop180)
            {
                auto& top = outFaces[static_cast<int>(CubeFace::PosY)];
                top.Uv = Easy3D::UvRect{top.Uv.U1, top.Uv.V1, top.Uv.U0, top.Uv.V0};
            }
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
                case Pattern::AxisTopColorBottomOpenSideColorSideOpen:
                {
                    setTex(entry.Primary);
                    setTex(Opposite(entry.Primary));
                    setColor(CubeFace::PosY, kTopSwatchV);
                    // Bottom (NegY) stays open -- already false from the
                    // "start all open" init above.
                    const bool primaryIsZAxis =
                        (entry.Primary == CubeFace::PosZ || entry.Primary == CubeFace::NegZ);
                    const CubeFace colorSide = primaryIsZAxis ? CubeFace::PosX : CubeFace::PosZ;
                    // The other axis' remaining face (NegX if primary is Z,
                    // NegZ if primary is X) stays open -- already false.
                    setColor(colorSide, kMidSwatchV);
                    break;
                }
            }

            return true;
        }

        // Icons 108/109: "krychle, 2 boční strany vlastní textura, 1 boční
        // strana textura ikony 107, 1 boční strana průhledná, zdola hnědá
        // plná barva, shora samostatná textura trávy". Top is intentionally
        // left OPEN here (Visible=false), same reasoning as icon 107 itself
        // -- the real top surface is GETerrainRenderer's separate grass-top
        // overlay plate (IsGrassTopIcon()/m_grassRenderer), not this table.
        // Facing (which 2 adjacent sides get the icon's own texture, which 1
        // gets icon 107's, which 1 is open) has no confirmed cue in the
        // questionnaire text and the crops didn't resolve it even after
        // direct user review on mobile (2026-07-09) -- defaults to a fixed,
        // consistent assignment (PosZ+NegZ = own texture, PosX = icon 107's
        // texture, NegX = open), the same tie-break approach as every other
        // ambiguous icon in this table, per the user's explicit go-ahead.
        // Handled outside kDirectionalEntries/Pattern since it's the only
        // entry needing a SECOND icon's texture (icon107Uv) -- not worth a
        // new Pattern case for 2 icons.
        if (icon == 108 || icon == 109)
        {
            for (auto& face : outFaces)
            {
                face.Visible = false;
                face.Uv = tileUv;
            }

            outFaces[static_cast<int>(CubeFace::PosZ)].Visible = true;
            outFaces[static_cast<int>(CubeFace::NegZ)].Visible = true;

            auto& icon107Face = outFaces[static_cast<int>(CubeFace::PosX)];
            icon107Face.Visible = true;
            icon107Face.Uv = icon107Uv;

            // NegX stays open (already false from the loop above).

            auto& bottom = outFaces[static_cast<int>(CubeFace::NegY)];
            bottom.Visible = true;
            bottom.Uv = SwatchUv(tileUv, kBottomSwatchV);

            // Top (PosY) stays open -- the real grass top is drawn
            // separately, see this block's comment above.
            return true;
        }

        return false;
    }
}
