# World 199: comparison and rebuild proposal

Audit date: 2026-11-06. Repository observed on `develop`, HEAD `ba7a7d18` when recorded. This is an audit of the existing world, not an implementation or approval to rebuild it.

**Verdict:** Galaxy's finale is playable, but it substantially under-delivers the original's content, journey and finish. It replaces an extensive, densely connected succession of challenges with one plaza and eight similarly sized chambers. Restoring a few missing objects would not repair that structural problem. Rebuild the level as an authored three-dimensional journey, using the original's identity, mechanics and encounter vocabulary; do not convert, extrude or trace its map.

## 1. Scope, evidence and limits

Read the repository and parent `AGENTS.md`, `CLAUDE.md` faithful-remake rules, `CURRENT.md`, `handoff.md`, the completion plan, the world brief/generator/scripts, the reference notes, and relevant Mobile Eggbert reader/rendering code. No build, game launch, generator execution, source/world modification, commit, push, stash or reset was performed. No access to `../speedyblupi-data`. Only this report and evidence images in `build-probe/world199-compare/` were written. Existing unrelated changes were left alone.

Primary evidence:

- Committed copies: [the original's overview](../evidence/world199-original-overview.jpg), [Galaxy's floor plans](../evidence/world199-galaxy-floorplans.jpg). The full set below is in `build-probe/` (not versioned; regenerate it with the method in this section if it is gone).
- [Original full map, 6400 × 6400 PNG](../../build-probe/world199-compare/original_map.png), [1600 × 1600 overview](../../build-probe/world199-compare/original_overview.png), and [overview with coordinates](../../build-probe/world199-compare/original_coordinates.png).
- Original detail strips: [upper](../../build-probe/world199-compare/original_upper.png), [middle](../../build-probe/world199-compare/original_middle.png), [lower](../../build-probe/world199-compare/original_lower.png), [ending at full tile scale](../../build-probe/world199-compare/original_ending.png).
- Original gameplay samples: [page 1](../../build-probe/world199-compare/original_video_p1.jpg), [page 2](../../build-probe/world199-compare/original_video_p2.jpg), [page 3](../../build-probe/world199-compare/original_video_p3.jpg). Extracted from the existing local all-levels recording named in the completion plan, at one-minute intervals from 6:24:10 through 6:53:10, plus 6:53:28, :36, :41 and :45. These establish observed order and appearance, not a frame-by-frame proof of every transition.
- Copies of the supplied Galaxy play-pass sheets: [page 1](../../build-probe/world199-compare/galaxy_playpass_p1.jpg), [page 2](../../build-probe/world199-compare/galaxy_playpass_p2.jpg), [page 3](../../build-probe/world199-compare/galaxy_playpass_p3.jpg). These are existing captures, not new captures of today's dirty source tree.
- [Galaxy floor slices](../../build-probe/world199-compare/galaxy_floorplans.png), [ending comparison](../../build-probe/world199-compare/ending_comparison.jpg).
- [Galaxy generator](../../tools/worlds/world199.py), [brief](../briefs/world199.md), [main playthrough](../../tools/autoplay/world199.txt), [the old spare route](../../tools/autoplay/spare/world199.txt) (replaced 2026-11-06 by the alternate route of the rebuild), and supplied trace `build-probe/play-pass-run-1/world199/autoplay_trace.txt`.
- Original data: `../mobile-eggbert/worlds/world199.txt`, parsed using [parse_reference](../../tools/world_inventory.py). Galaxy data: read with `PYTHONDONTWRITEBYTECODE=1 python3 tools/vwr_inspect.py worlds3d/world199.vwr` and its reader. Object identities checked against [object notes](../../mobile-eggbert-reference/03-objects.md), current definitions and the stored channels/icons.

### Map-rendering method and corrections to old notes

This is a **static sprite atlas of the authored map**, not a simulated screenshot. Each map cell is 64 px. The file is column-major: line index is X, comma index is Y, as `Worlds::GetDecorField` and `parse_reference` implement. Some old format notes describe it as row-major; following that text would transpose the map.

Decor uses `Content/icons/object-m.png`: 20 columns, 64 px crops, 65 px stride, 1 px leading margin. BigDecor is drawn first from `explo.png`, using its 144 px source grid and the original icon sizes (128 px normally here; 144 for 66–68), with the original +2 px baseline and nudges for 66–68 and 87–89. **The five BigDecor 203 placements are the original renderer's exception: water weed uses `object-m`, not `explo`.** See Mobile Eggbert `Decor::Build` around lines 717–747 and `Pixmap::DrawIcon` around 548–559. The old world-format note incorrectly says BigDecor generally uses object-m.

MoveObjects use the file's actual `channel` and `icon`: channel 1 → object-m (including lifts/crates), 10 → element (60 px, ten columns), 11–13 → blupi1 (60 px, ten columns). Objects are drawn at `posStart`, with their stored icon frame; the player is shown at `blupiPos` using idle frame 0. This intentionally does not mix saved `posCurrent` offsets into a start-position inventory. Static Decor includes authored flight-barrier outlines even though ordinary gameplay hides them. A neutral dark background replaces scrolling parallax; animation, transient shots, runtime tinting and runtime foreground interleaving are not simulated. No source sprite-sheet copies or renderer script were saved.

World hashes, to distinguish this audit from concurrent work:

| Input | SHA-256 |
|---|---|
| Mobile world199.txt | `c63e19eb711b6993be6983bca1353b483c4253fbf3b7d545ab7db85b7248027a` |
| Galaxy world199.vwr | `9ef36e20b6eeba05b12b2da3175d1eeeea56a94a40cb1e76d867a39512e26f14` |
| Galaxy world199.py | `11e7ad509b4f0e9a33f470c27d1da80045443e2888365d09887c43ab83845399` |
| Main autoplay world199.txt | `ab5b776f3e343870f24252924db7e8e47ceda91fba09b87ff3698589f352e3bf` |

## 2. What the original actually is

### Size and identity

The bonus finale behind the main hub's twelve gold bars occupies a **100 × 100 cell map**, with authored Decor reaching all four edges: **7,425 occupied Decor cells, 177 distinct Decor icons, 196 MoveObjects of 20 types, 47 BigDecor placements**. Header: region 31, music 1, spawn `(4482,6)` pixels, approximately cell `(70,0)`, facing right. The red arrow is at `(898,3908)` pixels, approximately `(14,61)`. Coordinates below are X right / Y down, zero-based.

It is a slime labyrinth containing substantial pieces of earlier worlds: the white palace, brick library, blue machine room, grey riveted station, a village street, cheese passages, brown rock and toy/star architecture. The slime is shaped into shafts, thin ledges, overhangs, caves and connectors; it is not merely a texture enclosing a central lobby. Mechanical rooms connect to other regions at different levels. Palms/plants, columns, vases, fountains, statues and machines give destinations visual identity.

There is **no treasure quota**: zero type-5 treasure, one normal exit, no secret exit. Thirty-eight eggs supply lives, rather than forming a mandatory collect-all objective. Duplicate coloured keys enable multiple gates/routes, but are not an unlimited permission flag: opening a door consumes its colour, and another key of a currently held colour cannot simply be stockpiled.

### Regions, observed order and branching

The following is an ordered reading of the recorded journey, with map-derived branches explicitly identified. It is **not a claim that every player must visit every region in this order**. Difficulty assessments describe the spatial/mechanical demands, not a measured player study.

