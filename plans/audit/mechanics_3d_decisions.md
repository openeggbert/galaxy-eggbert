# Intentional 3D adaptations (M3D-NNNN)

Where a 2D rule of `Decor.cpp` has no unique 3D reading, the reading chosen and why. Earlier
adaptations are recorded in `../GALAXY_EGGBERT_COMPLETION.md` §5 (D-rows: the dynamite blast in
3D D23, a fan's mouth D24, Z patrols D25/D26, follower homing D27, trails D28, the head box D38,
the key opening a door's whole block of cells D58, ...) and §11.

## M3D-0001 — a pushed or pulled crate moves one whole cell at a time

- 2D: the crate is at any pixel X (it follows Blupi's hands); only one axis exists.
- 3D: crates move along X and Z and must fit one-cell corridors and gaps on both axes; a crate
  left between two cells could never be pushed along the other axis through a corridor, and
  aligning it by hand to a pixel is not humanly possible with tank controls.
- Reading: the crate moves continuously at the original's speed (`CaisseGetMove`, its ramp, the
  stack slowdown, Power), held at the original's 57 px from Blupi, but a move once started finishes
  its cell (at most 64 px: 0.64 s pushing at full speed, 1.07 s pulling); holding the button carries
  it on into the next cell without a stop. Meanwhile Blupi keeps pushing or pulling and cannot turn;
  a jump takes him off it at once (the original's Jump interrupts Push) and the crate then finishes
  its cell alone. Support is checked for the destination cell (no 19 px overhang over a ledge's
  edge as in the original, which stops the crate when a corner's 20 px support patch leaves the
  floor).
- Pull: also needs room and ground for Blupi a cell behind him (a pull never takes him off a
  ledge; in the original he would walk off it and fall, ending the pull).
- Push direction: the axis he faces most (not within ~10 degrees of a diagonal, as
  `CrateInFront`); while he pushes or holds he squares up to the crate's face unless the player
  turns him.
- Evidence: `VerifyInteractionSystem` "Crate push timing against the original".
