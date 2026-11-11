# World 201 — the first multiplayer world (the machine arena)

Built by `tools/worlds/world201.py` → `worlds3d/world201.vwr` (mission 201, region 0, music 2 as the
original). Played only in a network game (`plans/plan_gemultiplayer.md`, owner Q-019): the lobby's
host chooses it, every player must have the same file (the content hash).

**Reference** (Speedy Blupi 2, `DATA/WORLD201.BLP`, read with a throwaway DescFile dump of
`blupi_recon/games/sb2/src/decor.h`'s layout): a small closed arena of the blue machine panel
(icon 10, 768 of its 778 cells), four starts on one floor (x 450, 834, 1090, 1474 px, row 10,
facing alternately), three lifts (1), sixteen helicopters (13), sixteen tanks (28), twenty-six
ammunition packs (29), eight drinks (30) and thirteen personal bombs of each team (200-203). No
crate, goal or enemy Blupi (a network game would take them out, decblock.cpp:1430).

**3D design**: a closed machine hall (x, z 36–64, floor walk 10, roof at 22: room for the
helicopters, the flight barrier would let them out).
- The four starts in the corners (yellow 39,38; orange 61,38; blue 39,62; green 61,62), each with
  a row of four of its team's bombs beside it.
- A raised centre (x, z 46–54, walk 14) reached by stairs north and south and a lift east; two
  helicopters, ammunition and a drink on it.
- Balconies along the west and east walls (walk 14), each with its lift and two helicopters.
- Four tanks on the floor, ammunition round the hall, drinks.

**Checks**: `VerifyShippedWorlds` (loads, header mission 201); a two-machine run (two instances
under Xvfb, `GALAXY_NET_NAME`) starts in it with each player at his colour's start.