| Order / approximate bounds | What the player encounters and must do | Route role and difficulty |
|---|---|---|
| 1. Start shaft and gun ledges, X63–76 / Y0–14 | Four enemy Blupis occupy staggered slime shelves around the initial descent; a long vertical lift runs at X70 between Y1 and Y11. Shot lanes, landings and the lift are already combined. | Immediate exposure and decisions about descent/return, not a safe sightseeing plaza. Visible at 6:24:10. |
| 2. Upper-right basin, star/lift passages and fan tower, X77–99 / Y0–28 | Water pockets, narrow slime/star passages, five vanishing platforms, two type-47 conveyor platforms, a star lift, four follow stars near X81–82/Y15–16, keys in upper alcoves, and a metal chamber of directional fans/wind. The lollipop is near (96,25), invert near (79,19). | Multiple upper routes and rewards. Timed footholds, swimming, restricted clearances, pursuit and wind interact. Samples 6:25–6:29 show star/grille passages, water and the fan room. Exact visits to every small platform are not established by these minute samples. |
| 3. Central lift shafts and blue-machine crossing, X15–72 / Y0–32 | The X50–52 shaft has eight short alternating lift records; the X56 shaft adds another vertical lift. Across the lower upper-map corridor, mines and fans lead to a blue machine cavern: mines within its open interior, emitters along its edge and a lightning column at (34,22–24). Beside it, a dead-end well has the wasp at Y31 and a moving mine across Y26. | Interlocking horizontal and vertical routes. The lift comb is a substantial timing sequence, not one elevator. The well's sting/balloon escape must be read together with the mine overhead. Machine edge visible at 6:30:10; entering the well is not proven by the sampled video. |
| 4. West key routes, palace and brick library, X0–46 / Y0–25 | The palace has stacked white balconies, saw rows, switches, pillars, railings, cobwebs, mines, birds, lifts and a statue recess. The adjacent brick interior mixes shelves, two crates, saws, moving platforms, bulldozers and key ledges. One library lift has asymmetric advance/recede values 40/176. Red/green/blue doors also stand together near (2–4,2). | A multi-storey challenge and key-distribution network. Teleporters connect (1,10)↔(1,22) and (1,1)↔(2,29). The recorded player traverses the palace and library, then returns through palace/teleport areas, 6:31–6:37. Mines above and below footholds constrain jumps and crouching; switching saws is one tool among several. |
| 5. West lava terraces, X0–20 / Y29–50 | Several long lift crossings above lava; narrow ledges and pillars divide the spaces; a crumbling bridge and clustered type-16 mines complicate the lower crossing. | Increasing commitment: a missed transfer falls into lava. These are consecutive traversals with different spans/heights, seen at 6:37–6:38, not a single room entered and immediately retraced. |
| 6. Village street and roofs, X21–53 / Y39–49 | A long run of recognisable multi-storey houses, arches, windows, slate/dormer roofs, eggs and multiple birds at different heights; a vertical lift near X24. Seven of the nine birds start in this street band, the other two in the upper-west area. | Land on and move between roofs while reading patrols above/beside them. The 6:39:10 sample shows the street. The timeline records a bird death later in that minute; this audit did not re-extract the exact death instant. |
| 7. Grey-metal station, X53–99 / Y30–50 | Riveted enclosures, grilles/girders, a five-star group near X61–65/Y45, further stars across the eastern room, lift components around X68–69/Y46, four moving mines in adjacent vertical lanes, lava and more directional wind. | Pursuit, transfer timing and environmental hazards overlap. A distinct industrial region, not just a blue room. Seen around 6:40:10. |
| 8. Cheese tunnels, X77–99 / Y51–71 | Irregular cheese passages and constricted upper/lower lanes; three short bulldozer patrols at Y57, moving and hanging mines around Y61–65, fans along the lower floor and eggs at the far side. | Clearance matters: mines overhead, moving enemies on the route and wind/fan mouths below. 6:41–6:42 show this pressure. It is not simply a cheese-coloured lava bridge. |
| 9. Helicopter/slime cavern network, X17–77 / Y51–79 | The map's helicopter supply is concentrated here and near the final approach: **22 in total**, including a row at Y57 and replacements through lower chambers. Four large creatures are in this lower network; the other two are in the upper-west slime room. Star footholds, two spike traps, two drips, plants/columns and narrow openings punctuate the caves. | Repeated board/land/leave choices, ceiling clearance, routes around creatures and optional egg niches. Some helicopters are alternatives or replacements; 22 placements do not mean 22 compulsory flights. Recorded views at 6:43–6:46 and 6:48 show flight and on-foot transitions. |
| 10. Brown-rock circuit, X45–99 / Y72–99 | Uneven rock tunnels connect to a lower sequence of lava pockets, five moving mines, opposing/vertical wind situations and overhead bars. | Late combined traversal: jump timing, headroom, hanging and wind over deadly floors. Recorded at 6:47, 6:49 and 6:50. The alternation with slime confirms a circuit, not a rigid material-by-material checklist. |
| 11. Toy chamber, X34–44 / Y80–96 | A tall framed field of ball blocks and isolated footholds. Ten of the map's 22 flight-barrier cells are distributed among these footholds; the other twelve are near the start. | Precision and route/vehicle choice late in the journey. The 6:51:10 sample shows its interior. Barriers block foot/ground vehicles and allow helicopter/overcraft; there is no placed overcraft in this world. Do not mistake the atlas's visible barrier outlines for normal in-game scenery. |
| 12. Lower water basin, column/star climb and monument, X0–33 / Y58–99 | The lower-left pool and planted island lead into an extensive ascent among columns, star blocks and the two short star lifts at Y88. The route returns to the upper final pocket: green key around (2,59), statue and fountains around X7–9/Y58–60, then the green door at (13,60) and red arrow around (14,61), enclosed by a compact star-block frame. | Final ascent and recognisable arrival. Swimming at 6:52, column/star climbing at 6:53:10 and :28, statue at :36, arrow/frame at :41, Win at :45. The ending is an architectural reveal and tribute, not merely an exit object. |

**Topology:** the initial upper network offers keys, mechanisms and detours; the western teleporter pairs link different heights; the palace/library and slime shafts cross-connect; the village and metal station lead into the large lower cavern circuit; rock/toy/water/column passages provide the late journey back toward the exit pocket. The final pocket is spatially near earlier west-side routes, but separated by terrain and its gate. Map adjacency alone is not proof that a shortcut is playable; a complete key-state route graph would require a dedicated reference playthrough, beyond these samples.

The reference footage spends approximately 29 minutes 39 seconds in 199 (6:24:04.75–6:53:44.20). That includes exploration, pauses/revisits and a death; it is not a speedrun target or a calibrated comparison to Galaxy autoplay.

## 3. Exact object and gate comparison

Counts below are **placed records**, not runtime-generated bullets, effects, door animations or composite-platform counts. “Missing” means absent from this level, not absent from Galaxy's engine.

