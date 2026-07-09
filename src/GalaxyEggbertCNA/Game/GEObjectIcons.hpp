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
    // Coverage (2026-07-09, NEXT.md §3): 66 confirmed ObjectTypes now have a
    // real icon (up from 65, 65 up from 31 earlier this session) -- the
    // latest addition is ObjectType38's electric arc (also blupi1.png-
    // sourced, see IsBlupiPngSourced/UsesBlupi1Texture below). Its real
    // behavior is two-channel (blupi1.png ticks 0-29, then element.png
    // ticks 30-89), and 03-objects.md flags the element.png-only
    // simplification as "under consideration" -- but that question turns
    // out to be moot today: GetObjIcon is always called with phase=0 (no
    // per-instance animation timers exist yet), and tick 0 is unambiguously
    // the blupi1.png channel, icon 266, per 03-objects.md's own
    // "...blupi1channel.png" crop label. The element.png channel (ticks
    // 30-89) has no representation yet and needs a real animation timer
    // plus true dual-texture billboard support first -- a residual gap for
    // later, not a decision blocker today. Genuinely no icon exists in
    // mobile-eggbert source data for ObjectType0/18/22/58 -- default:
    // return 0 is correct for those, not a gap.
    //
    // 53/92 (explo.png) and 98/99/100 (explo.png) return their documented
    // first-frame icon only (no cycling): 53's 45 frames and 92's 128 frames
    // would run off explo.png's 100-icon grid under a naive consecutive-icon
    // assumption (same reasoning as element.png's 56/57 and object-m.png's
    // 52 above); 99/100 additionally have real leading invisible (-1)
    // frames that a static first-real-frame icon can't represent without a
    // per-instance animation timer, which doesn't exist yet (GetObjIcon is
    // always called with phase=0 today, see GalaxyEggbertCnaGame.cpp).
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

    // explo.png UV rect for a given icon: 1440x1440 px, 144x144 px tiles, 10
    // columns x 10 rows (100 icons, 0-99), no gap (confirmed by direct file
    // inspection -- dimensions divide evenly, same as element.png; see
    // mobile-eggbert-reference/08-animations.md §4 for the per-tile size).
    ObjectIconUv GetExploIconUv(int icon);

    // True for the 12 confirmed Category B ObjectTypes sourced from
    // explo.png (explosions/visual effects: ObjectType8/9/10/11/53/90/91/
    // 92/93/98/99/100; mobile-eggbert-reference/03-objects.md, added
    // 2026-07-09). The icon GetObjIcon() returns for these types is an
    // explo.png index -- look it up via GetExploIconUv(), NOT
    // GetElementIconUv() or GETileAtlas::GetTileUv().
    bool IsExploPngSourced(ObjectType type);

    // blupi.png/blupi1.png UV rect for a given icon: 600x2040 px, 60x60 px
    // tiles, 10 columns x 34 rows (340 icons, 0-339), no gap (confirmed by
    // direct file inspection; both sheets share the identical layout --
    // mobile-eggbert-reference/03-objects.md line 228).
    ObjectIconUv GetBlupiIconUv(int icon);

    // True for the 4 confirmed Blupi-skin ObjectTypes (ObjectType200/201/
    // 202/203, mobile-eggbert-reference/03-objects.md, added 2026-07-09)
    // plus ObjectType38's electric arc (added same day -- its blupi1.png
    // channel is the only one rendered today, see GetObjIcon()'s header
    // comment). The icon GetObjIcon() returns for these types is a
    // blupi.png/blupi1.png index -- look it up via GetBlupiIconUv().
    bool IsBlupiPngSourced(ObjectType type);

    // True for the ObjectTypes that source blupi1.png instead of blupi.png:
    // the 3 of the 4 Blupi-skin types (ObjectType201/202/203 -- ObjectType200
    // uses blupi.png itself) plus ObjectType38's electric arc. Per
    // 03-objects.md, Blupi1_11/_12/_13 all read the identical blupi1.png
    // pixels in a raw crop -- any tint difference between 201/202/203 is
    // applied at render time in mobile-eggbert, not reproduced here (no
    // per-instance tinting exists in GalaxyEggbertCNA yet), so all 3 render
    // identically to each other today.
    bool UsesBlupi1Texture(ObjectType type);
}
