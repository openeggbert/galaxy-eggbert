# Editor parity (ED-NNNN)

Speedy Blupi 1 / Speedy Blupi 2 editors (`../blupi_recon`, decompiled; `../free-eggbert`
`src/decdesign.cpp`, `src/event.cpp`) against Galaxy's 3D editor (`src/GalaxyEggbert/Editor/`).
Galaxy-only 3D UX is allowed where the original has no equivalent; it must not change gameplay.

| ID | Behaviour | Original evidence | Galaxy | Status | Next |
|---|---|---|---|---|---|
| ED-0001 | Repeated clicks cycle a placed item's variants | traced 2026-11-01, below | `EditorVariants.hpp` (`VariantCycler`) in `WorldEditor::HandlePlacement`: a click on the block the previous click placed cycles it; Ctrl places without cycling (owner's choice, MR-0003) | FIXED_AND_VERIFIED for block families (`VerifyGEWorldEditor`); BigDecor families (trees, palms, houses ...) not yet | BigDecor cycling (ED-0006); a live UI run |
| ED-0002 | Ten editable slots per gamer, three gamers; slot 1 seeded from the showcase | owner requirement (completion plan §10) | done `7eac7d5` | FIXED_AND_VERIFIED for the slots (`VerifyGEEditorStorage` 2026-11-03: ten slots per gamer as custom_001-010, numbering, no overwrite when full, older worlds copied into a free slot, slot 1 seeded once with the showcase and the gallery into the next free slot, edited copies never overwritten, the browser's open/new/delete/back); play-test from a slot: ED-0004 | end-to-end script |
| ED-0003 | Palette groups and order (ten groups) | Eggbert 2 editor; the PC editor's `PlaceDecor4` (blupi_recon decdesign.cpp) for the lifts | `PaletteCategories.cpp`; 2026-11-03: the moving mechanisms' four lifts were the conveyor types 47/48 and a plain lift in the wrong places, every lift at one default timing, and the slime creature type 18 (in no original world): now type 1 at the PC editor's speeds per cell (slow 20/20 rest 20, normal 10, fast 6 rest 3, fast/slow 5 up 22 back rest 2) scaled by the cells when the end is set, the conveyor lift 47, the slime creature 54 | FIXED_AND_VERIFIED (the moving mechanisms); the other nine groups compared with free-eggbert's decdesign.cpp menus 2026-11-03: every group's entries in the PC editor's order and number (scenery 11, technical 8, rock 10, buildings 10, hazards 12, treasures 9 with the personal bomb 200, keys and progression 10, vehicles 5, characters 8); two readings to confirm: the characters' four coloured Blupis (the PC editor's multiplayer starts, which the port does not have) place the personal bombs 200-203, and 'closed while not all chests are found' places the hub door | `VerifyGEWorldEditor`; the owner's look at those two |
| ED-0004 | Play-test -> stop -> edit -> save -> reload | — | done (completion plan §10) | FIXED_AND_VERIFIED | live 2026-11-03 (`tools/qa_steps/editor_playtest.txt`): a block placed, the dice saves and starts the play-test, the pause menu's Back reloads the same saved world in the editor (`plans/evidence/editor-playtest-back.png`); the game log shows the editing, play-test and editing loads of custom_001 |
| ED-0005 | Undo/redo across every edit including metadata | — | `WorldEditor` command history | FIXED_AND_VERIFIED | `VerifyGEWorldEditor` 2026-11-03: one history mixing music, spawn point, BigDecor, a turned tile and an object's end and timing undoes to the start and redoes to the end step by step (blocks, objects, sky, cycling and box fill were covered before) |
| ED-0006 | Variant cycling of the BigDecor families (menu 1: trees, palms, plants, houses, machine and kid backgrounds, slime, palace) | `PlaceDecor1` entries 0-8 (random families) | `BigDecorVariantFamilies()`; a click again on the cell where the previous click put a BigDecor cycles it (the ray meets the same cell); Ctrl places a random one | FIXED_AND_VERIFIED (`VerifyGEWorldEditor`; live 2026-11-04, `tools/qa_steps/editor_bigdecor_cycle.txt`, `plans/evidence/editor-bigdecor-cycle-live.png`: three clicks on one cell showed three trees of the family in turn, and the saved world holds the one BigDecor, the last, 22) | |
| ED-0007 | Placing or removing a tile re-tiles it and its four neighbours (`Decor::AdaptBorder` -> `AdaptMidBorder`: a 4-bit mask below/above/right/left by `IsRightBorder`, looked up in `Tables::table_adapt_decor`, nine families of sixteen; random swaps for 35/156, 251-253, 254/255, 347/362, 348/363; the blitz column 304/305 grown below) | `mobile-eggbert` Decor.cpp:10713-10960 (not called in the port: the PC editor's) | `Editor/AutoTile.hpp` (2026-11-03): `IsRightBorder` ported rule by rule, `table_adapt_decor` transcribed, the random twins, the lightning column; run after every placement (a cycled variant too, as the PC editor's `PlaceItemFromMenu*`), every erase (`DeleteCel`) and each cell of a box fill, one undo step; right = +X or +Z, left = -X or -Z (MR-0005). Not carried over: the cheese/cave edge plates the original lays in empty cells round a mass (`table_adapt_fromage`; reconsidered 2026-11-04 after Q-004/Q-012: the edge plates the worlds place are now their scenes' reliefs, but laid by the editor in every empty cell beside a mass they would stand before every face of every corridor in 3D, where the mass's own faces now show its scenes' lumps -- an intentional 3D adaptation) and the deletion of an object in a cell made solid | FIXED_AND_VERIFIED | `VerifyGEWorldEditor` (a lone rock 147, bordered on Z 151, the bar's mounts, the bolt); live 2026-11-04 (`tools/qa_steps/editor_autotile.txt`, `plans/evidence/editor-autotile-live.png`): three rocks placed in slot 1 became 153, 151 and 152 by their neighbours, and the cells under them were re-tiled (rock 35 -> 37, grass 108 -> 107), read back from the saved `custom_001.vwr` |
| ED-0008 | A moving object's end by the next click (the PC editor's `m_liftEnd`: a lift or creature placed from the moving-mechanisms menu, the next click elsewhere sets its end and multiplies its step by the cells, a click on its start cycles its look) | `blupi_recon` games/sb2/src/decdesign.cpp:479-506, 508-743 | `WorldEditor` `pendingEnd_`, `WithEndAt` (2026-11-04, OR-0072): the next click sets the end, its step scaled as the T key's; a click on it again or Escape leaves it still; a prompt over the view. Not carried over: the click on its start cycling the lift's look (Galaxy's lifts are one model) or the enemy Blupi's colour channel (one sheet, OR-0063) | FIXED_AND_VERIFIED (`VerifyGEWorldEditor`: the end two up by the next click, no second lift; live 2026-11-04: placed and ended, saved (49, 5, 26) -> (49, 7, 26)) | |
| ED-0009 | Galaxy's editor additions for 3D (editor UX, no new gameplay): V cycles the aimed block's variant (its family, else its re-tiling row's edge pieces); O turns a model with a front a quarter at a time (a plate, fan or wind tile between its axes, M3D-0005); H shows the keys | owner reports OR-0070/OR-0071 (2026-11-04) | `WorldEditor::HandleVariant`, `HandleTurn` (`PlateRotationMetadata.hpp` `QuarterTurns`), `StatusLines` | FIXED_AND_VERIFIED (`VerifyGEWorldEditor`; live: H's keys, V on a grass end saved) | |

## ED-0001 — the original's variant cycling (traced 2026-11-01)

Sources (read only): `blupi_recon/games/sb2/src/decdesign.cpp:82-97` (`PlaceDecor`, Speedy Blupi 2,
decompiled), `blupi_recon/port/games/sb{1,2}/game/editor_wide.cpp:45-83` (`BuildDown`,
`PortEditorSideMouse` from `CEvent::BuildDown`/`BuildMove`, the same in Speedy Blupi 1 and 2);
`free-eggbert/src/decdesign.cpp:50-66` (`PlaceMenuItem`, an earlier reconstruction of the same
function, its flags differently named).

- Each palette entry of a family with variants owns a table `{n, icon1 .. iconN}` and a remembered
  index `m_lastDecorIndexes[k]` (kept across cells and clicks, per family).
- Mouse button **pressed** on a cell (`WM_LBUTTONDOWN` -> `BuildDown(..., bMix = TRUE)`): if the cell
  already holds the family's remembered variant, the index advances by one (wrapping to 0 after
  the last) and the next variant is placed; otherwise the remembered variant is placed (no advance).
  So the first click places the current variant, each further click on the same cell cycles it.
- **Dragging** with the button held (`WM_MOUSEMOVE` -> `bMix = FALSE`): paints the remembered
  variant on every cell crossed, never cycling.
- **Ctrl + click** (`MK_CONTROL`): families flagged random (`bRand`: trees, palms, plants, houses,
  machine and kid backgrounds, slime, palace, square and special tech blocks, mechanical squares, ...)
  get a random variant; the others the remembered one, no cycling.
- **Right button**: deletes the cell (decor, big decor and any MoveObject there).
- Families without a table (e.g. the marine plant, the pipes) place a single icon.
- The remembered index persists for the session (the next new cell gets the variant last cycled to).

Galaxy equivalent to build: the same rule per palette entry that has variants, in 3D (the "cell"
is the targeted voxel): a click on a block that holds the entry's current variant cycles it; a
held-button drag (box fill excluded) paints without cycling; each step one undoable command; the
icon and only the icon changes (a variant must stay in its family: never a hazard for a harmless
tile). Tables: the variants are the original's (`dectables.cpp` / `design_tables.h`), checked
against Galaxy's palette groups (`PaletteCategories.cpp`) family by family.
