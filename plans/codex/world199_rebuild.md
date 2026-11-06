# Task: rebuild world 199, the finale, as a true 3D journey closely inspired by Mobile Eggbert's (Galaxy Eggbert)

Repository: /rv/data/development/github.com/openeggbert/galaxy-eggbert (develop). Read `AGENTS.md`,
`../AGENTS.md`, `CLAUDE.md` ("Faithful-remake rules") and `plans/codex/README.md`.
Recommended: `gpt-6-astra`, effort **xhigh** (large spatial design plus a full playthrough).

## Hard rules
- Never modify anything outside this repository; never read `../speedyblupi-data`. The reference
  (`../mobile-eggbert`) is read only.
- Commit when done (see `plans/codex/README.md`; no AI attribution; never push). This is a big task:
  commit in a few discrete steps if that helps (e.g. layout + objects, then playthrough, then finish),
  each leaving the tree working (ctest green, the old or new playthrough passing).
- Build only in `build-cna-opengl3/` (`CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv`, `--parallel 4`);
  silent game runs; scratch under `build-probe/world199-rebuild/`; one game at a time.
- Worlds are hand-authored true 3D tile worlds: **no import, conversion or extrusion of the original's
  2D cell layout, no fixed-depth corridors, no 2.5D** (the owner rejected that twice). No new gameplay
  mechanic, no new asset set (approved assets by their existing paths only).

## The owner's direction (2026-11-06)
"The last, final world is a mockery and sloppy; compare it with the final world in Mobile Eggbert",
then: **"just take strong inspiration from Mobile Eggbert's final world and do it in 3D somehow"**.
So the original's journey is the main guide this time, not just its themes: the same sequence of
regions with their identity and set pieces, the same encounter relationships, the same object
budget, the same ending -- reinterpreted as volumes a player moves through in X, Z and height.

## Inputs
- `plans/audit/world199_comparison.md` -- read it whole. §2 the original's twelve regions in order,
  §3-§4 exact counts (196 objects / 20 types, 47 BigDecor, fans, barriers, bridges, saws, lightning,
  spikes, drips, water/lava), §6 the gaps, **§7 the rebuild plan** (sections A-L with envelopes, the
  reconciled object allocation table summing to 196, the decor and hazard budgets, the key-state route
  R1..G3 and the ending), **§8 the acceptance gates**.
- Images: `plans/evidence/world199-original-overview.jpg` (and the full set in
  `build-probe/world199-compare/` if still there: full map, detail strips, the reference video's
  samples, the ending at tile scale).
- The current generator `tools/worlds/world199.py` (reuse its verified mechanics -- wasp well,
  lightning timing, bar dismount, consumed keys, the Win exit -- not its plaza/ring/enclave layout),
  the kit (`tools/worlds/kit.py`, `slime.py`, `palace.py`, `station.py`, `village.py`, `cheese.py`,
  `tools/worldkit.py`), the brief `plans/briefs/world199.md` (rewrite it to the new design), the
  playthrough `tools/autoplay/world199.txt`, and how other worlds' generators/briefs are written.

## Do
1. Design first: write the new `plans/briefs/world199.md` -- sections A-L (adjust where 3D needs it,
   say why), their order and connections, heights, the key-state route, where each object of the
   196 goes and why, the 47 BigDecor, the ending (basin -> column/star ascent -> Blupi statue with its
   column and two fountains -> the green gate -> the compact star frame -> red arrow -> Win).
2. Author it in `tools/worlds/world199.py` (rewrite; keep mission 199, sky region 31, music 1, no
   treasure, the existing Win behaviour) and generate `worlds3d/world199.vwr`. Every section must be
   a distinct, volumetric place with finished surfaces (no bare flat walls/floors dominating the
   view, no placeholder rooms), using the original's materials per region.
3. Verify by §8 of the report: the inventory reconciled (`python3 tools/vwr_inspect.py`),
   `python3 tools/levelcheck.py worlds3d/world199.vwr` (`--rides`), `python3 tools/flightcheck.py`, the
   key-state and recoverability review, a complete new `tools/autoplay/world199.txt` reaching Win along
   the main journey (and an alternate-route script if branches matter), the hub chain that includes it
   if any, `tools/verify_golden_*` and ctest (the golden scenes do not use 199; if they do, explain).
4. Look at it from the player's height: a play pass of 199 (`PLAY_PASS_JOBS=1 tools/play_pass.sh
   build-cna-opengl3 build-probe/world199-rebuild/pass world199`), plus screenshots of each section's
   entrance, hardest moment and exit, and of the ending; fix what looks unfinished. Put a few of the
   best into `plans/evidence/world199-rebuild-*.jpg` for the owner.
5. Record: `plans/audit/world199_comparison.md` gets a closing section "Rebuild done" (what matches,
   what differs and why), the level ledger the project uses for worlds (see `handoff.md` §4 "World
   changes"), `handoff.md` §0's queue line.

## Report
The design summary, counts reconciled against the original, the checks' results, the playthrough's
PASS line and ticks, the evidence paths, and anything left for the owner's review.
