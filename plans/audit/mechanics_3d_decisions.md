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
