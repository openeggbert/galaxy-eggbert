#pragma once
#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>

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

// Hub signs (`Decor::IsWorld()`, `Tables::world_terminal`): touching one makes Blupi wave
// Bye and go to sign N's destination, `WorldRuntime::ComputeWorldSelectTarget()`: world N's hub
// from the main hub (mission 1), mission N of this world from a world hub. Each world's sign
// has a second icon, shown once the world is completed (`Decor::AdaptDoors()`, mission 1):
// 158-165 -> 166-173 (worlds 1-8), 309 -> 310 (world 9), 411-415 -> 416-420 (worlds 10-14).
// 174-181 are the numbered mission signs of the world hubs, 184 the sign back to the main hub.
// (Until 2026-10-25 Galaxy used 166-169 as worlds 9-12 and 170-176 as invented door tiles.)
constexpr uint16_t WorldSelect1 = 158;
constexpr uint16_t WorldSelect2 = 159;
constexpr uint16_t WorldSelect3 = 160;
constexpr uint16_t WorldSelect4 = 161;
constexpr uint16_t WorldSelect5 = 162;
constexpr uint16_t WorldSelect6 = 163;
constexpr uint16_t WorldSelect7 = 164;
constexpr uint16_t WorldSelect8 = 165;
constexpr uint16_t WorldDone1 = 166;   // ...to 173: worlds 1-8 completed
constexpr uint16_t WorldSelect9 = 309;
constexpr uint16_t WorldDone9 = 310;
constexpr uint16_t WorldSelect10 = 411; // ...to 415: worlds 10-14
constexpr uint16_t WorldDone10 = 416;   // ...to 420
constexpr uint16_t MissionSign1 = 174;  // ...to 181: missions 1-8 of a world hub
constexpr uint16_t ReturnSign = 184;    // back to the main hub
// Galaxy's own portal to its showcase world999 (user-requested 2026-07-17, not in the
// original): the original's spare world-13 sign, a blank frame (Galaxy has 12 worlds).
constexpr uint16_t DemoPortal = 414;

inline bool isWorldSelect(uint16_t bt) {
    return GalaxyEggbert::GetBlockDefinition(bt).worldSelectIndex > 0;
}
inline int worldSelectIndex(uint16_t bt) {
    return GalaxyEggbert::GetBlockDefinition(bt).worldSelectIndex;
}

// Hub doors (`Decor::SearchDoor()`, `SearchGold()`, `AdaptDoors()`): in a world hub a blue bar
// (182) one or two cells beside mission sign N slides up once mission N-1 is won; in the main
// hub one gold bar (183) slides up per completed world. See Game/HubDoors.hpp.
constexpr uint16_t HubDoor = 182;
constexpr uint16_t GoldDoor = 183;


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

inline bool isSpring(uint16_t bt) { return GalaxyEggbert::GetBlockDefinition(bt).spring; }

// Temp tile — oscillating 20-frame cycle (icons 324-329); invisible for 2 frames.
// When SetActive(false) during invisible frames, Blupi falls through (faithful to mobile-eggbert IsTemp).
constexpr uint16_t Temp = 324;

// Ventilator fan tiles — base frames only (127-128, 130-131, 133-134, 136-137 are passable Air).
// Touching a fan at its mouth kills Blupi unless shielded (IsVentillo -> BlupiDead,
// WorldRuntime::TryConsumeFan()).
constexpr uint16_t FanLeft  = 126;  // blows in -X direction
constexpr uint16_t FanRight = 129;  // blows in +X direction
constexpr uint16_t FanUp    = 132;  // blows upward (+Y)
constexpr uint16_t FanDown  = 135;  // blows downward (-Y)

inline bool isFan(uint16_t bt) {
    return GalaxyEggbert::GetBlockDefinition(bt).fanDirection !=
           GalaxyEggbert::BlockFanDirection::None;
}

// Wind tiles (110-125): the air a fan blows, 110 toward -X, 114 toward +X, 118 up and 122 down
// (each with its three frames). They push Blupi (Decor.cpp:2771-2798) and are pass icons,
// nothing to stand on (2026-10-26).
inline bool isWind(uint16_t bt) {
    return bt >= 110 && bt <= 125;
}