| Type | Object | Mobile | Galaxy | Shortfall |
|---:|---|---:|---:|---:|
| 1 | Ordinary lift tile | 26 | 14 | 12 |
| 2 | Ground mine | 9 | 2 | 7 |
| 3 | Hanging mine | 9 | 0 | 9 |
| 4 | Bulldozer | 6 | 2 | 4 |
| 6 | Egg | 38 | 30 | 8 |
| 7 | Red-arrow exit | 1 | 1 | 0 |
| 12 | Crate | 2 | 1 | 1 |
| 13 | Helicopter | 22 | 6 | 16 |
| 16 | Moving-mine type | 36 | 9 | 27 |
| 20 | Bird | 9 | 5 | 4 |
| 26 | Lollipop | 1 | 1 | 0 |
| 33 | Enemy Blupi / blupit | 4 | 1 | 3 |
| 40 | Invert | 2 | 1 | 1 |
| 44 | Wasp | 1 | 1 | 0 |
| 47 | Conveyor platform | 2 | 0 | 2 |
| 49 | Red key | 2 | 2 | 0 |
| 50 | Green key | 3 | 3 | 0 |
| 51 | Blue key | 3 | 3 | 0 |
| 54 | Large creature | 6 | 3 | 3 |
| 96 | Follow star | 14 | 10 | 4 |
| **Total** | **20 original types; 18 Galaxy types** | **196** | **95** | **101** |

Nine of the original's 36 type-16 records have identical start/end positions: **27 actually travel and 9 are stationary**, despite the type's usual name. All nine Galaxy type-16 records travel. Across ground mines, hanging mines and type 16, the total is **54 → 11**. Across types 2/3/4/16/20/33/44/54/96, enemies/hazard creatures total **94 → 33**.

Both worlds have zero treasure, secret exits, jeep, tank, skateboard, overcraft, fish, robot type 32, dynamite, drink, charge, shield pickup, tentacle and type-48 conveyor placements. Do not add those to an “all mechanics” finale merely because the engine supports them. Runtime bullets from type 33 are not authored object records.

### Lifts: the reduction is worse than the record count suggests

Mobile has 26 ordinary lift records plus two conveyor records. The ordinary lifts include eight alternating short transfers in one shaft, long vertical and horizontal journeys, two small palace platforms, a village lift, differently skinned platforms and the late pair of star lifts. Two side-by-side pairs account for four records; they should not be sold as four independent journeys.

Galaxy's 14 lift records form just **three synchronised platforms**: two 2 × 2 plaza elevators (four records each) and a 3 × 2 machine-room shuttle (six records). Its “14 lifts” is not fourteen lift encounters. It has no conveyor platform. The current code is capable of real height changes, but the authored lift itinerary has been collapsed.

### Keys and doors: counts match, purpose does not

| Colour | Mobile key count | Mobile doors and cells | Galaxy keys | Galaxy door groups / raw cells |
|---|---:|---|---:|---|
| Red | 2 | 2: (2,2), (84,1) | 2 | 2 groups / 12 cells |
| Green | 3 | 3: (3,2), (13,60), (85,1) | 3 | 3 groups / 18 cells |
| Blue | 3 | 3: (2,23), (4,2), (85,27) | 3 | 3 groups / 18 cells |

Galaxy has **eight** connected 3-wide × 2-high door groups, each consuming one key, not 48 independently keyed doors. Its blue groups are the palace, outer finale and inner finale. The brief's “four blue doors” and its statement that duplicates are simply spare key flags are stale/misleading; the current scripts correctly spend keys on individual doors. The original's terminal gate is **green**, while Galaxy makes the finale a **two-blue-door** errand. Matching eight keys does not preserve the original routing or ending identity.

## 4. Tile hazards, architectural vocabulary and decor

Raw Decor cells in a 2D map and voxels in a 3D world measure different things. The following counts diagnose presence, diversity and repeated encounter opportunities; a bigger filled volume is not automatically more gameplay. Animation frames are not separate traps. In particular **347 is ordinary slime here; only 373 is the spike trap**.

| Feature / icons | Mobile cells | Galaxy cells | Meaning of difference |
|---|---:|---:|---|
| Lava 68 | 60 | 240 | More voxel fill does not restore the successive lava crossings. |
| Water 91/92 | 83 + 29 | 288 + 72 | Original distributes water among upper passages and final basin; Galaxy has one 9 × 8 pool, five cells deep. |
| Fans left/right/up 126/129/132 | 10 / 2 / 9 | 1 / 0 / 0 | Twenty fans gone; directional variety gone. |
| Wind left/right/up 110/114/118 | 108 / 18 / 35 | 4 / 0 / 0 | 161 authored wind cells become one four-cell westward lane. |
| Bars 138/202 | 11 + 3 | 0 + 7 | One machine-pit bar crossing replaces several placements/contexts. |
| Lightning emitters / strike cells 304/305 | 4 / 3 | 3 / 9 | Galaxy retains lightning as one broad entrance curtain; counts are not independent cycles. |
| Vanishing saucers 324 | 5 | 2 | A short climb replaces a richer upper-shaft sequence. |
| Teleporters 330/331 | 2 + 2 | 2 + 2 | Two pairs retained, but both now connect the same plaza's floor/ring. |
| Flight barriers 214 | 22 | 0 | A real level mechanic missing; affects foot-versus-flight routing. |
| Crumbling bridge 364 | 9 | 9 | Same raw count, different design: Galaxy packs it into one 3 × 3 crossing. |
| Spike traps 373 | 2 | 2 | Count retained; both now sit on plaza circulation routes. |
| Saws 378 / switches 384 | 15 / 3 | 24 / 2 | Extra blades packed into two identical rows do not preserve three switch contexts and mixed palace/library layouts. |
| Drips 404 | 2 | 2 | Original separated cavern ledges become two drips over one village passage. |
| Crushers / springs | 0 / 0 | 0 / 0 | Correctly absent. The old brief still mentions crushers in its staging description. |

Decor-family inventory (ranges are explicit so the totals can be reproduced):

| Family / icons | Mobile cells | Galaxy voxels |
|---|---:|---:|
| Blue panel vocabulary 1–31 | 237 | 767 |
| Stone masonry 35–39 | 249 | 663 |
| Small machine props 48–67 | 32 | 0 |
| Stone column 76 | 45 | 0 |
| Grey riveted metal 78–84 | 895 | 0 |
| Mechanical pieces 87–90 | 5 | 0 |
| Shelves/cupboards 139–143 | 17 | 312 |
| Cobble/stone family 144–156 | 249 | 170 |
| House/window/roof vocabulary 185–198 | 95 | 132 |
| Grille/girder 200/201 | 65 | 0 |
| Ball/toy blocks 215–230 | 81 | 721 |
| Star blocks 231 | 64 | 1,311 |
| Striped poles 235/236 | 29 | 0 |
| Cheese 246–249 | 349 | 977 |
| Brick and its edge vocabulary 261–283 | 238 | 971 |
| Brown rock and edge vocabulary 284–303 | 963 | 801 |
| Slime 341–363 | 3,077 | 358,887 |
| White palace stone/trim 386–397 | 219 | 807 |
| Balustrade 398 / fluted column 399 | 14 / 26 | 0 / 0 |
| Cobwebs 401–403 | 10 | 0 |

These rows deliberately do not classify every decorative icon as a mechanic. Additional original-only authored icons include 337 (7 cells), 376 (2) and 410 (8); restore their visual roles only after checking the current definitions and sprites. Do not invent a new hazard for them. The complete per-icon census is appended below.

### BigDecor: all 47 placements missing

| Explo icon, except noted | Family | Mobile | Galaxy |
|---|---|---:|---:|
| 18 | Palm | 1 | 0 |
| 23 / 24 | Leafy plants | 2 / 2 | 0 / 0 |
| 29 | Ball machine | 2 | 0 |
| 66 / 67 / 68 | Large stone-column variants | 6 / 7 / 11 | 0 / 0 / 0 |
| 69 | Vase | 5 | 0 |
| 87 | Monument column | 1 | 0 |
| 88 | Fountain | 3 | 0 |
| 89 | Blupi statue | 2 | 0 |
| 203, object-m exception | Water weed | 5 | 0 |
| **Total** | | **47** | **0** |

