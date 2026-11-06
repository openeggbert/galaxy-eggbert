# World 199 — the bonus world, rebuilt as a journey (2026-11-06)

Built by `tools/worlds/world199.py` → `worlds3d/world199.vwr` (mission 199, region 31, music 1, no
treasure, the red arrow ends the game: the win screen). Direction (owner, 2026-11-06): "just take
strong inspiration from Mobile Eggbert's final world and do it in 3D somehow". The audit is
`plans/audit/world199_comparison.md` (read §2 for the original's twelve regions, §7 for this plan's
source, §8 for its gates); this brief is the design that was built, with its coordinates.
The first version of this world (a plaza, a ring gallery and eight 13 x 13 enclaves) was replaced:
it kept 95 of the original's 196 objects and none of its 47 BigDecor and read as a hub exercise.

## The shape: one descent, one climb, twelve places

The player starts at the top of the north-west corner and travels down through eleven places to the
bottom of the world, then climbs a monument at the north-west corner again, directly under where he
started. Each place is a hand-authored volume cut in a single mass of slime (341-363) or built of
its own world's material inside it; heights are *walking levels* (a floor block at y gives walk
y + 1), X and Z are raw cells.

| | Place (the original's region) | X x Z | Walk | Material | Enters from | Leaves to |
|---|---|---|---|---|---|---|
| A | Gunfire shaft (the start shaft and gun ledges) | 8-20 x 8-20 | 92 down to 72 | slime | spawn pad (9, 92, 9) | tunnel east at z 13-15 |
| B | Basin and fan switchyard (upper-right basin, star passages, fan tower) | 30-52 x 6-26 | 72 up to 86 | slime, star blocks, grey plate | A's tunnel (west wall) | the blue gate (B1) in the south wall, into C |
| C | Lift comb and brick library (central lift shafts, brick library) | 30-52 x 32-50 | 70 down to 52 | brick, bookcases | B's blue gate (top) | the green gate (G1) west, into D |
| D | Palace and west galleries (white palace, teleporters) | 8-24 x 54-76 | 62 down to 50 | white stone | C's green gate | teleporters 330/331 to E, or the stair down |
| E | Lava terraces, lightning edge, wasp well (west lava terraces, machine edge) | 8-26 x 54-78 | 46 down to 34 | slime, blue panel | D (teleporter or stair) | tunnel east to F at z 48 |
| F | Village roofs (the village street) | 32-62 x 30-48 | 34 | cobble, facades, slate | E's tunnel | tunnel east at z 42 |
| G | Riveted station (the grey-metal station) | 64-92 x 32-56 | 34 down to 26 | grey plate, grilles, girders | F's tunnel | the blue gate (B2) south, into H |
| H | Cheese passages (cheese tunnels) | 66-92 x 60-84 | 26 down to 18 | cheese | G's blue gate | tunnel west to I at z 70 |
| I | Helicopter jungle (helicopter/slime cavern network) | 30-62 x 52-88 | 38 down to 10 | slime, star ledges | H's tunnel (east wall) | tunnel east to J at z 55 |
| J | Rock gauntlet (brown-rock circuit) | 66-96 x 36-58 | 14 down to 6 | brown rock | I's tunnel; the blue gate (B3) | passage north-west to K |
| K | Toy chamber | 30-52 x 8-26 | 6 up to 22 | ball blocks, star accent | J's passage (east wall) | its top ledge, west wall |
| L | Basin, column ascent, monument (lower water basin, columns, ending) | 8-26 x 8-34 | 8 up to 38 | water, slime, columns, stars | K's ledge (a dive into the basin) | the red arrow |

(Heights, sizes and doors are as built in `tools/worlds/world199.py`; the section constants at the
top of each `section_X` function are the authority, this table is its summary.)

## The key-state route (two red, three green, three blue; one flag each, consumed by a door)

1. **B** holds R1 and B1 on different branches. B1 opens B's blue gate to C (B1 consumed).
2. **C**: R1 opens the library's red door (consumed); inside lie R2 and G1. G1 opens D's green gate
   (consumed). R2 opens D's optional palace-gallery loop (a second way to B2) and may simply stay held.
3. **D** holds G2 and B2. G2 opens the village's optional roof route (F); the street stays an
   alternative. B2 opens G's blue gate south into H (consumed).
4. **G** holds B3, reachable only after B2 is spent. B3 opens J's blue gate (the rock route).
5. **L** holds G3. A player still carrying optional G2 opens the final green gate with it; G3 is then
   not needed. The final green gate shows the compact star frame and the red arrow: the win screen.

No door can be flown over or entered from behind; every key lies on a way that a held key of its
colour does not block (a second key of a held colour cannot be taken, so each is placed after the
door of its colour). 2 + 3 + 3 keys, 2 + 3 + 3 door groups.

## Object budget (196 records; 20 types) — where each goes

| | Eggs | Lifts | Conv. 47 | Heli | M2 | M3 | M16 | Bull. | Bird | Blupi 33 | Big 54 | Star 96 | other |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| A | 2 | 2 | | | | | 3 | | | 4 | | | |
| B | 4 | 3 | 2 | | 1 | 1 | 3 | | | | | 4 | lollipop, invert, R1, B1 |
| C | 5 | 9 | | | | | 5 | 3 | | | | | 2 crates, R2, G1 |
| D | 4 | 4 | | | 5 | 4 | 6 | | 2 | | 2 | | invert, G2, B2 |
| E | 2 | 3 | | | 1 | 1 | 5 | | | | | | wasp |
| F | 6 | 1 | | | | | | | 7 | | | | |
| G | | 2 | | | | | 4 | | | | | 10 | B3 |
| H | 3 | | | 2 | | 3 | 4 | 3 | | | | | |
| I | 6 | | | 18 | | | 1 | | | | 4 | | |
| J | 2 | | | | 2 | | 5 | | | | | | |
| K | 2 | | | | | | | | | | | | |
| L | 2 | 2 | | 2 | | | | | | | | | G3, red-arrow exit |
| **Σ** | **38** | **26** | **2** | **22** | **9** | **9** | **36** | **6** | **9** | **4** | **6** | **14** | 2 crates, 1 lollipop, 2 inverts, 1 wasp, 8 keys, 1 exit |

Of the 36 type-16 records 27 travel and 9 stand still (D 2, E 3, H 4 stationary). The 26 lift
records are independent journeys except the paired decks the original also had.

## Tile hazards and decor

Fans 21 (10 left, 2 right, 9 up): B 10 / G 7 / J 4, each with its wind lane; saucers 324 x 5 in B;
teleporters 330/331 two pairs, D to E; flight barriers 214 x 22 (12 round A's start pad, 10 as the
toy chamber's filters); crumbling bridge 364 x 9 (E 5, J 4); saws 378 x 15 with three switch rows
(C, D, D); lightning 304 x 4 and 305 x 3 at E's approach; spikes 373 x 2 and drips 404 x 2
separated in I; lava 68 in E, G, J; water 91/92 in B, E's wasp well (none), L; bars 202 over E's
lava and J's trench.

BigDecor, 47: palm 18 x 1 (L's island), leafy plants 23/24 x 2 + 2 (B and L pools, landing
shelves), ball machines 29 x 2 (B, G), stone columns 66/67/68 x 6 + 7 + 11 (D's courtyard, C's
library, the ascent in L), vases 69 x 5 (D, L), monument column 87 x 1, fountains 88 x 3 (one in D,
two in L), Blupi statues 89 x 2 (D's recess, the ending), water weed 203 x 5 (in B's and L's water).
Solid ones leave lanes of at least 0.6 cell and never stand in a route's only lane.

## The ending

L: a basin (swum, weeds, a palm on its island) -> columns and star blocks to climb, two short star
lifts -> a terrace in the north-west corner under A, looking back through an opening at the shaft's
column of light -> the Blupi statue on its monument column between two fountains -> the green gate
-> a compact frame of star blocks (the only star-block place beside the toy chamber's accent) with
the red arrow in it -> the win screen. The terrace's walls are slime and stone, not star wallpaper.
