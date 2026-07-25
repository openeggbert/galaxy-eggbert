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
    // latest addition is ObjectType38's electric arc, sourced from
    // mobile-eggbert's real `table_electro[90]` (Tables.cpp, transcribed
    // with explicit user approval), its 90-tick one-shot cycle simplified
    // to a continuous `% 90` loop like every other multi-frame type here.
    //
    // Per-instance animation timers now exist (2026-07-09,
    // MobileObjSpec::phase / GEWorldRuntime::Update(), 20fps reference
    // tick rate matching mobile-eggbert's own Config::ScaleTime(1) base) --
    // GetObjIcon() is no longer always called with phase=0, so every
    // phase-indexed formula in this function actually animates now, not
    // just ObjectType38. ObjectType38 is also the one type whose CHANNEL
    // (which texture sheet) depends on phase, not just its icon within a
    // fixed sheet: blupi1.png for ticks 0-29 of its cycle, element.png for
    // 30-89 (Decor.cpp confirms the switch). GetObjIcon() itself always
    // returns the correct icon for whichever channel is active --
    // IsBlupiPngSourcedAtPhase() below is what a renderer must additionally
    // check to pick the right texture/UV function per instance, per frame.
    // Genuinely no icon exists in mobile-eggbert source data for
    // ObjectType0/18/22/58 -- default: return 0 is correct for those, not a
    // gap.
    //
    // 53/92 (explo.png) still return their documented first-frame icon
    // only, now genuinely for the original reason (not just a phase=0
    // artifact): their 45/128 documented frames would run off explo.png's
    // 100-icon grid under a naive consecutive-icon assumption, same as
    // element.png's 56/57 and object-m.png's 52. 99/100 (explo.png) also
    // stay static: their real leading invisible (-1) frames still can't be
    // represented by a single per-tick icon return without a richer
    // "sometimes render nothing" mechanism, which doesn't exist yet even
    // though phase itself now advances.
    int GetObjIcon(ObjectType type, int phase);

    // ObjectType4 (bulldozer) real per-direction/per-turn-transition icon
    // (ENEMY-013, added 2026-07-20) -- unlike GetObjIcon()'s own ObjectType4
    // case above (a continuous phase-indexed cycle, only ever "left"-facing
    // frames, kept as the fallback for callers with no patrol-state
    // context), this ports the real `Decor.cpp:8628-8668` 4-state turn/walk
    // step machine exactly. @p patrolGoesLeftFromStart is a static per-
    // object property (real posStart.X > posEnd.X); @p patrolStep matches
    // MobileObjSpec::patrolStep (1=dwell@start, 2=advance, 3=dwell@end,
    // 4=recede); @p patrolTimeTicks matches MobileObjSpec::patrolTime cast
    // to ticks (real `m_moveObject[i].time`, resets to 0 each step
    // transition, same 20Hz-reference-rate convention used throughout this
    // engine).
    int GetBulldozerIcon(bool patrolGoesLeftFromStart, int patrolStep, int patrolTimeTicks);

    // Same shape as GetBulldozerIcon() above, for the other 3 patrol
    // enemies with a real, distinct per-direction/turn-transition icon
    // table (ENEMY-016/018/024, added 2026-07-20): ObjectType17 (fish),
    // ObjectType20 (bird), ObjectType44 (wasp/bee). Same parameter meaning
    // in each case.
    int GetFishIcon(bool patrolGoesLeftFromStart, int patrolStep, int patrolTimeTicks);
    int GetBirdIcon(bool patrolGoesLeftFromStart, int patrolStep, int patrolTimeTicks);
    int GetWaspIcon(bool patrolGoesLeftFromStart, int patrolStep, int patrolTimeTicks);

    // ObjectType54 (large creature, ENEMY-026, added 2026-07-20) -- same
    // real 4-state step machine, but the real data has no left/right
    // distinction at all (table_creature_left/right are byte-identical;
    // both turn steps share one table_creature_turn2), so there is no
    // direction parameter to take here.
    int GetCreatureIcon(int patrolStep, int patrolTimeTicks);

    // ObjectType32 (blupih, ENEMY-041) / ObjectType33 (blupit, ENEMY-042),
    // added 2026-07-20 -- same shape as GetBulldozerIcon()/GetFishIcon()/
    // GetBirdIcon()/GetWaspIcon(). The real projectile-fire trigger at the
    // same site in Decor.cpp is a separate, already-implemented mechanic
    // (ENEMY-020/022) -- these only port the icon selection.
    int GetBlupihIcon(bool patrolGoesLeftFromStart, int patrolStep, int patrolTimeTicks);
    int GetBlupitIcon(bool patrolGoesLeftFromStart, int patrolStep, int patrolTimeTicks);

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

    // BigDecor uses the same explo.png icon domain. The original Eggbert 2
    // renderer calls QuickIcon(CHEXPLO, icon, ...), even for trees, palms,
    // houses and other scenery. Keeping a named helper prevents these ids
    // from accidentally being interpreted as object-m.png terrain again.
    ObjectIconUv GetBigDecorIconUv(int icon);

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
    // plus ObjectType38's electric arc (added same day). The icon
    // GetObjIcon() returns for these types is a blupi.png/blupi1.png index
    // -- look it up via GetBlupiIconUv(). NOTE: ObjectType38 is only
    // blupi.png-sourced part of the time (its real behavior is two-channel)
    // -- this predicate alone is phase-blind and returns true for it
    // unconditionally; renderers that need the correct per-instance,
    // per-tick answer must use IsBlupiPngSourcedAtPhase() below instead.
    bool IsBlupiPngSourced(ObjectType type);

    // Phase-aware version of IsBlupiPngSourced() (2026-07-09) -- for
    // ObjectType200/201/202/203 identical to IsBlupiPngSourced() (always
    // true, they never switch sheets). For ObjectType38 (electric arc, a
    // real two-channel animation: blupi1.png for the first 30 of its
    // 90-tick cycle, element.png afterward, Decor.cpp ~line 8997), only
    // true while @p phase's tick is within that blupi1.png window --
    // GetObjIcon() already returns the matching correct icon for either
    // channel at any phase, but the CALLER still has to pick the right
    // texture/UV function (GetBlupiIconUv() vs GetElementIconUv()) via this
    // predicate, since a single MoveObject switches sheets mid-animation.
    bool IsBlupiPngSourcedAtPhase(ObjectType type, int phase);

    // True for the ObjectTypes that source blupi1.png instead of blupi.png
    // whenever they ARE blupi-sourced (see IsBlupiPngSourcedAtPhase() for
    // whether they currently are): the 3 of the 4 Blupi-skin types
    // (ObjectType201/202/203 -- ObjectType200 uses blupi.png itself) plus
    // ObjectType38's electric arc (always blupi1.png, never blupi.png,
    // during its blupi-sourced window). Per 03-objects.md, Blupi1_11/_12/
    // _13 all read the identical blupi1.png pixels in a raw crop -- any
    // tint difference between 201/202/203 is applied at render time in
    // mobile-eggbert, not reproduced here (no per-instance tinting exists
    // in GalaxyEggbertCNA yet), so all 3 render identically to each other
    // today.
    bool UsesBlupi1Texture(ObjectType type);
}