One statue is in the upper palace; the other is at the ending. The ending's statue, column and two fountains are a specific missing composition, not interchangeable decoration that can be replaced by scattering extra foliage.

## 5. What Galaxy currently asks the player to do

Native inspector: mission 199, sky 31, music 1, spawn `(65,30,35)`; **367,236 non-air blocks, 89 block types, 43 occupied Y levels, bounds X2–97/Y2–44/Z2–97; 95 MoveObjects; zero BigDecor**. “43 heights” mostly measures the solid enclosing mass; it is not 43 playable floors. Slime is 97.7% of all stored blocks.

The actual design is a 33 × 33 plaza (X/Z34–66), walking floor Y20, three-cell-wide ring at Y30, a stair, two grouped elevators and two floor↔ring teleporter pairs. Eight chambers all use **13 × 13 interiors**, shells 15 × 15, and three-wide entry passages; most have seven cells of interior air height, the library/toys ten. Their repeated shell is explicit in `Room` and `enclave`.

| Region | Contents and player task | Route role |
|---|---|---|
| Plaza / ring | 8 eggs, 4 helicopters, 2 bulldozers, 3 birds, 4 bobbing mines, 4 follow stars; two platform assemblies; one pool; two spike traps. Side well has wasp + ninth moving mine; step/saucer climb uses two saucers. | Recurring transit space; the same slime backdrop connects nearly everything. The well is a genuine mechanic, but optional on the exit dependency chain. |
| East machine, lower | 2 eggs, 2 helicopters, red key, 2 moving mines, one six-tile lift assembly. Lightning entrance, lava trench, bars for return, one west-blowing fan. | Strongest combined challenge in the current main route. Cross, take key/eggs, retrace. |
| North library, lower | 3 eggs, green key, one crate, three full-width bookcase steps rising to Y26. | Red gate; a largely unopposed climb. Shelves read as a tiled staircase rather than a furnished library. |
| West village, lower | 2 eggs, blue key, 2 ground mines, one shooting Blupi, small house masses; two drips over the entrance. | Green gate; walk around the shooting lane to the back of a square room, then return. Main script explicitly times the shots. |
| South cheese, lower | 2 eggs, green + blue keys, 2 moving mines, 3 × 3 crumbling bridge over one lava trench. | Green gate; cross, gather keys, cross back. No cheese-tunnel maze, overhead hanging mines or fan floor. |
| East toys, upper | 3 eggs, red key, lollipop, 6 follow stars, one four-high ball/star tower. | Red gate; power jump to the key. Optional in main pass, exercised in spare script. |
| North palace, upper | 3 eggs, green key, 2 birds, two 12-saw rows and two switches. | Blue gate; two similar floor obstacles. Optional in main pass. Spare script jumps the rows, so that script alone does not prove meaningful switch use. |
| West rock, upper | 2 eggs, blue key, invert, 3 large creatures, two lava patches. | Green gate; a room of patrols. Optional in main pass; no long rock route or late bar/wind gauntlet. |
| South finale, upper | 5 eggs, outer and inner blue gates, red arrow in a star-block frame; entire shell also uses star blocks. | Two blue keys and a final short approach. Win screen retained. Monument and long column/star ascent absent. |

**Main script order:** start/ring → machine/red key → optional wasp well → library/red door/green key → west/south plaza circulation → cheese/green door/blue and green keys → saucer climb → open outer blue finale gate → teleporter down → village/green door/second blue key → teleporter up → inner blue gate → arrow/Win. It walks past many vehicles and never establishes a helicopter journey as a required test. Toys, palace and rock are not visited.

**Spare script order:** machine → upper toys/lollipop/second red key → library → rock/blue key → palace/green key → cheese → remainder of main exit route. This is useful coverage, but two scripted routes do not turn the rooms into a sustained finale. The supplied main trace reaches Win at tick 16,488 and passes at 16,608. It proves that route completed in that capture, not that the original's scale or difficulty has been matched.

### Player-height visual evidence

- **p1 t360, t3240, t4320–5040; p2 t7920–8280, t10440–11520:** large repetitive slime planes, broad flat paving, long straight edges and recurring square openings. The image is dominated by enclosing material instead of landmarks or the next challenge.
- **p1 t1080–2880:** the machine has real lightning, lava, lift, bar and fan content. Nevertheless, its flat blue banks and panel walls form a simple rectangular exercise; repeated panel tiling is the dominant composition.
- **p1 t5760–6480:** bare brick wall ahead, bookcase textures forming stairs underfoot. It reads as an unfinished climb blockout, not the original's multi-level brick/shelf/lift/saw/crate interior.
- **p2 t8640–10080:** nearly uniform cheese walls and banks around one rectangular trench. The original cheese is perforated, constricted and threaded with enemies/fans; the shared yellow material alone does not supply that identity.
- **p2 t13320–14400 and p3 t14760:** high grey walls/corners and flat cobble dominate the village route. The houses exist in source, but the captured route fails to present a long, recognisable roof-and-facade street.
- **p3 t16200:** walls and floor are saturated in the same star pattern. This weakens the exit frame's visual hierarchy. The original reserves the compact frame as a landmark beside the statue and fountains. The next sampled frame is already Win; the contact sheet alone does not provide a good close view of Galaxy's actual arrow.

The supplied sheets do not show the interiors of toys, palace and rock. Their structural assessment above comes from generator/native data and the spare script; this audit makes no claim to fresh visual verification of those rooms. Extra live runs were unnecessary to establish the central gaps and were avoided on the shared machine.

## 6. Honest gap report

1. **The journey has been replaced with a hub exercise.** The original makes the player traverse and revisit a succession of substantial regions; Galaxy repeatedly returns to one square plaza and its ring.
2. **Half the authored object content is gone:** 196 → 95 records, including 38 → 30 eggs. The number of stored voxels conceals this reduction rather than compensating for it.
3. **The lift finale has nearly vanished.** Twenty-six original ordinary lift tiles with many separate transfers become three grouped platforms; both conveyor platforms disappear.
4. **Flight has become scenery/resources to walk past:** 22 → 6 helicopters, with no authored flight-barrier cells and no extended lower helicopter cavern network.
5. **Mine variety and pressure collapse:** 54 → 11 mine-family records; all nine hanging mines are missing, eliminating their crouch/clearance combinations.
6. **Creature encounters are thinned and relocated:** bulldozers 6 → 2, birds 9 → 5, enemy Blupis 4 → 1, large creatures 6 → 3, follow stars 14 → 10. The gunfire descent and rooftop bird sequence no longer exist as such.
7. **The grey-metal station is missing outright:** 895 original grey-metal Decor cells → zero, along with its grilles, fan pressure, lifts and mine/star lanes as a combined setting.
8. **Fans are tokenised:** 21 → 1, with rightward and upward fans absent. One wind lane cannot stand in for the original's opposing and vertical pressures.
9. **Repeated rooms substitute for distinct places.** Eight 13 × 13 interiors share the same doorway/shell grammar. Changing material and one obstacle does not make them eight developed regions.
10. **Several surviving counts hide lost combinations:** bridge cells and spikes match, but distributed encounters have been compressed into one crossing or one circulation loop. More saw cells make two repeated rows, not a richer palace.
11. **The visual omissions are large:** 47 → 0 BigDecor, plus absent columns, balustrades, grilles, poles and cobwebs. Bare walls and flat floors are supported by both the data and the player-height sheets.
12. **The difficulty curve is fragmented:** a demanding machine room is followed by a simple shelf climb and repeated returns; major optional rooms can be skipped on the main route. The original sustains combinations and culminates in rock, toy, water and column traversal.
13. **Key quantity survives but key dramaturgy changes:** the original final green gate becomes two blue gates plus a plaza errand. The world brief overstates door count and understates key consumption.
14. **The finale lacks its monument:** statue, fountains, long column/star approach and controlled star-frame reveal are gone. Wallpapering a chamber with stars is a weak substitute.
15. **Passing checks has been mistaken for completion.** Reachability and a Win trace show a working route. They do not establish faithful density, encounter composition, visual finish or a convincing climax. This level needs structural reauthoring, not another “all lesson types present” tick.

