# World 070 — the hub of world 7 (the toy blocks)

Built by `tools/worlds/world070.py` → `worlds3d/world070.vwr` (mission 70, region 22 as the
original). World 7's tiles and pieces: `tools/worlds/toys.py`.

**Reference** (`worlds/world070.txt`, video 3:36:37 and 3:43:04): a small heap of toy tiles in the
region-22 sky (dark with yellow stripes): balls and marbles on coloured grounds (215-220, 215 the
most), star badges (230/231), toadstools (233/234), striped candy poles (235/236), a lollipop and a
half ball in the BigDecor layer (45, 39). Blupi starts between two stacks: mission 1's sign (174)
stands open beside him on the left, mission 2's (175) on the right behind its blue bar (182);
3 and 4 one storey up in the left and the right stack, 5 a storey higher in the left one, each
behind its bar.

**3D design**: a court of toy balls (x 30–70, z 36–64, walking level 5) floating in the sky, a rim
three high (out of a jump's reach) with striped posts topped by balls (221–225); two towers of giant
toy blocks on it, set off the rim so the sky shows round them.
- The west tower, three blocks stacked, each smaller and of another toy: W1 of orange and yellow
  balls (x 34–42, z 41–51, top walk 9), W2 of star badges (x 34–39, z 41–48, walk 13), W3 of
  toadstools (x 34–36, z 41–45, walk 17). The east tower, two: E1 of marbles (x 58–66, z 41–51,
  walk 9), E2 of balls (x 61–66, z 41–48, walk 13).
- The signs in the faces toward the court, as the original's order: 1 open in W1's east face at
  (42, 5, 47), beside the start; 2 in E1's west face (59, 5, 47); 3 in W2's east face (38, 9, 45)
  over W1; 4 in E2's west face (62, 9, 45) over E1; 5 in W3's east face (35, 13, 43) over W2.
  2–5 sit two deep with the bar in the face (`kit.sign_niche`), so there is no way round a bar;
  Blupi arrives outside the bar last opened. Star badges frame each opening.
- Steps: small toy blocks two high at the towers' feet (one jump each): to W1 at x 40–42,
  z 52–54 (walk 7), on W1 to W2 at x 37–39, z 49–51 (walk 11), to E1 at x 58–60, z 52–54.
- Toy decor on the court (BigDecor): three lollipops (45–47), two half balls (39, 40), a cone
  with balls (36).
- Spawn (49, 5, 52) facing −Z, sign 1 ahead on his left, sign 2's bar on his right.

**Checks**: `levelcheck.py` (all five signs reachable, bars counted open) and a variant with the
bars closed (`build-probe/world7-agent/barcheck.py`: only sign 1 reachable). `VerifyShippedWorlds`.
Live (`--mission 70`, spawns facing each sign, F3): the start view over the court, sign 1 in its
star frame, the bars of 2–5 in their faces, W1 and W2 tops in front of 3 and 5. Seen there: the
signs' digits show mirrored in every hub (world 4's and 6's too), reported to the main session.
