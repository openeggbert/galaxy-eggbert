# mobile-eggbert `.txt` Level File Format

**Status:** believed complete — all sections of the format (`DescFile`, `Decor:`, `BigDecor:`,
`MoveObject:`) have been identified and documented. Not yet verified: whether any *other* section
types exist beyond these four (only 78 real files were scanned; if a section type only appears in
files not yet inspected, it would be missed here).

Level files live in `../mobile-eggbert/worlds/world*.txt` (78 files). The authoritative parser is
`Decor::Read(int gamer, int rank, bool bUser)` in `src/WindowsPhoneSpeedyBlupi/Decor.cpp`
(~line 11301), which reads a `"DescFile"` section, a `100×100` `"Decor"` grid, a `100×100`
`"BigDecor"` grid, and a run of `"MoveObject"` records. (A *different*, much larger function a few
hundred lines earlier in the same file — with underscore-wrapped field names like `_blupiPos_` and
a `"Jauge"`/HUD section — is the **save-game** format, not the level format; it is out of scope
here and is the "byte-level save layout" already flagged as reference-only in `CLAUDE.md`.)

## 1. Header line (`DescFile`)

One line, space-separated `key=value` pairs. Real example (`world001.txt`, line 1):

```
DescFile: posDecor=250;5570 dimDecor=100;100 world=0 music=0 region=0 blupiPos=770;5894 blupiDir=2
```

Fields read by `Decor::Read()`:

| Field | Meaning | Currently parsed by galaxy-eggbert? |
|---|---|---|
| `posDecor` | initial scroll/camera position, pixel `x;y` | No |
| `dimDecor` | level dimensions in tiles, `x;y` (always `100;100` in practice) | No |
| `music` | music track index | No |
| `region` | background/sky region id (see `05-backgrounds.md`) | Yes — read into `skyRegion_`, but **not currently used** to pick a real background; galaxy-eggbert's Simple3D target instead picks one of 5 hardcoded sky colors indexed by *world number*, not this field (`GalaxyEggbertSimpleGame.cpp`, `TODO(S3D-sky)`) |
| `blupiPos` | Blupi's start position, pixel `x;y` | Yes — divided by 64 to get spawn tile |
| `blupiDir` | Blupi's start facing (`Direction`: 0=None,1=Left,2=Right) | No |

