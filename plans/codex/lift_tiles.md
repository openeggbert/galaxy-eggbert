# Task: a lift with any tile's look is drawn as that tile, moving (Galaxy Eggbert)

Repository: /rv/data/development/github.com/openeggbert/galaxy-eggbert (develop). Read `AGENTS.md`
and `../AGENTS.md` and follow them.

## Hard rules
- Never modify anything outside this repository; CNA, cna-engine, mobile-eggbert, speedyblupi-data,
  blupi_recon, free-eggbert are read only (never read speedyblupi-data at all).
- Commit the finished task yourself (one commit, no AI attribution; see `plans/codex/README.md`);
  never push, stash or reset. Keep any unrelated uncommitted changes you find.
- Build only in `build-cna-opengl3/`: `export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv;
  cmake --build build-cna-opengl3 --parallel 4`; `ctest --test-dir build-cna-opengl3 -j4` (all pass).
  Silent game runs; runs under `build-probe/lift-tiles/`. If the build fails inside
  `../../libcna/cna` (another session's live tree), report and stop.

## Background (owner 2026-11-06)
The owner asked why, as in Mobile Eggbert, a moving platform cannot be any object/tile. In the
original a lift (ObjectType1) carries its own icon, any decor tile of object-m (the levels use 29
most, then 361, 387, 340, 153, 215 and others; count them over `../mobile-eggbert/worlds/*.txt` with
`tools/world_inventory.py`), and it is drawn as that tile moving. Of the options offered, the owner
chose only **"drawing the lift as the tile"** (rendering; no new gameplay).

## Galaxy now
- Data: `MoveObjectRecord::visualIcon` (`include/GalaxyEggbert/MoveObjectRecord.hpp`, serialized),
  `MobileObjSpec::visualIcon`, `ObjectDefinitionRegistry::Resolve(type, phase, visualIconOverride)`
  (an override switches the texture source to ObjectM).
- Editor: the palette already gives a lift the look of the selected block when its entry's visual id
  is negative (`src/GalaxyEggbert/Editor/EditorPalette.cpp` ~194-198, `selectedObjectVisualIcon_`).
- Drawing: `GalaxyEggbertGame::ConveyorLiftModel` (~3325) draws a lift with a model only when the
  icon has one (29 -> Decor/techno29.rd4, "lift29"); otherwise the lift falls back to whatever the
  object path draws (check: probably a sprite/billboard, which the audit wants gone).

## Do
1. A lift whose icon has no object model is drawn as that tile would be in the terrain, moving with
   the lift: a cube with the tile's faces from the terrain atlas (`TileAtlas`, the directional-cube
   faces where the tile has them, `TileCoverage` for cut-out tiles), lit as the terrain; a tile that
   the terrain draws as a model (`kStillModelIcons` etc.) is drawn as that model. 29 stays the
   techno29 lift. Reuse the terrain's per-tile face/UV code rather than duplicating it (factor a
   helper if needed). Collision and riding stay exactly as they are.
2. A pure-logic verifier where possible (e.g. which draw path a lift icon takes: model, tile cube,
   none), and ctest all pass; golden frames/trace must still pass.
3. Live: a probe world under `build-probe/lift-tiles/` (a copy of a small world with three lifts set
   to 29, 361 and 153, or made with the editor's palette) and screenshots of them moving; report paths.
   Also say how an author sets a lift's look in the editor today, and whether that works.
4. Note: no campaign world sets a lift's look other than 29, so the player sees no change until worlds
   or the editor use it; say so in the ledger entry (`plans/audit/editor_parity.md` or
   `behavior_differences.md`, whichever fits; write it in that ledger's format).

## Report
Files changed, the draw paths, test lines, screenshot paths, anything left.
