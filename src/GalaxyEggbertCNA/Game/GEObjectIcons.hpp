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
    // (tracked as DOC-007, not fixed there either): every type here is drawn
    // from element.png, but ObjectType1/12 actually need object-m.png and
    // ObjectType32/33 need blupi1.png in real mobile-eggbert data. Not fixed
    // in this first CNA rendering pass -- see NEXT.md.
    int GetObjIcon(ObjectType type, int phase);

    // element.png UV rect for a given icon: 600x1740 px, 60x60 px tiles, 10
    // columns, no gap/leading margin (confirmed by direct file inspection --
    // unlike object-m.png, element.png's dimensions divide evenly, see
    // mobile-eggbert-reference/03-objects.md's DOC-231 note).
    struct ObjectIconUv { float U0, V0, U1, V1; };
    ObjectIconUv GetElementIconUv(int icon);
}