(`world=` also appears in the header but is not read by `Decor::Read()` in this port — likely
vestigial/unused for level files, distinct from the level's filename-encoded world number.)

## 2. `Decor:` grid — the primary tile map

A literal `Decor:` line, followed by exactly 100 lines, each a comma-separated row of 100 integers
(one row = one Z/row index, columns = X). Real excerpt (`world001.txt`, first `Decor:` data row, truncated):

```
Decor:
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,10,10,10,10,10,10,...
```

Cell value convention (from `Decor::Read()`): the raw integer is the tile icon index into
`object-m.png`. `0` (including an empty/missing cell, since trailing commas produce empty tokens)
means **no tile**, normalized in mobile-eggbert to `-1` ("empty"). Any positive integer is a real
icon id. galaxy-eggbert's `BlockTypes::fromMobileIconId()` already implements the equivalent
conversion (`icon <= 0` → `Air`), plus an additional step mobile-eggbert's *tile map* does not do
at load time: it also converts icons flagged in a 441-entry `isMobileTransparent` passability table
to `Air` (galaxy-eggbert's own collision simplification — collapsing "visually present but
non-blocking" 2D tiles straight to no-block, rather than keeping them as passable-but-textured
tiles). This is a galaxy-eggbert design choice, not something read from mobile-eggbert's file
format itself.

## 3. `BigDecor:` grid — a second tile layer

Immediately follows the `Decor:` grid: a literal `BigDecor:` line, then another `100×100`
comma-separated grid, same icon vocabulary (`object-m.png` via `PixmapChannel::Object`), same `-1`
= empty convention.

**Fixed 2026-07-03**: both galaxy-eggbert `GEWorldRuntime` implementations (Simple3D and CNA) now
correctly parse this section into a `GetBigDecor()`-exposed flat array — previously it was
silently dropped because the row-counter had already reached 100 from the main `Decor:` grid by
the time `BigDecor:` appeared. Parsing only; nothing renders it yet (see `09-open-questions.md`).

Confirmed non-empty in real files — e.g. `world013.txt`'s `BigDecor:` section has 14 real non-air
cells (verified via `tools/VerifyBigDecorParsing.cpp`), `world021.txt` has more. Not every level
uses it (`world001.txt`'s `BigDecor:` section is effectively empty — 2 non-air cells).

Purpose, from `Decor::Build()`'s render-order doc comment (`Decor.cpp` ~line 635): `BigDecor` is
drawn as its own back-to-front layer — parallax background image, **then `BigDecor`**, then the
main `Decor` tiles + objects + Blupi, then HUD. So it is a secondary, background-layer tile grid
using the same tile icon set as the main grid (including animated icons — the draw loop special-
cases icon `203`, the same `Marine` animated-water icon documented in `02-tiles.md`, via the same
frame table). Whether `BigDecor` cells participate in collision was not directly verified in this
pass and should be checked before deciding its 3D representation.

## 4. `MoveObject:` records — moving/interactive objects

Immediately follows `BigDecor:`: zero or more lines, each `MoveObject: field=value ...`, terminated
implicitly by end-of-section (next unrecognized section or EOF). Real example (`world001.txt`,
line 204, the file's only `MoveObject`):

```
MoveObject: type=7 stepAdvance=1 stepRecede=1 timeStopStart=0 timeStopEnd=0 posStart=962;5124 posEnd=962;5124 posCurrent=962;5124 step=1 time=0 phase=800 channel=10 icon=31
```

Full field list, from `Decor::Read()`:

| Field | Meaning |
|---|---|
| `type` | `ObjectType` id (see `03-objects.md`) |
| `stepAdvance` | ticks to move from `posStart` to `posEnd` (run through `Config::ScaleTime()`) |
| `stepRecede` | ticks to move back from `posEnd` to `posStart` |
| `timeStopStart` | dwell ticks at `posStart` before advancing |
| `timeStopEnd` | dwell ticks at `posEnd` before receding |
| `posStart`, `posEnd` | endpoints of linear motion, pixel `x;y` (equal for stationary objects) |
| `posCurrent` | current position, pixel `x;y` (redundant with the above at file-save time) |
| `step` | current motion phase index (redundant/save-time state) |
| `time` | current dwell-timer value (redundant/save-time state) |
| `phase` | current animation phase counter (redundant/save-time state) |
| `channel` | sprite sheet id (`PixmapChannel`, e.g. `10` = `Element`) |
| `icon` | current icon index within that sheet (redundant/save-time state) |

`posCurrent`/`step`/`time`/`phase`/`icon` are runtime/save-time snapshots of values the simulation
recomputes every frame from `type`/`stepAdvance`/`stepRecede`/`posStart`/`posEnd` — a fresh level
load does not strictly need them to be meaningful, only the first seven fields define the object's
design-time behavior.

**Fixed 2026-07-03**: `GEWorldRuntime::LoadFromMobileEggbertFile` (Simple3D) previously only
instantiated a hardcoded subset of `type` values, silently dropping 12 real, in-use types. A scan
of all 78 real world files found `type=` values in actual use:
`1,2,3,4,5,6,7,12,13,16,17,19,20,21,24,26,30,32,33,40,44,46,47,49,50,51,54,55,96`. All 29 of these
now spawn (verified via `tools/VerifyMoveObjectTypes.cpp` against real level files, 12/12 newly
added types confirmed) — see `03-objects.md` for per-type behavior status. The CNA target's
`GEWorldRuntime::LoadFromMobileEggbertFile` still doesn't parse `MoveObject:` lines at all
(explicitly deferred in its source comments — out of scope for this reference pass).