## 7. Concrete hand-authored 3D rebuild plan

### Authoring contract

Keep mission 199, region 31, music 1, zero treasure quota and the existing Win behavior. Design the rooms and terrain directly as original X/Y/Z cell volumes in Galaxy; **no import/conversion of original cell positions, no extrusion, no fixed-depth corridor masquerading as 3D**. Use the original map only to establish themes, objects, mechanics, encounter relationships and the ending's identity.

Replace the plaza/enclave topology. The route should descend, cross, loop around, enter below an earlier overlook, fly through a cavern, descend into rock and finally climb to the monument. Each substantial section needs movement in both horizontal axes, meaningful height changes and at least one view of a later or earlier part. Challenge lanes can be narrow where a mine, shot or fan demands it, but their surrounding architecture must be volumetric. Long bare stretches must earn their length by anticipation, recovery or a view; they cannot serve merely to separate pickups.

The following is a **proposed allocation**, not a reproduction of the 2D coordinates. Sizes are rough interior envelopes in cells, **X × Z × height**. Y ranges indicate intended vertical occupation, not continuous solid fill. The section ledger in the next table is exact and sums to the original's 196 objects.

| Section, intended order | Theme, rough envelope and 3D structure | What it tests / original identity to retain |
|---|---|---|
| A. Gunfire descent | Slime; 18 × 18 × 22, Y62–84. Shelves wrap three sides of a shaft, with offset cover and two lift journeys. | Four shooting Blupis, timed descents and visible safe landings. Preserve the start's hostile ledges; author shot crossings that remain relevant when the player can move in Z. |
| B. Upper basin and fan switchyard | Slime, stars, grey metal; 26 × 24 × 20, Y58–78. Two water pockets at different heights, an annular ledge with cross-links, five saucers and both conveyor platforms on distinct transfers. | Swimming plus timed platforms, pursuit and wind. Put left/right/up fan situations here, not one large flat fan floor. Lollipop and invert have source-grounded traversal contexts. Red and blue key niches visible before commitment. |
| C. Lift comb and brick library | Brick, cupboards, slime; 26 × 22 × 24, Y58–82. Nine lift records: staggered transfers around a vertical void, connected by shelf galleries crossing in X and Z. | The original long lift sequence, asymmetric lift timing where appropriate, crate handling, bulldozer timing, shelf/saw interactions and key branches. Shelves are furniture/ledges with readable fronts, not wall-to-wall floor texture. |
| D. Palace and west key galleries | White stone; 24 × 26 × 22, Y50–72. Balconies around a courtyard, a statue recess, transverse bridges, an undercroft and a return overlook. | Saws/switches, ground/hanging mines, birds and two large creatures occupy different but connected lanes. Two teleporter pairs link meaningful west-side routes, not two points on one hub. Spend a key before exposing a replacement of the same colour. |
| E. Lava terraces, machine edge and wasp well | Slime/blue panel; 22 × 24 × 18, Y22–40. Three distinct lift transfers cross terraced lava voids; a side well has a mine over the wasp's escape route. | Lift timing above lava, crumbling bridge recovery, lightning near a landing, bars with a safe dismount, and the original wasp/balloon escape. The well remains an optional trap/lesson; do not invent an egg obligation inside it. |
| F. Village roofs | Cobble, facades, slate; 30 × 18 × 14, Y42–56. A bent street with two rows of individually shaped houses, roofs at several heights, an alley under a roof bridge and one village lift. | Seven birds create readable roof-transfer timings rather than random open-air danger. Street and roof routes reconnect; an optional green-gated roof loop adds risk/reward without blocking the ending. |
| G. Riveted station | Grey metal, girders, grilles; 30 × 20 × 16, Y42–58. Two offset halls around a lava service trench, paired lift tiles, narrow transfer landings and alternate-height galleries. | Ten follow stars, four moving mines, lift transfer and directional wind combine. Preserve pursuit and timing, but design cover/lanes so a generic sidestep cannot erase the encounter. Blue key on the far-side route, after spending the previous blue key. |
| H. Cheese passages | Cheese; 26 × 22 × 18, Y20–38. Intersecting rounded chambers and low passages, an upper enemy lane and a lower fan lane, linked in more than one horizontal direction. | Three bulldozers, hanging mines, moving mines and two helicopters create clearance/vehicle decisions. This must look and play like perforated cheese caverns, not a rectangular trench room. |
| I. Helicopter jungle | Slime/star ledges; 36 × 32 × 28, Y10–38. Two linked flight volumes with offset ceilings, natural-looking tile overhangs, landing shelves, a high crossing and low pedestrian returns. | Eighteen helicopters distributed as purposeful starting/replacement craft, four large creatures, two separated spike encounters and two separated drip ledges. The original's sustained flight-and-landing sequence becomes a major late section. No new power or enemy type. |
| J. Rock gauntlet | Brown rock; 32 × 24 × 16, Y6–22. A descending switchback opens onto a lava trench; bars turn the route toward an ascending side passage and fan gallery. | Five moving mines, ground mines, lava, mixed wind and hanging with restricted headroom. A demanding combination late in the level; each lethal section has a readable safe approach, not an unseen first-person collision. |
| K. Toy chamber | Ball blocks with a deliberately limited star accent; 14 × 18 × 22, Y8–30. A tall freestanding lattice of footholds, several X/Z approach angles and flight-filtered openings. | Precise transfers and vehicle/foot routing. Retain ten flight-barrier cells as deliberate filters; no arbitrary new lock mechanic. The other twelve barrier cells belong to A's start/return boundaries. |
| L. Basin, column ascent and final monument | Water, slime, columns, restrained star blocks; 20 × 28 × 30, Y6–36. Swim through a basin, climb between freestanding columns and two short star lifts, arrive on a terrace overlooking travelled space. | Last green key, readable green gate, compact star frame and red arrow. Show a Blupi statue with its column and two fountains before the final approach. Two nearby helicopters are route resources, not a way to bypass the final gate. |

Spatial fit proposal within the existing 100³ format: A in the upper northwest; B upper northeast; C above the west-middle; D upper southwest; F across the centre; G above the southeast; H southeast below G; I the lower central volume; J below the east side; K low north-centre; L low northwest. Example non-overlapping principal X/Z envelopes: A 6–23/6–23; B 58–83/6–29; C 6–31/30–51; D 6–29/56–81; E 6–27/36–59; F 32–61/30–47; G 64–93/52–71; H 66–91/74–95; I 28–63/50–81; J 66–97/40–63; K 42–55/6–23; L 6–25/6–33. Vertical separation permits overlapping projections. Reserve authored connectors and camera clearance in the gaps; prove the connections before filling or decorating shells. This is a packing sketch, not permission to retain these rectangles as the finished visible room shapes.

