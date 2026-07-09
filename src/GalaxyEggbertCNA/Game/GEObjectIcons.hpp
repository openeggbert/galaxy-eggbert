#pragma once

#include <GalaxyEggbert/def/ObjectType.hpp>

namespace GalaxyEggbert::CNA
{
    // element.png icon index for a MoveObject's ObjectType + animation phase.
    // Ported from GalaxyEggbertSimple3D's already-approved
    // GEDecorSystem::GetObjIcon() (galaxy-eggbert's own code, not a fresh
    // mobile-eggbert transcription) -- same tables, same simplifications
    // (continuous phase-indexed cycles instead of mobile-eggbert's real
    // 4-state turn/walk step machine; only "left"-facing frames for patrol
    // enemies, no separate mirrored table).
    //
    // Known limitation, inherited from the Simple3D reference target
    // (tracked as DOC-007, not fixed there either): ObjectType32/33 need
    // blupi1.png, not element.png, in real mobile-eggbert data -- still not
    // fixed. ObjectType1/12/47/48 (see IsUniformCubeObject below) and
    // ObjectType14/15/31/35/52 (see IsObjectMPngSourced below) DO now
    // correctly source object-m.png instead of element.png -- not fixed via
    // this function itself, since GetElementIconUv is still hardcoded to
    // the element.png sheet layout; the caller picks the right sheet/UV
    // function based on those two predicates.
    //
    // Coverage (2026-07-09, NEXT.md §3): 49 of 69 confirmed ObjectTypes now
    // have a real icon (up from 31) -- the 14 Category B types added this
    // pass, plus the pre-existing 31 and the 4 already-cube-routed types.
    // Still default: return 0 (wrong/placeholder icon): 12 explo.png-sourced
    // types (8/9/10/11/53/90/91/92/93/98/99/100), 4 blupi.png/blupi1.png
    // Blupi-skin types (200/201/202/203), and ObjectType38's two-channel
    // electric arc -- all deliberately deferred, see NEXT.md §8. Genuinely
    // no icon exists in mobile-eggbert source data for ObjectType0/18/22/58
    // -- default: return 0 is correct for those, not a gap.
    int GetObjIcon(ObjectType type, int phase);

    // element.png UV rect for a given icon: 600x1740 px, 60x60 px tiles, 10
    // columns, no gap/leading margin (confirmed by direct file inspection --
    // unlike object-m.png, element.png's dimensions divide evenly, see
    // mobile-eggbert-reference/03-objects.md's DOC-231 note).
    struct ObjectIconUv { float U0, V0, U1, V1; };
    ObjectIconUv GetElementIconUv(int icon);

    // True for the two confirmed exceptions that render as a solid
    // UniformCube instead of a billboard (mobile-eggbert-reference/
    // 15-3d-render-mapping-design.md §5): platform lifts (ObjectType1/47/48
    // -- Blupi physically stands and rides on top, a flat billboard would
    // look wrong for something load-bearing) and crates (ObjectType12 -- a
    // pushable box is naturally a cube in every direction). Both are
    // confirmed sourced from object-m.png (channel=1, PixmapChannel::Object,
    // see 03-objects.md), the same sheet as terrain, not element.png.
    bool IsUniformCubeObject(ObjectType type);

    // True for the 5 confirmed Category B ObjectTypes that are billboards
    // (unlike IsUniformCubeObject's cubes) but sourced from object-m.png
    // instead of element.png (ObjectType14/15/31/35/52 -- water splash/
    // bubble, charge power-up, bridge construction; mobile-eggbert-
    // reference/03-objects.md, added 2026-07-09). The icon GetObjIcon()
    // returns for these types is an object-m.png index -- look it up via
    // GETileAtlas::GetTileUv(), NOT GetElementIconUv().
    bool IsObjectMPngSourced(ObjectType type);
}
