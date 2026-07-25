#pragma once
#include <cstdint>

// Block type IDs for galaxy-eggbert's voxel World format.
//
// Design: block type = icon index into object-m.png.
// Air = 0 (no block). Every other value is the icon ID displayed as the
// tile face texture. Named constants equal their icon IDs so gameplay code
// can reference them by name without a separate mapping table.

namespace GalaxyEggbert {
namespace BlockTypes {

// object-m.png dimensions: 1301×1431 px, 64×64 px per tile, 20 columns.
// Real packing (confirmed from mobile-eggbert's Pixmap::GetSrcRectangle,
// PixmapChannel::Object case: srcGap=1) is NOT a flat 64px grid — each cell
// sits on a 65px pitch (64px icon + 1px gap), with a 1px leading margin:
// pixelX = kSheetGap + col*(kTileSize+kSheetGap), same for Y. Column/row
// count is still derived from kTileSize alone (matches mobile-eggbert's own
// `width / bitmapGridX` using the pre-gap grid size), so kSheetCols is
// unaffected — only the per-tile pixel offset needs the gap term (see
// tileUV() below).
constexpr int kSheetW    = 1301;
constexpr int kSheetH    = 1431;
constexpr int kTileSize  = 64;
constexpr int kSheetGap  = 1;
constexpr int kSheetCols = kSheetW / kTileSize; // 20

constexpr uint16_t Air      =   0;   // empty / no block

// Named tile types — value equals the icon index in object-m.png.
// NOTE (2026-07-08): Ground/StoneA/StoneB's names/comments below are WRONG --
// direct user identification (mobile-eggbert-reference/
// questionnaire-all-remaining-tiles.md, round 2) found all three to actually
// be machine-piece graphics (DirectionalCube render mode: 1-2 textured faces
// + flat-color/transparent fallback faces), not bulk ground/stone material.
// Kept here (not renamed/removed) because renaming would require touching
// every existing usage; do not add new bulk-terrain usages of these three --
// use BrickWall/RockPile below instead, which ARE confirmed genuine bulk
// material (UniformCube, textured on all 6 faces).
constexpr uint16_t Ground   =  10;   // grass/ground
constexpr uint16_t StoneA   =  18;   // light stone
constexpr uint16_t StoneB   =  25;   // dark stone
// Confirmed genuine bulk terrain material (UniformCube, all 6 faces textured)
// via the same round-2 identification pass above -- safe replacements for
// StoneA/StoneB in new bulk-fill code.
constexpr uint16_t RockPile  =  35;   // pile of rocks, non-passable
constexpr uint16_t BrickWall = 261;   // brick wall, non-passable
// Renamed from Wall (2026-07-06) -- confirmed by direct crop inspection to be a
// golden pillar/post, not brick-wall texture; rare (1/78 files, only 12 cells,
// forming a gate/portal-frame shape immediately after icon 182, the real door
// tile). See mobile-eggbert-reference/02-tiles.md and
// mobile-eggbert-reference/15-3d-render-mapping-design.md for the full finding.
constexpr uint16_t GoldPillar = 183;
// NOTE (2026-07-08): "floating platform" below is WRONG in the same way as
// Ground/StoneA/StoneB above -- round-1 Q&A (mobile-eggbert-reference/
// questionnaire-unidentified-tiles.md, confirmed again in 02-tiles.md's icon
// 200 entry) found this to be a passable grate/grid graphic (DirectionalCube:
// 4 textured side faces, top/bottom genuinely open, not a fallback color),
// not a real solid floor. Do not use it for new solid-floor fills -- use
// RockPile/BrickWall (UniformCube, confirmed genuine bulk material) instead
// until the DirectionalCube render mode (see NEXT.md §8 task 2) exists to
// render it correctly as an open grate.
constexpr uint16_t Platform = 200;   // floating platform

// Real hub-screen world-select markers (`Decor::IsWorld()`, icons 158-165,
// plan.md `170`/`TILE-057`) -- these 8 constants used to be named `Sp0`-`Sp7`
// under an earlier, now-debunked "GalaxyEggbert::Def::SecretPower 0-7" hypothesis (the real
// GalaxyEggbert::Def::SecretPower enum only has 5 values and comes from unrelated MoveObject
// pickups instead, see `170`'s own writeup). Renamed 2026-07-17 once put to
// actual use for the hub/mission-progression system: touching one selects
// world/level N (1-8), the real destination computed contextually by
// `GEWorldRuntime::ComputeWorldSelectTarget()` depending on the CURRENT
// mission (same real marker range means "pick a world" from the global hub,
// or "pick a level" from a world hub -- exactly mirroring real source).
constexpr uint16_t WorldSelect1 = 158;
constexpr uint16_t WorldSelect2 = 159;
constexpr uint16_t WorldSelect3 = 160;
constexpr uint16_t WorldSelect4 = 161;
constexpr uint16_t WorldSelect5 = 162;
constexpr uint16_t WorldSelect6 = 163;
constexpr uint16_t WorldSelect7 = 164;
constexpr uint16_t WorldSelect8 = 165;
// Real source's own icons 166-173 are a SECOND bank of the same 8 markers
// (the "unlocked" cosmetic-icon-swap variant, `Decor::AdaptDoors()`'s
// `m_mission==1` branch -- confirmed NOT a real access gate, just a visual
// reveal of a per-world hidden-gold flag, see plan.md SCORE-013's full
// writeup). Not modeled here (no cosmetic swap), so this engine reuses
// 166-169 as 4 genuinely NEW indices (9-12) instead, extending the real
// 8-world-marker range to the full 12 real world hubs this session's
// content actually needs (found 2026-07-17).
constexpr uint16_t WorldSelect9  = 166;
constexpr uint16_t WorldSelect10 = 167;
constexpr uint16_t WorldSelect11 = 168;
constexpr uint16_t WorldSelect12 = 169;

inline bool isWorldSelect(uint16_t bt) { return bt >= WorldSelect1 && bt <= WorldSelect12; }
inline int worldSelectIndex(uint16_t bt) { return isWorldSelect(bt) ? static_cast<int>(bt - WorldSelect1 + 1) : -1; }

// Real per-world sublevel-select door gate (`Decor::AdaptDoors()`'s
// `m_mission % 10 == 0` branch, `SearchDoor()`/icon `182`, found
// 2026-07-17): within a world hub, sublevel-select marker index N (2-8;
// index 1 is always ungated) sits behind a real solid door tile that opens
// once the PREVIOUS sublevel has been completed (`Decor::OpenDoorsWin()`'s
// `m_doors[mission+1]=1`). This engine keys the same real per-mission
// unlock flag directly (`GESaveData::IsMissionDoorUnlocked()`) rather than
// porting the real 200-entry `m_doors[]` array's exact indexing scheme.
// icons 170-176 are a free, previously-unused range (distinct from real
// source's own icon 182 -- this engine doesn't need to match that exact
// value, only the gating BEHAVIOR).
constexpr uint16_t ProgressDoor2 = 170;
constexpr uint16_t ProgressDoor3 = 171;
constexpr uint16_t ProgressDoor4 = 172;
constexpr uint16_t ProgressDoor5 = 173;
constexpr uint16_t ProgressDoor6 = 174;
constexpr uint16_t ProgressDoor7 = 175;
constexpr uint16_t ProgressDoor8 = 176;

inline bool isProgressDoor(uint16_t bt) { return bt >= ProgressDoor2 && bt <= ProgressDoor8; }
inline int progressDoorIndex(uint16_t bt) { return isProgressDoor(bt) ? static_cast<int>(bt - ProgressDoor2 + 2) : -1; }

// Engine-specific (NOT a real mobile-eggbert concept): a dedicated portal
// in the global hub leading to `world999.vwr`, this engine's own quarantined
// mechanics-showcase/test world (found 2026-07-17, user-requested --
// mobile-eggbert's real 78-world structure gets a genuine 79th world here,
// purely for this engine's own development/testing use, never confused
// with a real mission number). Always leads to mission 999 directly,
// unlike `WorldSelect`'s contextual `N*10`/`X0+N` reinterpretation.
constexpr uint16_t DemoPortal = 177;

constexpr uint16_t Marker   = 309;
constexpr uint16_t Tile411  = 411;
constexpr uint16_t Tile412  = 412;
constexpr uint16_t Tile413  = 413;

// Hazard tiles — same rule: value = icon index. Gameplay checks use these.
constexpr uint16_t Lava     =  68;   // kills Blupi on contact (anim group 68–72)
constexpr uint16_t Spike    = 373;   // kills Blupi on contact (anim group 347,373,374)
constexpr uint16_t Crusher  = 317;   // kills Blupi on contact (anim group 317–323)
constexpr uint16_t Saw      = 378;   // kills Blupi on contact (anim group 378–383)
// "Water drip" (real Decor::IsGoutte, confirmed 2026-07-14 via direct
// Decor.cpp read: icon 404 kills on contact, real GalaxyEggbert::Def::BlupiAction::Glu, same
// gate/sound/behavior shape as Spike -- NOT a slow/glue debuff despite the
// name, a real 6th confirmed instant-kill hazard). Real visual appearance is
// a green vase/bulb-on-a-neck (Billboard render mode per
// mobile-eggbert-reference/02-tiles.md), not a drip/liquid graphic despite
// the functional name -- render falls back to the default UniformCube here,
// which is a correct, already-accounted-for resolution (no dedicated visual
// identified as more faithful).
constexpr uint16_t Drip     = 404;   // kills Blupi on contact (real IsGoutte)

// Water tiles — decorative (anim groups 92–95 and 96–98).
constexpr uint16_t Water1   =  92;
constexpr uint16_t Water2   =  96;

// Spring tile — launches Blupi upward automatically on contact (SoundChannel41).
constexpr uint16_t Spring   = 211;

inline bool isSpring(uint16_t bt) { return bt == Spring; }

// Temp tile — oscillating 20-frame cycle (icons 324-329); invisible for 2 frames.
// When SetActive(false) during invisible frames, Blupi falls through (faithful to mobile-eggbert IsTemp).
constexpr uint16_t Temp = 324;

// Ventilator fan tiles — base frames only (127-128, 130-131, 133-134, 136-137 are passable Air).
// Fan blows Blupi: kills on contact unless shielded (mobile-eggbert IsVentillo → BlupiDead).
constexpr uint16_t FanLeft  = 126;  // blows in -X direction
constexpr uint16_t FanRight = 129;  // blows in +X direction
constexpr uint16_t FanUp    = 132;  // blows in -Z direction (2D: up = -Y)
constexpr uint16_t FanDown  = 135;  // blows in +Z direction (2D: down = +Y)

inline bool isFan(uint16_t bt) {
    return bt == FanLeft || bt == FanRight || bt == FanUp || bt == FanDown;
}

// Blitz tile — electric floor, kills Blupi 25 % of ticks (animPhase % 4 == 0).
// Icon 305 is a solid floor with an active-kill phase (same pattern as Crusher).
constexpr uint16_t Blitz    = 305;

inline bool isBlitz(uint16_t bt) { return bt == Blitz; }

// Blitz emitter -- the icon real `Decor::BlitzActif()` checks one cell above a
// Blitz(305) floor tile to decide whether to play the zap sound (Decor.cpp:
// 620-634, found 2026-07-17). Not itself hazardous; kPassable[304]==false so
// this engine's own world loader (BlockTypes::fromMobileIconId()) already
// preserves it verbatim rather than collapsing it to Air.
constexpr uint16_t BlitzEmitter = 304;

// Teleporter tiles — solid pillars; icons 330-333 encode the pair ID.
// When Blupi is adjacent, scan the world for the matching icon to find the exit.
constexpr uint16_t Teleport1 = 330;
constexpr uint16_t Teleport2 = 331;
constexpr uint16_t Teleport3 = 332;
constexpr uint16_t Teleport4 = 333;

inline bool isTeleporter(uint16_t bt) {
    return bt == Teleport1 || bt == Teleport2 || bt == Teleport3 || bt == Teleport4;
}

// Switch tiles — Blupi presses Action to toggle (384=open/active, 385=closed/inactive).
// ActiveSwitch scans ±20 tiles in X at same Z for linked saws and toggles them.
constexpr uint16_t Switch    = 384;  // open — linked saws spinning (deadly)
constexpr uint16_t SwitchOff = 385;  // closed — linked saws stopped (safe)

// Stopped saw state set by switch. 378 = spinning (hazard), 379 = stopped (safe).
constexpr uint16_t SawStopped = 379;

inline bool isSwitch(uint16_t bt) { return bt == Switch || bt == SwitchOff; }

// Bridge tile — passable trigger in mobile-eggbert; in galaxy-eggbert rendered as solid.
// When Blupi steps on it, bridge-building animation (ObjectType52, PICKUP-064) triggers.
constexpr uint16_t Bridge = 364;

inline bool isBridge(uint16_t bt) { return bt == Bridge; }

// Marine tile — animated water surface (table_marine, 11 frames: 203→208→203).
constexpr uint16_t Marine = 203;

// Door tiles — blocked by key. Icon 334=red(key49), 335=green(key50), 336=blue(key51).
// Removed when Blupi is adjacent and holds the matching key.
constexpr uint16_t Door1    = 334;
constexpr uint16_t Door2    = 335;
constexpr uint16_t Door3    = 336;

inline bool isDoor(uint16_t bt)      { return bt == Door1 || bt == Door2 || bt == Door3; }
inline int  doorKeyType(uint16_t bt) {
    if (bt == Door1) return 49;
    if (bt == Door2) return 50;
    if (bt == Door3) return 51;
    return -1;
}

// Map any icon in an animated tile group to the group's base/master icon.
// All tiles in a group share one cached material whose UV is updated each frame.
inline uint16_t tileAnimBase(uint16_t icon) {
    if (icon >= 68  && icon <= 72)  return Lava;     // lava 8-frame loop
    if (icon == 347 || icon == 373 || icon == 374) return Spike; // spike 16-frame
    if (icon >= 317 && icon <= 323) return Crusher;  // crusher 10-frame
    if (icon >= 378 && icon <= 383) return Saw;      // saw 6-frame loop
    if (icon >= 92  && icon <= 95)  return Water1;   // water1 6-frame loop
    if (icon == 91  || (icon >= 96 && icon <= 98)) return Water2; // water2 6-frame
    if (icon >= 126 && icon <= 128) return FanLeft;  // fan-left 3-frame
    if (icon >= 129 && icon <= 131) return FanRight; // fan-right 3-frame
    if (icon >= 132 && icon <= 134) return FanUp;    // fan-up 3-frame
    if (icon >= 135 && icon <= 137) return FanDown;  // fan-down 3-frame
    if (icon >= 324 && icon <= 329) return Temp;     // temp tile 20-frame
    if (icon >= 203 && icon <= 208) return Marine;   // marine 11-frame
    return icon;
}

// True for water tiles (91=deep water, 92=water surface) — Blupi swims when grounded on these.
inline bool isWater(uint16_t bt) {
    uint16_t b = tileAnimBase(bt);
    return b == Water1 || b == Water2;
}

// True if a tile type is a hazard (kills Blupi on contact when not shielded).
inline bool isHazard(uint16_t bt) {
    uint16_t base = tileAnimBase(bt);
    return base == Lava || base == Spike || base == Crusher || base == Saw || base == Blitz;
}

// Block type → icon index. Since block type IS the icon index, this is trivial.
// Returns -1 for Air.
inline int toIconIndex(uint16_t t) {
    return (t == Air) ? -1 : static_cast<int>(t);
}

// True for icon IDs that have all-zero table_decor_quart sub-cells in
// mobile-eggbert (fully transparent/decorative — no collision).
// Icons 68 (Lava) and 317 (Crusher) are excluded and kept solid for the
// galaxy-eggbert hazard system even though they are quart-passable.
inline bool isMobileTransparent(int icon) {
    static const bool kPassable[441] = {
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true, false, true, true, true, true, true, false, false, true, true, false, false,
        false, false, false, false, false, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, false, true, true, false, true, true, false, true, true, false, true, true, true, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true,
        true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        true, true, false, false, true, true, false, false, false, false, false, false, false, false, false, false, false, false, true, true,
        true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true,
        true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        true, true, true, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        true, false, true, true, false, false, true, true, true, true, true, false, false, false, false, false, false, false, true, true,
        true, true, true, true, false, true, true, true, true, true, false, false, false, false, false, false, false, true, false, true,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, true, true, true, true, true, true, false, false, true, true, true, false, false,
        true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true,
        true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false
    };
    return icon >= 0 && icon < 441 && kPassable[icon];
}

// Convert a mobile-eggbert decor icon ID to a block type.
// Passable (decorative) tiles become Air; everything else keeps its icon ID.
// 0 or negative → Air.
inline uint16_t fromMobileIconId(int icon) {
    if (icon <= 0) return Air;
    if (isMobileTransparent(icon)) return Air;
    return static_cast<uint16_t>(icon);
}

// UV of tile for icon index (row-major, 20 cols).
// Pixel offset accounts for the real 65px pitch (kTileSize + kSheetGap), not
// a flat 64px grid — see kSheetGap's comment above. Without this, sampling
// drifts by 1px per column/row and bleeds neighbouring icons' pixels in by
// the time it reaches later rows/columns (confirmed visually: icon 68 was
// picking up ~4-9px of icon 48's content before this fix).
// Half-pixel inset: at u_box=1.0, GPU computes floor(u_atlas * W) which without
// inset lands on the FIRST pixel of the NEXT atlas tile, producing a dark seam.
// Inset moves the sampled range to pixel centres [+0.5px .. lastPx-0.5px].
inline void tileUV(int icon, float& uOff, float& vOff, float& uScale, float& vScale) {
    constexpr int kPitch = kTileSize + kSheetGap;
    const int col = icon % kSheetCols;
    const int row = icon / kSheetCols;
    const float pxX = static_cast<float>(kSheetGap + col * kPitch);
    const float pxY = static_cast<float>(kSheetGap + row * kPitch);
    const float stepU = static_cast<float>(kTileSize) / static_cast<float>(kSheetW);
    const float stepV = static_cast<float>(kTileSize) / static_cast<float>(kSheetH);
    const float halfU = 0.5f / static_cast<float>(kSheetW);
    const float halfV = 0.5f / static_cast<float>(kSheetH);
    uOff   = pxX / static_cast<float>(kSheetW) + halfU;
    vOff   = pxY / static_cast<float>(kSheetH) + halfV;
    uScale = stepU - 2.0f * halfU;
    vScale = stepV - 2.0f * halfV;
}

} // namespace BlockTypes
} // namespace GalaxyEggbert