### Reconciled object allocation

Abbreviations: L ordinary lift **records**, C47 conveyor records, M2/M3/M16 mine types, B4 bulldozer, B20 bird, B33 enemy Blupi, C54 large creature, S96 follow star. H13 helicopter. Blank cells mean zero. These are budgets for spatially purposeful encounters, not permission to park unused objects in a store room.

| Section | Eggs | L | C47 | H13 | M2 | M3 | M16 | B4 | B20 | B33 | C54 | S96 | Other pickups/objects |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| A | 2 | 2 | | | | | 3 | | | 4 | | | |
| B | 4 | 3 | 2 | | 1 | 1 | 3 | | | | | 4 | Lollipop 1; invert 1; red key 1; blue key 1 |
| C | 5 | 9 | | | | | 5 | 3 | | | | | Crates 2; red key 1; green key 1 |
| D | 4 | 4 | | | 5 | 4 | 6 | | 2 | | 2 | | Invert 1; green key 1; blue key 1 |
| E | 2 | 3 | | | 1 | 1 | 5 | | | | | | Wasp 1 |
| F | 6 | 1 | | | | | | | 7 | | | | |
| G | | 2 | | | | | 4 | | | | | 10 | Blue key 1 |
| H | 3 | | | 2 | | 3 | 4 | 3 | | | | | |
| I | 6 | | | 18 | | | 1 | | | | 4 | | |
| J | 2 | | | | 2 | | 5 | | | | | | |
| K | 2 | | | | | | | | | | | | |
| L | 2 | 2 | | 2 | | | | | | | | | Green key 1; red-arrow exit 1 |
| **Total** | **38** | **26** | **2** | **22** | **9** | **9** | **36** | **6** | **9** | **4** | **6** | **14** | **2 crates, 1 lollipop, 2 inverts, 1 wasp, 8 keys, 1 exit** |

Of M16's 36 placements, preserve **27 travelling / 9 stationary**; proposed stationary allocation D2, E3, H4, with the remaining 27 travelling. Ordinary lift records must retain many independent trajectories: allow the two original-style paired decks, but do not spend the budget on three oversized platforms. Target 24 independent ordinary lift paths from 26 records, plus two conveyor encounters. Verify the resulting single-tile transfers live before accepting this exact deck allocation in 3D.

**Decor budget:** restore all 47 BigDecor placements by family from §4, using existing approved assets. Reserve one statue + one fountain for the palace, and one statue + two fountains + the monument column for the end. Place the palm and four plants at pools/landing shelves, the two ball machines at machine landmarks, the five water weeds in actual water, and the 24 large columns/five vases where they define architecture and sightlines. Model collision and camera clearance must be checked, not assumed.

**Hazard/feature budget:** 21 fans (10 left, 2 right, 9 up), distributed B/G/J as 10/7/4 devices; preserve the original wind-direction variety and document every device's effective air lane. Start with the 161 original wind-cell budget as an audit comparison, then justify any 3D volume changes by interaction, not by raw voxel equality. Five vanishing saucers in B; two teleporter pairs across D/E return routes; 22 flight-barrier cells in A/K; nine bridge cells over separate E/J crossings (provisional split 5/4); 15 saw cells with three independently meaningful switch contexts in C/D; four lightning emitters and a three-cell strike column around E's machine approach; two spikes and two drips separated across I. Restore bars in both the machine/lava and rock sequences. Water and lava volumes must fit safe swimming exits and actual crossings; do not require equality of fill volume. No crushers, springs, fish, treasure or extra vehicle types.

### Key-state route, branches and ending

Use **two red, three green, three blue door groups** and exactly matching key supplies. Suggested dependency sequence:

1. B exposes red R1 and blue B1 in separate challenge branches. B1 opens the upper route toward C; R1 opens C's library interior. No same-colour duplicate is forced into the inventory.
2. C supplies R2 and G1. G1 opens D. R2 opens an optional palace gallery loop, with an independently available return, so visiting it does not consume a key needed for the main ending.
3. D supplies G2 and B2. G2 opens F's optional roof route; the street remains an alternative. B2 opens G's station route. This spreads key decisions over the journey instead of placing several gates in a final box.
4. G supplies B3 only after B2 has been spent. B3 opens the late rock route to J, reached through H/I. No gate can be flown over or entered from its rear unless that is the documented alternate route.
5. L supplies G3 after the basin/column ascent. A player still carrying optional G2 may use it at the finale instead; G3 remains a valid replacement. Open **one final green gate**, show the compact star-block frame and red arrow, then use the existing Win transition. The final statue/column/two-fountain group should be visible from the approach and again beside the frame.

This is a proposed new 3D dependency graph, not a claim that it reproduces the original exact solution. Confirm its eight consumptions and the optional-loop cases with the flag-based key rules before authoring finish detail. Each branch must reconnect or clearly return to a decision point. Keep optional eggs and alternate craft optional. Do not make all 38 eggs mandatory, add a boss, add treasure bars inside 199, or invent a new ending mechanic.

### What to reuse from the existing generator kit

| Existing code | Reuse in the rebuild | Necessary restraint/check |
|---|---|---|
| `tools/worldkit.py`: `World`, `place`, `bigdecor`, plate rotation and native writer | The existing format, exact numeric object/tile IDs, patrol endpoints, object icons and decor placement. | Respect current creature timing normalization; do not assume arbitrary author-supplied ticks override original speeds. No new runtime mechanic is needed by this proposal. |
| `tools/worlds/kit.py`: `wall`, `slab`, `rail`, `props`, `lift`, `mine` | Small authored platforms, panels, mechanical detail and both ground/hanging mines. | `lift` uses shared advance/recede timing; use `World.place` when a verified asymmetric journey is required. Record composite decks separately from journeys. |
| `slime.py`: `slime`, `skin`, `pole`, `arch` | Shaped slime masses, terrace edges, shafts and route landmarks. | `skin` is a finish helper, not a level-design algorithm. Avoid one giant filled block carved into repeated boxes. Its historical rendering comments are not current visual acceptance evidence. |
| `palace.py`: `stone`, `windows_row`, `parapet`, `gate`; BigDecor constants | White balconies, window recesses, monument and fountains. | Verify actual exposed faces, passable railings, switches and gates in the current renderer; some helper comments predate later model work. |
| `station.py`: `metal`, `metal_cells`, `girder_mast`, `brick`, `water`; plant/column/machine constants | Restore the grey station, brick library, wind machinery and functional water basins. | `shell` is a primitive only; it must not produce another run of identical chambers. Honour water exit heights and distinguish walk-through trim from solid walls. |
| `village.py`: `house`, `facade_row`, `roof_row`, `stone`, `bird`/`patrol` | Houses with varied heights, dormers, roofs and bird lanes across a real street. | Orient readable facades toward the route; review the helper's historical +Z assumptions against current face behavior. No blind “house” call whose visible side is blank. |
| `cheese.py`: `cave`, `bubble`, `hole`, `shaft`, `tube`, `pit_surface`, `cheese` | Authored connected cheese volumes, perforations and clearance-controlled passages. | Choose every chamber/connector deliberately and check camera clearance. Seeded surface variation does not replace authored topology. |
| Existing world199 encounter logic | Wasp well, lightning timing, safe bar dismount, consumed-key handling and native Win exit. | Reuse verified mechanics and test patterns, not `Room`/`enclave` layout or the plaza/ring skeleton. |