// Bars and ropes (138, 202): Blupi hangs from them (Decor::GetTypeBarre), they never stop him,
// so a jump from under one rises into it and catches it.
inline bool isBarre(uint16_t bt) {
    return bt == 138 || bt == 202;
}

// The flight barrier (214): an invisible wall (drawn only by the ShowSecret cheat) that stops
// Blupi on foot and in the ground vehicles and lets the helicopter and the overcraft through
// (Decor::DecorDetect skips it in m_blupiHelico and m_blupiOver).
constexpr uint16_t FlightBarrier = 214;

// Blitz tile — electric floor, kills Blupi 25 % of ticks (animPhase % 4 == 0).
// Icon 305 is a solid floor with an active-kill phase (same pattern as Crusher).
constexpr uint16_t Blitz    = 305;

inline bool isBlitz(uint16_t bt) {
    return GalaxyEggbert::GetBlockDefinition(bt).hazard ==
           GalaxyEggbert::BlockHazardKind::Blitz;
}

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
    return GalaxyEggbert::GetBlockDefinition(bt).teleporterIndex > 0;
}

// Switch tiles — Blupi presses Action to toggle (384=open/active, 385=closed/inactive).
// ActiveSwitch scans ±20 tiles in X at same Z for linked saws and toggles them.
constexpr uint16_t Switch    = 384;  // open — linked saws spinning (deadly)
constexpr uint16_t SwitchOff = 385;  // closed — linked saws stopped (safe)

// Stopped saw state set by switch. 378 = spinning (hazard), 379 = stopped (safe).
constexpr uint16_t SawStopped = 379;

inline bool isSwitch(uint16_t bt) { return GalaxyEggbert::GetBlockDefinition(bt).switchBlock; }

// Bridge tile — passable trigger in mobile-eggbert; in galaxy-eggbert rendered as solid.
// When Blupi steps on it, bridge-building animation (ObjectType52, PICKUP-064) triggers.
constexpr uint16_t Bridge = 364;

inline bool isBridge(uint16_t bt) { return GalaxyEggbert::GetBlockDefinition(bt).bridge; }

// Marine tile — animated water surface (table_marine, 11 frames: 203→208→203).
constexpr uint16_t Marine = 203;

// Door tiles — blocked by key. Icon 334=red(key49), 335=green(key50), 336=blue(key51).
// Removed when Blupi is adjacent and holds the matching key.
constexpr uint16_t Door1    = 334;
constexpr uint16_t Door2    = 335;
constexpr uint16_t Door3    = 336;

inline bool isDoor(uint16_t bt)      {
    return GalaxyEggbert::GetBlockDefinition(bt).doorKeyType >= 0;
}
// The bars that close a way until something opens them: the hub doors (182, 183), the key
// doors (334-336) and the treasure doors (421-440). Their 2D mask is a bar down the cell's
// middle; in 3D they close the whole cell, or Blupi could slip past at its side along Z.
inline bool isDoorBar(uint16_t bt) {
    return bt == HubDoor || bt == GoldDoor || (bt >= Door1 && bt <= Door3) || (bt >= 421 && bt <= 440);
}
inline int  doorKeyType(uint16_t bt) {
    return GalaxyEggbert::GetBlockDefinition(bt).doorKeyType;
}

// Map any icon in an animated tile group to the group's base/master icon.
// All tiles in a group share one cached material whose UV is updated each frame.
inline uint16_t tileAnimBase(uint16_t icon) {
    return GalaxyEggbert::GetBlockDefinition(icon).animation.baseIcon;
}

// True for water tiles (91=deep water, 92=water surface) — Blupi swims when grounded on these.
inline bool isWater(uint16_t bt) {
    return GalaxyEggbert::GetBlockDefinition(bt).water;
}

// True if a tile type is a hazard (kills Blupi on contact when not shielded).
inline bool isHazard(uint16_t bt) {
    const auto hazard = GalaxyEggbert::GetBlockDefinition(bt).hazard;
    return hazard != GalaxyEggbert::BlockHazardKind::None &&
           hazard != GalaxyEggbert::BlockHazardKind::Fan;
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
    return icon >= 0 && GalaxyEggbert::IsSupportedBlockType(static_cast<uint16_t>(icon)) &&
           GalaxyEggbert::GetBlockDefinition(static_cast<uint16_t>(icon)).mobileTransparent;
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
