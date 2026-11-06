# World 199 — the bonus world, rebuilt as a journey (2026-11-06)

Built by `tools/worlds/world199.py` → `worlds3d/world199.vwr` (mission 199, region 31, music 1, no
treasure, the red arrow ends the game: the win screen). Staged variant for `levelcheck.py`:
`--stage 1 out.vwr` (every door open, no hazard, creature or saw; the lifts, ferries over lava aside,
as built). Direction (owner, 2026-11-06): "just take strong inspiration from Mobile Eggbert's final
world and do it in 3D somehow". The audit is `plans/audit/world199_comparison.md` (§2 the original's
twelve regions, §7 this plan's source, §8 its gates, "Rebuild done" the result).
The first version of this world (a plaza, a ring gallery and eight 13 x 13 enclaves) was replaced: it
kept 95 of the original's 196 objects and none of its 47 BigDecor and read as a hub exercise.

## The shape: one descent, one climb, twelve places

The player starts at the top of the north-west corner and travels down through eleven places to the
bottom of the world, then climbs a monument at the north-west corner again, under where he started.
Each place is a hand-authored volume cut in one mass of slime (341-363), or built of its own world's
material inside it. Heights are *walking levels* (a floor block at y gives walk y + 1); X and Z are raw
cells. Each place is one `section_x()` function of the generator, whose docstring is the authority for
its coordinates; this table is the summary.