No new asset set is required. Use approved assets by their existing paths. This plan does not authorize reading or converting Ray Dream sources, modifying sibling repositories, or changing game behavior.

## 8. Verification and acceptance after a separately authorized rebuild

1. **Inventory and graph before polish.** Run the native inspector and reference inventory in read-only mode; reconcile all 196 object records, 20 types, 47 decor placements, 38 eggs and the eight key/gate groups. Also report lift journeys versus component tiles, 27 travelling versus nine stationary type-16 mines, all fan directions, and 0 treasure. Count matching is necessary here, not sufficient.
2. **Reachability in stages with explicit limits.** Run `python3 tools/levelcheck.py worlds3d/world199.vwr`; add `--rides` and `python3 tools/flightcheck.py worlds3d/world199.vwr` where relevant. Use separately named staged variants in the existing probe area only when the checker cannot model keys, powers, timed hazards or balloon escape. Publish exactly what each stage removes/replaces. A calm stage with open gates and stairs replacing lifts cannot certify the real route or difficulty.
3. **Recoverability and key-state review.** At each key, optional loop, helicopter landing, pool and one-way drop, show a way onward or back. Test held duplicate colours, each gate consuming one key, optional R2/G2 use/skip cases, both teleporter directions and return paths after a craft is lost. Do not call a checker limitation a softlock, or a waived checker result a pass.
4. **Complete ordinary playthrough.** Rewrite `tools/autoplay/world199.txt` to cover the intended main journey and assert key consumptions, vehicle transitions, critical lifts, switches and Win. Use a separate complete alternate-route script for optional branches; require it to reach Win too. Exercise all regions, both conveyor platforms, all meaningful switch contexts, the lollipop, invert, wasp escape, swimming exits, hanging mines and representative wind combinations. Do not satisfy coverage solely with teleported spawns or a route that walks around every vehicle.
5. **Readability and visual finish at player height.** Capture entrance, hardest combination and exit/reveal of every section, plus transitions and reverse views. Create a play-pass contact sheet covering the entire route, with timestamps and coordinates. Inspect bare backs of facades, walls/floors that dominate repeated frames, camera collisions, full-volume texture repetition, missing undersides, floating props, unsafe unseen hazards and ambiguous landing surfaces. Review the spare route visually too.
6. **Finale-specific acceptance.** Show basin → column/star ascent → statue/column/two fountains → green door → compact star frame/red arrow → Win. The frame must read as the destination from the approach; the room must not be uniformly star wallpaper. No claim of fidelity based only on a Win trace.
7. **Owner review of the journey.** Present the new full plan, complete sheets and original comparison at the same review. Assess variety, sustained combinations, route clarity and earned climax. Do not use voxel count, 43 occupied heights or a lesson-type checklist as a substitute for this review.

Those are future gates, **not tests run by this audit**. For later shared-machine runtime checks, run one game at a time and silence sound; use the user-specified frozen `build-probe/pass-bin` where its behavior is compatible with the rebuilt data. If later source changes make it stale, document that fact before selecting a different executable. Nothing in this report calls for a build during this read-only task.

## 9. Reproducible per-icon census

The following appendix is generated from the two input files with their existing read-only parsers. The 2D and 3D columns retain their different units. BigDecor and MoveObjects are counted separately above.

### Mobile Decor cells

```text
1:14  2:17  4:8  5:9  6:15  7:12  8:14  9:11  10:60  11:16
12:16  13:16  14:12  16:1  17:1  22:2  25:13  35:191  36:22  37:36
48:1  56:7  57:1  60:1  63:7  64:15  68:60  73:4  76:45  78:163
79:108  80:115  81:116  82:127  83:128  84:138  87:2  88:1  90:2  91:83
92:29  110:108  114:18  118:35  126:10  129:2  132:9  138:11  139:3  140:3
141:1  142:6  143:4  144:8  145:8  149:1  150:1  151:1  152:2  154:3
155:3  156:222  185:21  186:20  187:6  188:6  189:7  191:1  192:3  194:2
195:1  196:9  197:8  198:11  200:2  201:63  202:3  214:22  215:18  216:61
217:1  218:1  231:64  235:25  236:4  246:97  247:82  248:73  249:97  261:106
262:23  263:23  264:14  265:15  267:8  270:5  271:5  272:4  273:3  276:1
280:5  281:10  282:14  283:2  284:396  285:13  286:15  287:9  288:6  289:6
290:6  291:10  292:14  293:11  294:7  295:3  296:6  297:1  298:1  300:17
301:422  302:7  303:13  304:4  305:3  324:5  330:2  331:2  334:2  335:3
336:3  337:7  341:338  342:349  343:337  344:339  345:358  346:234  347:86  348:113
349:156  350:160  351:43  352:52  353:47  354:53  355:74  356:70  357:11  358:13
359:3  360:5  361:1  362:107  363:128  364:9  373:2  376:2  378:15  384:3
386:6  387:1  389:2  391:2  392:4  393:19  394:6  395:7  396:88  397:84
398:14  399:26  401:4  402:4  403:2  404:2  410:8
```

### Galaxy non-air voxels

```text
2:4  4:9  5:7  6:6  8:7  9:4  10:699  11:8  12:8  13:7
14:8  35:594  36:18  37:10  38:21  39:20  68:240  91:288  92:72  110:4
126:1  139:59  140:45  141:67  142:76  143:65  144:12  145:10  146:18  147:17
148:13  149:12  150:20  151:16  152:21  153:19  156:12  186:7  187:4  188:13
189:9  190:66  196:20  197:13  202:7  215:187  216:171  217:157  218:206  231:1311
246:256  247:234  248:237  249:250  261:853  262:36  263:40  283:42  284:639  301:162
304:3  305:9  324:2  330:2  331:2  334:12  335:18  336:18  341:58123  342:58189
343:57897  344:58032  345:57949  346:58082  347:3559  362:7056  364:9  373:2  378:24  384:2
386:19  387:23  388:12  390:15  392:19  393:17  396:681  397:21  404:2
```

Evidence images in `build-probe/` are ignored local artifacts; retain that directory with the report when handing the audit to another machine. All image links above are relative to this report.

## 10. Rebuild done (2026-11-06)

World 199 was rebuilt following §7 and checked against §8. Design and coordinates:
[the brief](../briefs/world199.md); generator `tools/worlds/world199.py`; main playthrough
`tools/autoplay/world199.txt` (28 434 ticks to the win screen), alternate route
`tools/autoplay/spare/world199.txt` (41 986 ticks, also to the win screen). Evidence (`plans/evidence/`): `world199-rebuild-places-1..4.jpg` (entrance, hardest moment and exit of
every place, labelled), `world199-rebuild-pass-1..4.jpg` (the play pass of the main script, a frame every
360 ticks with its tick and position), `world199-rebuild-ending.jpg` (the basin and island, the monument
terrace, the star frame with the red arrow).

### What matches

