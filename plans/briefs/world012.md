# Mission 012 — the crate lesson (blue machine world)

Built by `tools/worlds/world012.py` → `worlds3d/world012.vwr` (mission 12, region 0); hints:
`Tables::table_training2` (5 records) as hint zones.

**Reference** (`worlds/world012.txt`, video 1:50–3:12): rooms climbing to the right, each ending
at a ledge too high to jump to — push a crate to the red dot under it (tile 3), "Practical,
right?", pull a crate back out of a corner to the red dot, stack two crates on the red dot, then
three, climb them; the exit on top. 9 crates, no treasures. A crate never falls in the original
(`TestPushOneCaisse` wants support under it): a stack is built by pushing a crate off a shelf onto
the crate level with it.

**3D design**: five machine terraces climbing away from the start (−Z), each higher than the last
by its lesson's ledge (3, 3, 4, 5 blocks; Blupi jumps 2):
1. T0 → T1: push the crate four cells along the floor to the red dot at T1's foot, climb it.
2. T1 → T2: the crate sits at the end of a one-wide alcove whose walls stand three above its top;
   only pulling (crouch, facing it, back away) brings it out to the red dot.
3. T2 → T3: one crate along the floor (−Z) to the red dot, the second pushed (−X) off a one-block
   shelf onto it.
4. T3 → T4: three crates from three sides — along the floor (−Z), off a one-block shelf (+X), off a
   two-block shelf (−X).
A low roof over each shelf (two above its crate's top) leaves Blupi no full jump there, so a
shelved crate is no stair to the next terrace until it stands on the stack. The red arrow on T4.

**Checks**: `levelcheck.py` on the world and on `world012.py --solved` (crates at their goals):
exit unreachable / reachable; with lessons 1..k solved Blupi reaches walk levels 3, 5, 9, 14, then
the top — every lesson is needed. Live: push to the red dot, pull out of the alcove.
