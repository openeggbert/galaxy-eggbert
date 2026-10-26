# World 123 — mission 3 of world 12 (the library)

Built by `tools/worlds/world123.py` → `worlds3d/world123.vwr` (mission 123, region 19 as the
original). Staged variants for `levelcheck.py`: `--stage K out.vwr` (K treasures taken: the doors
"2", "4" and "9" open as they count; from K ≥ 2 the crates pushed into their step).

**Reference** (`worlds/world123.txt`, video 0:16:04-0:19:15): a library in brick (261-263, 283):
bookcases (139-143) in stacks, fifteen treasures on top of them and between them, climbed from
case to case; treasure doors (422 "2", 424 "4", 429 "9", 432 "12") shutting the ways on; a lift (60
ticks) up to the east wing, two crates (12) to push into a step, a pit of lava, three eggs, golden
forks and pillars.

**3D design**: a great hall of brick with three wings.
- The west hall (walk 7): spawn (8, 7, 72) by the exit. North of the start a stack of bookcases:
  five rows across the hall, each two higher than the one in front and touching it, a stair of
  bookcases from walk 9 to 17, a treasure on each row; two more on the floor; an egg. Door "2" east.
- The middle hall: a pit of lava across it crossed on a causeway of brick; a high shelf of
  bookcases four high (walk 11) with three treasures: push the crates into a step (one, then one
  on it); an egg. Door "4" east.
- The east wing: a lift (2 × 2, 60 ticks, dwelling 30) up ten to its gallery (walk 17): bookcases
  along it, three treasures; door "9" to the last room with two treasures and an egg.

**Checks**: `levelcheck.py`: stage 9 reaches the fifteen treasures, three eggs and the exit; stage 2
stops at door "4"'s wing and door "9". `VerifyShippedWorlds`. Live (`--mission 123`, direct view
F3): the hall and the stack of bookcases (their books on the faces), a treasure on its top row,
the pit of lava, the lift, the gallery. Not played through: the climb, the crate step, the lift.
