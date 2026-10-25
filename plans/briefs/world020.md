# World 020 — the hub of world 2 (the white palace)

Built by `tools/worlds/world020.py` → `worlds3d/world020.vwr` (mission 20, region 25), with the
world-2 tile pieces in `tools/worlds/palace.py`.

**Reference** (`worlds/world020.txt`, video 3:03:27, `video-evidence/w020/sheet.jpg`): a
staircase climbing through the palace before the mosaic. Blupi starts at the bottom beside
mission 1's sign (174, open); above it, each two rows higher and a step to the left, the signs
of missions 2-5 (175-178) stand on ledges behind blue bars (182), and ledges on the other side
of the shaft are the steps between them. White stone (396) and its trims (391-397) only.

**3D design**: a walled courtyard (x 30–70, z 30–74, walk 1) with a stepped palace mound in
the middle: four white tiers, each two blocks above the last (walk 3, 5, 7, 9), so Blupi climbs
round the mound with one jump per tier.
- Sign 1 in the mound's south foot at (50, 1, 62), open; Blupi starts in front of it at
  (48, 1, 66) facing it.
- Sign 2 in tier 2's east riser (58, 3, 50), sign 3 in tier 3's west riser (45, 5, 50), sign 4
  in tier 4's east riser (52, 7, 50), sign 5 in a shrine on the top tier (52, 9, 50) opening
  west. Each gated sign sits two deep, the bar in the doorway under a lintel; the niches open
  along X, since a door bar blocks along X only (the 2D tile mask extruded through Z: crossed
  along Z, Blupi walks past it at the side of the cell). After a win Blupi arrives on the
  terrace just outside the bar that opened.
- Windows (391) and trims on the south faces of the tiers and the north wall, where those
  tiles show their picture; grass beds with palms and plants in the courtyard corners,
  fountains before the mound and columns on the tiers' corners (BigDecor models).

**Checks**: `levelcheck.py worlds3d/world020.vwr`: all five signs reachable (bars counted open);
`VerifyShippedWorlds worlds3d`: 80/80. Live (`--mission 20`, shots in
`build-probe/qa-shots/world2/hub/`): the courtyard and the mound from the spawn, sign 1 in the
foot, each bar in its niche from the terrace in front of it (spawns 62,3,50 / 41,5,50 /
56,7,50 / 48,9,50), the palms and plants in a corner bed.
