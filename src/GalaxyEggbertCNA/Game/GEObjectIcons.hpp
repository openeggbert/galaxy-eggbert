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
    // (tracked as DOC-007, not fixed there either): the icon numbers this
    // returns are correct for every type, but ObjectType32/33 need
    // blupi1.png, not element.png, in real mobile-eggbert data -- still not
    // fixed. ObjectType1/12/47/48 (see IsUniformCubeObject below) DO now
    // correctly source object-m.png instead of element.png (2026-07-09,
    // GalaxyEggbertCnaGame's separate cube-object render path) -- not fixed
    // via this function, since GetElementIconUv is still hardcoded to the
    // element.png sheet layout; the caller picks the right sheet/UV function
    // based on IsUniformCubeObject().
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
}