| | Place (the original's region) | X x Z | Walk | Material | Enters from | Leaves to |
|---|---|---|---|---|---|---|
| A | Gunfire shaft (the start shaft and gun ledges) | 8-20 x 8-20 | 92 down to 72 | slime | spawn pad (9, 92, 9) | tunnel east (z 13-15) |
| B | Basin and fan switchyard (upper-right basin, star passages, fan tower) | 30-53 x 6-26 | 72 up to 86 | slime, stars | A's tunnel | blue gate (B1), south wall (z 28) |
| C | Lift comb and brick library | 32-51 x 34-48 | 82 down to 58 | brick, bookcases | B's gate (balcony, walk 82) | green gate (G1), west wall (x 31) |
| D | White palace and its galleries (palace, teleporters) | 11-25 x 41-65 | 58 up to 70 | white stone | C's green gate | teleporter 330 (the keep) to E |
| E | Lava terraces, lightning passage, wasp well | 10-26 x 43-63 | 46 down to 34 | blue machine panel | teleporter 330 | passage east (z 45) to F |
| F | Village roofs (village street) | 33-61 x 31-45 | 34 | cobble, facades, slate | E's passage | tunnel east (z 43) |
| G | Riveted station (grey-metal station) | 67-91 x 34-54 | 34 / 30 | grey plate | F's tunnel, blue gate (B2) | tunnel south (x 88-90) |
| H | Cheese passages (cheese tunnels) | 62-92 x 60-84 | 26 down to 18 | cheese | G's tunnel (high in the first cave) | tube west (z 74) |
| I | Helicopter jungle (helicopter/slime cavern network) | 31-61 x 53-87 | 18 / 10 / 30 | slime, star perches | H's tube | tunnel west (z 56-58), blue gate (B3) |
| J | Rock gauntlet (brown-rock circuit) | 6-27 x 36-59 | 12 down to 6 | brown rock | I's tunnel | passage north (x 28-30) |
| K | Toy chamber | 32-50 x 11-26 | 6 up to 22 | ball blocks, star accent | J's passage | ledge west (z 14-18), tunnel to L |
| L | Basin, column ascent, monument (lower water basin, columns, ending) | 9-26 x 9-34 | 10 up to 34 | water, slime, columns, stars | K's tunnel, 12 above the east bank | green gate (G3), red arrow |

Connections in plan: A to B along the north edge; B south into C (a blue gate); C west into D (a green
gate); D's keep down by teleporter to E; E east into F; F east into G (a blue gate); G south into H;
H west into I; I west into J (a blue gate); J north-east into K; K west into L. Every connector is a
tunnel three wide and three to four high, with something in it (a gate, a drop, a view) rather than
bare length; the longest is I to J's, J to K's.

## Each place and what it holds

- **A.** A 13 x 13 shaft; four ledges (walk 88, 84, 80, 76) round it one wall each, a floor at 72. A
  shooting Blupi (33) walks each ledge's wall row; the way is the ledge's void row. Twelve flight-barrier
  cells fence the east ledge's void side; two lifts rise from the floor; three mines bob in the void.
- **B.** West bank, pool 1 (swum, exit by `up jump`), middle bank (the lollipop and a pillar four high
  with an egg), pool 2 crossed on two conveyor stones (47, they push east: jump on, run, jump off), the
  east ledge under four left fans (their winds cross the way), B1 at its north end; a stair of steps and
  three saucers up to the upper gallery (walk 82), a row of three saucers over a gap, four up fans, two
  right fans, a hanging mine over a low stretch, R1 at the west end; the blue gate. A lift to a shelf
  (walk 86) with the invert, guarded by four sleeping follow stars at its corners (they wake within
  about 2.4 cells).
- **C.** The comb: eight single-cell lifts shuttling three cells each (82 to 58), two apart round the
  void, odd ones start high, even ones low, so each is jumped to from the one before at the end of both
  dwells. The library behind the red door (R1): three aisles with a bulldozer each (the corridor at
  x 46 crosses their lanes), shelves between, R2 and G1 at the corridor's ends; two crates, a lift to a
  high shelf, five mines, bookcases along the hall's walls; the green gate (G1) to D.
- **D.** A courtyard (walk 58) with a statue, a fountain, columns and vases, two big creatures and two
  birds; a spiral of balconies each reached by a lift (west 62, south 66 with saw row 1, east 70, the
  bridge with saw row 2) to the keep: B2, teleporter 330. Behind a red door (R2) in the west wall a
  gallery with saw row 3 and G2. Teleporter 331's booth stands by the entrance (return from E).
- **E.** Arrival at the south-west terrace; lava pit P1 crossed on a crumbling clay bridge or a ferry;
  P2 crossed hanging from seven bars; the north-east terrace's lift down a shaft to a passage under a
  column of lightning (curtain timing); a ferry over P3 to a west wing (eggs, teleporter 331 back to
  D's court); the wasp's well (the balloon is the way out; hold look-up).
- **F.** A street with five houses (windows on their street faces), four birds crossing the street, a
  wall with niches (plants, columns); an alley behind a green door (G2) with a lift to the roofs, three
  birds across them, four eggs.
- **G.** Hall 1 with sleeping follow stars, up fans and two mines sweeping the way; a lava trench with
  two opposite ferries; hall 2 four lower with four left fans (winds across the way), B3, the exit.
- **H.** Three cheese caves with tubes between, a bulldozer across each cave's floor, three low mines,
  four standing mines, two helicopters, three eggs.
- **I.** A cavern: east shelf with eight helicopters in a row, three star-capped perches with a pair
  each, the floor with four big creatures, two spike traps, two drips, a stair from the east shelf for
  walking, a moving mine across the flight path; the way is a flight west to the west shelf.
- **J.** Three steps down west, a lava trench across the way north (a four-cell clay bridge or seven
  bars), two left fans, three moving and two ground mines across the north bank, then the gallery with
  two up fans at one-wide pinches and a mine above.
- **K.** Eight two-by-two ball-block columns spiralling up, one or two cells apart (a jump each), to an
  exit ledge fenced by ten flight-barrier cells; two eggs, a few star accents.
- **L.** Dropping out of K's tunnel onto the east bank, the basin (swum) to an island (palm, G3, an egg),
  six columns climbing north from the island, two star lifts, the monument terrace: the Blupi statue
  and its column, two fountains, vases; the green gate (G3) in the north wall; the compact star frame
  (floor, backdrop and gateway posts only) with the red arrow. The walls are slime.

## The key-state route (two red, three green, three blue; each door consumes the colour)

1. B holds R1 and B1 on different branches. B1 opens B's blue gate to C.
2. C: R1 opens the library's red door; R2 and G1 lie in it. G1 opens D's green gate. R2 is optional.
3. D: B2 on the keep; G2 and the red door R2 open on a gallery (optional). B2 opens G's blue gate.
4. G: B3 is taken after B2 is spent; it opens J's blue gate.
5. F's green door (G2, optional) opens the roofs. L: G3 on the island opens the last green gate. A
   player still carrying G2 at L passes with it and leaves G3 lying.

2 + 3 + 3 keys against 2 + 3 + 3 door groups. A second key of a held colour is never needed to go on.

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

Of the 36 type-16 records 27 travel and 9 stand still (D 2, E 3, H 4). `python3 tools/vwr_inspect.py
worlds3d/world199.vwr` reproduces the totals: 196 / 20 types.

## Tile hazards and decor

21 fans (10 left, 2 right, 9 up: B 10, G 7, J 4); 5 saucers 324 (B); teleporters 330 and 331 (D to E,
both pairs); 22 flight barriers 214 (12 on A's east ledge, 10 on K's exit ledge); 9 crumbling-bridge
cells 364 (E 5, J 4); 15 saws 378 and 3 switches 384 (D: the south balcony, the bridge, the gallery);
4 lightning emitters 304 and 3 strike cells 305 (E's passage); 2 spike traps 373 and 2 drips 404 (I);
14 bars 202 (E 7, J 7); lava 68 (E, G, J), water 91/92 (B, L; E has none); no crusher, spring or fish.
BigDecor, 47: palm 18 (L's island), plants 23 and 24 x 2 each (F's wall niches, L), ball machines 29 x 2
(B, G), columns 66/67/68 x 6, 7, 11 (B, C, D, F, I, L), vases 69 x 5 (D 3, L 2), monument column 87,
fountains 88 x 3 (D, L 2), Blupi statues 89 x 2 (D, L), water weed 203 x 5 (B 3, L 2). Solid ones leave
lanes of at least 0.6 cell; none stands in a route's only lane.

## Checks

`python3 tools/levelcheck.py --stage 1`: OK (the three perch eggs of I need the helicopter,
`--rides`/`flightcheck.py`). `tools/autoplay/world199.txt` plays the main route to the win screen
(28 434 ticks); `tools/autoplay/spare/world199.txt` the alternate route (about 42 000 ticks).
`VerifyShippedWorlds`, `tools/campaigncheck.py`, `tools/watercheck.py`.
