# World 050 — the hub of world 5 (the space station's lift tower)

Built by `tools/worlds/world050.py` → `worlds3d/world050.vwr` (mission 50, region 7 as the
original: black space with stars). World 5's tiles and pieces: `tools/worlds/station.py`.

**Reference** (`worlds/world050.txt`, video 4:32:09, 4:41:16, 4:53:41): a lift shaft in grey
riveted plates (78–84, 87) beside a lattice of girders (201) against black space. Blupi starts
at its foot left of the lift, mission 1's sign (174) to its right. Lift 1 (80 ticks, stop 10)
rises eight cells past ledges one cell wide beside it, two rows apart: signs 2, 3, 4 (175–177)
behind blue bars (182), and mission 5's sign (178) at its top. Lift 2 (60 ticks, stop 10)
starts there one column over and rises six cells past 6 and 7 (179, 180) to 8 (181). Blupi
steps off a lift at the ledge he wants. A bar slides up once the mission before it is won.

**3D design**: a station deck floating in space (x 37–62, z 40–62, walking level 11, a parapet
round its edge, the hull narrowing under it, girder masts at its corners) with a tower of plates
in the middle (x 46–53, z 47–52, roof walk 23). Sign levels three cells apart, out of a jump's
reach (the original's two rows are a jump in 3D): walk 11, 14, 17, … 32.
- Sign 1 open in the tower's south face at (49, 11, 52), a strip of grilles leading to it from
  the spawn (49, 11, 58), facing −Z.
- Lift 1, four lifts in step (2 × 2) at x 44–45, z 49–50 along the tower's west face, its cube
  in a pit flush with the deck, rises twelve cells to the roof (120 ticks: the original's ten
  ticks a cell, stop 10). Its balconies, each with the sign at the outer end in a small kiosk,
  two deep behind the bar: sign 2 west (walk 14, sign at x 38), 3 north (walk 17, z 43), 4
  south (walk 20, z 56).
- The roof (walk 23): sign 5 in a kiosk on its north edge (50, 23, 46), a parapet round it
  except where the lifts meet it.
- Lift 2 (2 × 2, x 54–55, z 49–50) rises from the roof nine cells (90 ticks): sign 6 north
  (walk 26, z 43), 7 east (walk 29, x 61), 8 on the top platform south of it (walk 32, z 56)
  beside a girder beacon.
- Blupi rides up and jumps off onto the balcony he wants (or rides to the top and drops onto
  one); a balcony missed is a fall to the deck. After a win he arrives one cell past the bar
  that opened, on its balcony.

**Checks**: `levelcheck.py` (all eight signs reachable, bars counted open); `VerifyShippedWorlds`
80/80. Live (`--mission 50`, shots in `build-probe/world5-agent/shots/hub-*`): the start view
with sign 1 in the tower's face; walked into sign 1 → mission 51 loads; lift 1 ridden from its
foot up and down past the balconies (sign 3's bar seen level on the way); a jump off lift 1
lands on sign 2's balcony at the right moment (3.8 s after the start; the earlier tries at 2.0–3.2 s
did not reach it); with a probe save (`doorsUnlocked` 52) bar 2 is open and Blupi
arrives on sign 2's balcony facing it, then walks in.
