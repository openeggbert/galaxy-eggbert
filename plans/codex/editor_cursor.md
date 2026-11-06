# Task: the 3D editor aims at the mouse cursor (and the touch point), not the screen centre (Galaxy Eggbert)

Repository: /rv/data/development/github.com/openeggbert/galaxy-eggbert (develop). Read `AGENTS.md`
and `../AGENTS.md` and follow them.

## Hard rules
- Never modify anything outside this repository; CNA, cna-engine, mobile-eggbert, speedyblupi-data,
  blupi_recon, free-eggbert are read only (never read speedyblupi-data at all).
- Commit the finished task yourself (one commit, no AI attribution; see `plans/codex/README.md`);
  never push, stash or reset. Keep any unrelated uncommitted changes you find.
- Build only in `build-cna-opengl3/`: `export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv;
  cmake --build build-cna-opengl3 --parallel 4`; `ctest --test-dir build-cna-opengl3 -j4` (all pass).
  Silent game runs. No new build directories or caches. If the build fails inside
  `../../libcna/cna` (another session edits that live tree), report it and stop.
- Editor UX may be invented (CLAUDE.md: the editor is the one approved exception) but must not add
  gameplay behaviour.

## The problem (owner 2026-11-06)
"In the 3D editor placing the moving objects seems not to work" -> "after the click nothing
appears". The editor aims with the camera's centre ray (`WorldEditor::UpdatePlacementPreview`,
`src/GalaxyEggbert/Editor/WorldEditorUpdate.cpp` ~183: `Raycast` from the camera along `forward`;
`hitCell*`/`placementCell*` in `WorldEditor.hpp` ~270-301), so the red preview and every placement
land where the screen centre points, whatever the mouse is over: the owner clicked where he wanted a
lift and saw nothing there. His decision: aim with the cursor ("Ano, mířit kurzorem").

## Do
1. The aim ray goes through the mouse cursor's pixel (desktop) or the last touch point in the view
   (touch), unprojected with the editor camera's view/projection and the viewport (CNA's
   `Viewport::Unproject` or the equivalent math; the game already projects points with
   `camera_.View()`, `camera_.Projection()` -- see uses in `GalaxyEggbertGame.cpp`). The preview,
   the click placement, removal (middle click), box fill, copy/paste (ED-0010), the moving objects'
   start/end clicks and BigDecor placement all use that one ray, so they stay consistent.
2. Keep: RMB mouse-look (while the right button turns the view, aim at the centre or freeze the
   aim -- choose what feels right and say why), the NumPad / on-screen X Y Z placement offsets on
   top of the ray's cell, the empty-world y=0 fallback. When the cursor is over the editor's UI
   (palette, bar, buttons such as PLACE), the preview keeps the last cell aimed in the view, so the
   PLACE button and the keyboard (Enter) place there instead of under the button.
3. Tests: extend `tools/VerifyGEWorldEditor.cpp`: with a known camera and viewport, a cursor at the
   centre gives today's cell; a cursor offset toward a known block hits that block's cell; the cursor
   over the UI keeps the last cell; a touch point aims the same way. All ctest pass.
4. Live: `tools/qa_live.sh` with a steps file (see `tools/qa_steps/editor_lift_click.txt` and the
   header of qa_live.sh; add a step file `tools/qa_steps/editor_cursor_aim.txt`; if the steps
   language has no mouse move, a click moves the cursor too): place a block and a lift at two
   different cursor positions off the centre; screenshots showing the preview under the cursor and
   the placed things there; save under `build-probe/editor-cursor/` and report the paths. Coordinates
   of the editor's controls used so far (800x480 window): editor menu (710,200); slot 1 (72,258);
   moving-mechanisms group (30,285); its first item (75,285); PLACE (500,450).
5. Update `plans/audit/editor_parity.md` only if you are sure of the format; otherwise list the facts
   in the report and write the entry yourself in the ledger's format.

## Report
Files changed, the design decisions (RMB look, UI hover, touch), test lines, screenshot paths,
anything left.
