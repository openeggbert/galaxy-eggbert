# Intentional 3D adaptations (M3D-NNNN)

Where a 2D rule of `Decor.cpp` has no unique 3D reading, the reading chosen and why. Earlier
adaptations are recorded in `../GALAXY_EGGBERT_COMPLETION.md` §5 (D-rows: the dynamite blast in
3D D23, a fan's mouth D24, Z patrols D25/D26, follower homing D27, trails D28, the head box D38,
the key opening a door's whole block of cells D58, ...) and §11.

## M3D-0001 — crates move continuously and rest anywhere (revised 2026-11-01 after OR-0021..0023)

- 2D: the crate is at any pixel X (it follows Blupi's hands); only one axis exists.
- First reading (D60, 2026-11-01 morning): a move once started finished its cell, so crates always
  rested on whole cells and support was checked for the destination cell. **Overturned the same
  day by the owner** (OR-0022: a pulled crate kept moving a second after the keys were released;
  OR-0023: a crate could not be pulled onto another so that it overhangs, as in Mobile Eggbert;
  OR-0021: a pull needed a whole free cell behind Blupi and so often did not start).
- Reading now: as the original, every tick of Push or Pop moves the crate group by
  `CaisseGetMove` pixels (its ramp, the stack slowdown, Power) if all of it fits there
  (`TestPushCaisse`: all or nothing), and nothing moves otherwise; letting go stops it at once;
  it rests wherever it stops. In 3D a crate is a unit box at any X and Z: it is tested against the
  blocks and the other crates it overlaps, and the seed row's crates need each of the four 20 px
  corner patches of their bottom over a solid block or a crate (the original's two lower
  corners, `TestPushOneCaisse`), so a crate overhangs a ledge, water or another crate by up to
  20 px as in the original.
- Pull: Blupi backs away with the crate; it needs room for his collision point (blocks and other
  crates) where it puts him, nothing more; he may back off a ledge and fall, which ends the pull
  (the original). A pull that cannot go is the original's Stop: the next starts over from rest.
- A push needs him walking into the crate's face: his centre within half a cell of its line
  across the push (where his collision point would meet its cube); walking past its side does not
  push it.
- Push direction: the axis he faces most (not within ~10 degrees of a diagonal, as
  `CrateInFront`); while he pushes or holds he squares up to the crate's face unless the player
  turns him. A crate left between two cells on one axis can still be pushed along the other: its
  box is tested where it is.
- Evidence: `VerifyInteractionSystem` "Crates in 3D" and "Crate push timing against the
  original" (OR-0021/0022/0023 checks).

## M3D-0002 — a crate slides into its lane when a sliver across blocks it (2026-11-01)

- 2D: crates move along one axis only; nothing across a push can catch them.
- 3D: a crate pushed along Z stops wherever Blupi stops (M3D-0001), which in practice is a few
  pixels past a cell; pushed next along X beside a block, that sliver of its box overlaps the
  block's row and the original's all-or-nothing test refuses every step. Aligning a crate to the
  pixel is not humanly possible with tank controls (found replaying mission 041: the crate at
  z 70.02 could not pass the bench at x 22, z 71).
- Reading: when a push or pull cannot go, and the seed crate lies off the centre of its lane by
  at most 16 px (`CratePush::kLaneSlide`, a quarter cell), the group moves across toward the
  lane centre instead, at the same speed and under the same tests (blocks, crates, support); once
  in the lane the move goes on. Farther off, nothing moves, as the original.
- Evidence: `VerifyInteractionSystem` ("a crate a pixel off its lane ... slides", "a third of a
  cell off ... stays blocked"); mission 041's playthrough.

## M3D-0003 — a bar is two pipes along its row; Blupi hangs on its centre line (2026-11-02)

- 2D: a bar is a line across the screen; Blupi hangs in profile under it, facing the way he moves
  (table_blupi 31/32/34: icons 156-158, 143-146, 165-166), his position snapped onto its height
  (BLUPISUSPEND, Decor.cpp:5442) and moved along it only.
- The Ray Dream poses behind those sprites (Blupi/suspendu*.rd4, no bar in the scenes) hold his
  two hands side by side across him, 0.35 of a cell apart (the palms of 156-158), which the side
  view shows as one grip. Under one pipe in 3D, neither hand would touch it (owner 2026-11-02:
  "the hands on the bar are wrong", and: the 3D game probably needs two bars, one per hand).
- Reading (BarRows.hpp): a bar cell is two parallel pipes along its row, 0.175 either side of its
  centre line -- from the original's side view still one pipe, as the tile. Its row runs along X
  or Z by its bar neighbours (a lone cell along X); with neighbours on both axes it is a field
  (worlds 071, 074, 075), where he moves freely as before. On a row, grabbing puts him on the
  centre line facing along it (the end nearest his yaw) and he moves along it only; turning picks
  the other end once past the perpendicular. His model hangs 6 px lower than standing in the
  bar's cell (BLUPISUSPEND 12 against BLUPIOFFY 6), and drawn with his right palm on his right
  pipe per icon (`HangingModelShift`): in the two-handed poses both palms on their pipes; in the
  pause hanging by one hand (162-164) his body under that pipe.
- Rejected: one pipe and Blupi turned across it (the sprites show him in profile, facing his
  travel); one pipe on his centre line (both hands miss it by 0.15-0.2 cell).
- Evidence: `VerifyBlupiMovement` (grabbed off the line and turned, he hangs on it facing along the
  row and moves along it only), `VerifyTileFaces` (rows and fields); live probe world 995,
  plans/evidence/bar-two-pipes.png (the original's sprites and tiles above); the playthroughs of
  every world with bars (064, 065, 066, 071, 074, 075, 083, 084) pass.

## M3D-0004 — a fan is a whole block; its mouth kills (2026-11-02)

- Original (Decor::IsVentillo, the fans' quarter masks): a fan's cell has one solid quarter-column at
  its back; its other three quarters are open toward its mouth, and Blupi's centre reaching the
  mouth band (16 px) kills him and wrecks the fan with its wind.
- Galaxy had made fans never solid in collision (`IsPointSolid`'s exceptions, with teleporters,
  wind, water and bars), so in 3D he walked through a fan from any side (owner 2026-11-02: "in
  Mobile Eggbert the fan cannot be passed; walking onto it Blupi dies and it explodes").
- Reading: the fan's cell is a whole block (`IsPointSolid`; `levelcheck.py` alike): from its back
  and its sides it is a wall; at its mouth, the cell in front of it, he dies and it goes with its
  wind (`WorldRuntime::TryConsumeFan`, as before). The registry keeps the original's mask
  (`VerifyDefinitionRegistries`).
- Evidence: `VerifyBlupiMovement` (walking at a fan along Z he stops at its face); levelcheck's
  reachability of every world with fans unchanged; the playthroughs (run 2026-11-02).

## M3D-0005 — fans and wind turned to blow along Z (2026-11-02, the owner's decision)

- Original: a 2D side view; the side fans (126 left, 129 right) and their wind (110 west, 114
  east) push along the screen's X only; up and down fans and winds (132/135, 118/122) vertically.
- 3D: a fan or wind tile can be turned 90 degrees about the vertical (the cell's plate-rotation
  metadata, as for saws and signs): -X/+X become -Z/+Z. The owner asked for wind tiles beside a
  fan turned to any side and chose to add this (Q-000 in `owner_questions.md`).
- Where: `BlupiController` (a turned 110/114 at his middle pushes along Z), `WorldRuntime::
  TryConsumeFan` (a turned side fan's mouth, and its wind, along Z), the fan model turned -90
  degrees, the wind plate turned by the existing plate rotation; the editor's O key turns the aimed
  tile (undoable); `World::findBlockExtraMetadata` reads it per cell; setting it counts as a block
  change for the renderer.
- Evidence: `VerifyBlupiMovement` (a turned east wind pushes along +Z), `VerifyInteractionSystem`
  (a turned fan's mouth along -Z, safe beside it); the gallery (world998) holds a turned fan and
  wind at (47, 1, 56-60).
