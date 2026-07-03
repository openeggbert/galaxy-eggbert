# Doors — Detailed Behavior (2D)

**Status:** believed complete for the 2D behavior itself (icon range, key mapping, opening
animation, collision) — the open item is entirely on the 3D-mapping side, not further 2D research.

Confirmed directly from `Decor.cpp` (not just inferred from `BlockTypes.hpp`):

- Door tiles are icons 334, 335, 336 in the main `Decor` grid (`Decor::IsDoor`, `Decor.cpp` ~7360:
  tests `icon >= 334 && icon <= 336`).
- The required key is derived as a bitmask: `doorKeyMask = 1 << (icon - 334)` — icon 334 needs bit
  0, 335 needs bit 1, 336 needs bit 2, matching `DoorKeyFlags::Key1/Key2/Key3` and
  `ObjectType49/50/51` (the three key pickups) exactly.
- Opening (`Decor::OpenDoor`, `Decor.cpp` ~11667): the tile's `icon` is set to `-1` (removed from
  the static grid, becomes passable) and a **temporary `MoveObject` of type 22** ("door opening
  animation") is spawned at that cell, using the door's own icon, sliding from the door's position
  upward by one tile (`posEnd.Y = posStart.Y - 1`) over `stepAdvance = Config::ScaleTime(50)` ticks,
  playing `SoundChannel33`. So visually a door **slides up and out of the way**, it does not fade,
  shatter, or swing.
- Doors are solid/blocking while present (implied by being a normal `Decor` grid icon, subject to
  the same `IsBlocIcon()` tile collision as walls) — i.e. in 2D a door is simply an opaque, solid
  textured tile like any other wall tile until removed.

Door icon crops (see `02-tiles.md`'s image-generation note):

![Door1](images/tile-334-Door1.png) ![Door2](images/tile-335-Door2.png) ![Door3](images/tile-336-Door3.png)

## Open question for the 3D mapping (not decided here)

The user wants doors in the 3D target rendered as **billboards with transparency** rather than
opaque textured cubes — presumably so a door can visually read as a thin barrier/frame rather than
a solid block, and/or so the slide-up-and-vanish animation translates naturally to a billboard
sliding along Y. This document does not resolve how (a new `BlockMetadata`-tagged block? a distinct
object type layered over an `Air` cell? something else) — that is exactly the kind of decision this
catalog exists to inform, not make. Tracked in `09-open-questions.md`.