- **The journey.** Twelve places in the original's order (§2): a gun shaft with four shooting Blupis, an
  upper basin with conveyor stones, saucers and fans, a lift comb (eight single-cell lifts) and brick
  library, a white palace with three saw rows and its teleporters, lava terraces with the lightning,
  bars, bridge and wasp well, a village street and roofs, a riveted station with ten follow stars and
  a lava trench, cheese caves with bulldozers and hanging mines, a helicopter jungle (eighteen
  helicopters, four big creatures, spikes, drips), a rock gauntlet, a toy chamber of ball-block
  columns, and a basin with a column ascent to the monument and the red arrow. The player descends
  from the north-west top and climbs the monument under his start.
- **Counts (`python3 tools/vwr_inspect.py worlds3d/world199.vwr`).** 196 MoveObjects of the same 20
  types, every type exactly as §3 (26 lifts, 9 + 9 + 36 mines of which 27 travel and 9 stand, 6
  bulldozers, 38 eggs, 22 helicopters, 9 birds, 4 enemy Blupis, 6 big creatures, 14 follow stars, 2
  crates, 2 conveyor platforms, 2 inverts, 1 lollipop, 1 wasp, 8 keys, the exit); zero treasure; 47
  BigDecor by family exactly as §4. Tiles: 10/2/9 left/right/up fans, 22 flight barriers, 9 bridge
  cells, 2 spikes, 15 saws with 3 switches, 2 drips, 5 saucers, 2 + 2 teleporters, 4 lightning emitters
  with 3 strike cells, 14 bars; doors and keys 2 red, 3 green, 3 blue.
- **Key route.** R1/B1 in B, library behind R1 holding R2/G1, G1 into D, B2 into G, B3 into J, G3 at the
  last gate; optional R2 (the palace gallery) and G2 (F's roofs); each door consumes its colour; a
  carried G2 also opens the last gate. Both scripts take the consumptions in order.
- **The ending.** Basin (swum), island with the palm and G3, six columns and two star lifts, the monument
  terrace with the Blupi statue, its column, two fountains and vases, the green gate, the compact star
  frame with the red arrow, the win screen.

### What differs, and why

- Original coordinates are not reproduced anywhere (no import, extrusion or 2.5D): the places are
  authored volumes. Areas are smaller than the §7 envelopes where nothing needed more room; the route
  between A and B and the long connectors are tunnels with a gate, a drop or a view in them.
- §7 put B2 at "G's station route" and B3 at J: built as B2 at G's entrance, B3 at I's tunnel into J,
  G's exit free. The toy chamber's ten flight barriers fence its exit ledge (they filter nothing a
  walker needs); the other twelve fence A's east ledge.
- The follow stars of G (ten) are sleeping set pieces beside the way, not a pursuit: waking one chases
  Blupi and it blows itself on a wall; no script wakes one on purpose.
- Fans: left fans cross the ways of B's east ledge, G's hall 2 and J's north bank (their winds push
  across the path); up fans stand in floors beside or across the way (jumped at one-wide pinches in J).
  The three eggs on I's perches and the egg on B's pillar need the helicopter or the lollipop's power
  jump (neither is in `levelcheck.py`'s model); `flightcheck.py` reaches the perches' eggs.
- G's trench is crossed on two ferries one way (hall 2 is four lower: a drop); the route is forward.

### Checks (2026-11-06)

| Gate (§8) | Result |
|---|---|
| 1 Inventory | 196 / 20 types, 47 BigDecor, all tile budgets: `python3 tools/vwr_inspect.py` (the table above) |
| 2 Reachability | `python3 tools/levelcheck.py --stage 1 out.vwr`: OK, exit and every key reachable; `--rides`: OK; `flightcheck.py`: OK, three perch eggs with a helicopter, the pillar egg needs the lollipop; the real world stops at the first doors, as every keyed world |
| 3 Key states, recoverability | Every pit and fall has a way on: pools swum out; E's lava crossings are single ways with the bridge regrowing (156 ticks); the wasp's well leaves by the balloon (alternate script); a lost helicopter in I leaves the floor's walking way (stair, creatures); T331 returns from E's west wing (alternate script, D climbed again with its saws off); held R2/G2 never block a gate |
| 4 Playthrough | main script PASS 28 434 ticks; alternate PASS 41 986 ticks (lollipop, invert, saw row 3, wasp, T331, F's roofs); both conveyor stones, three switch contexts (rows 1, 2 main; 3 alternate), swimming exits, hanging mines (crouched), wind (B, G, J) |
| 5 Visual | play-pass sheets (every 360 ticks, `PLAY_PASS_JOBS=1`, PASS 28 434 ticks) and entrance / hardest / exit frames of every place: `plans/evidence/world199-rebuild-places-*.jpg`, `-pass-*.jpg` |
| 6 Finale | basin, island, columns, star lifts, statue and fountains, gate, star frame, arrow: `world199-rebuild-ending.jpg` and the last pass sheet |
| 7 Owner review | open: the owner's look at the journey |

Left for the owner's review: the white palace reads as pale stone with sparse trim; the long tunnels
J to K, I to J are plain rock/slime by design; the monument column (87) is drawn as a plain white
block by its model.

### 10.1 Follow-up after the owner's review (2026-11-07)

The owner accepted gate 7 and asked for five changes; done so far, in order:

- **Required branches (item 5, first part).** What the alternate route used to cover is now on the way:
  R1 lies on a four-high pillar in B that only the lollipop's power jump reaches (the lollipop lies at its
  foot); the red door (R2) in D and saw row 3 are required, G2 with them; F's street is shut by a gatehouse
  and the way on is G2's door, the lift and the roofs; E's lift down moved to the west wing, so the ferry and
  the passage east (now 17 cells longer, under the terraces, with the lightning in it) are required. Main
  playthrough 28 434 -> 34 594 ticks (9.6 min). The alternate route (B's invert shelf, the wasp's well,
  teleporter 331 and D climbed again) is 46 298 ticks. F's street birds are now 3 (35, 38, 41) and the roofs
  have 4 (the fourth over the last house): still 7.
- **G's follow stars awake (item 1).** The ten stars now sit where the wake box (about 2.2 cells either way
  on X and Z, level with him; `InteractionSystem`, BD-0053) triggers on the way: one by the ferry and one
  across from it in hall 1, two at the landing and three round B3 in hall 2, three by the way south. Awake
  they home on him at a fifth of his speed and burst on any wall; a probe standing still at the landing
  dies to them in four seconds. The route waits for the ferry outside every wake box (x 73.6), boards at once
  and takes B3 without lingering. The mine across hall 1's second lane moved to x 70.
- **K's helicopter and barriers (item 2).** A helicopter stands on K's floor; in the hall's north-west corner,
  at the exit ledge's height, an alcove (x 32-36, z 11-12) holds an egg behind the ten flight-barrier cells
  (z 13, two high): a walker sees it and cannot enter, the helicopter flies through (the alternate route
  does, and back to the floor). The twelve barriers of A are as before: 22 in all. Helicopters stay 22: I's
  east shelf has seven instead of eight, K one (so the main route boards the seventh at x 57).
- **D's palace (item 3).** Balustrades (398) along the south balcony's, the east balcony's and the bridge's
  void edges (walked through), fluted columns (399) down the north wall and the east wall, window panels on
  the bridge stones' south faces, cobwebs in the upper corners. Routes untouched.
- **Connectors (item 4).** I to J (x 28-30, now five wide) and J to K (x 27-31): ribs and niches in the
  walls, ceiling ribs, a chimney each up to a skylight, a floor mosaic of rock and slime, and I's two columns
  moved into the first connector. Not shortened. Budgets: 196 objects / 47 BigDecor as before.
