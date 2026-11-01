# Editor parity (ED-NNNN)

Speedy Blupi 1 / Speedy Blupi 2 editors (`../blupi_recon`, decompiled; `../free-eggbert`
`src/decdesign.cpp`, `src/event.cpp`) against Galaxy's 3D editor (`src/GalaxyEggbert/Editor/`).
Galaxy-only 3D UX is allowed where the original has no equivalent; it must not change gameplay.

| ID | Behaviour | Original evidence | Galaxy | Status | Next |
|---|---|---|---|---|---|
| ED-0001 | Repeated clicks cycle a placed item's variants | traced 2026-11-01, below | `EditorVariants.hpp` (`VariantCycler`) in `WorldEditor::HandlePlacement`: a click on the block the previous click placed cycles it; Ctrl places without cycling (owner's choice, MR-0003) | FIXED_AND_VERIFIED for block families (`VerifyGEWorldEditor`); BigDecor families (trees, palms, houses ...) not yet | BigDecor cycling (ED-0006); a live UI run |
| ED-0002 | Ten editable slots per gamer, three gamers; slot 1 seeded from the showcase | owner requirement (completion plan §10) | done `7eac7d5` | UNREVIEWED (re-verify: create/edit/save/play-test/reopen per slot) | end-to-end script |
| ED-0003 | Palette groups and order (ten groups) | Eggbert 2 editor | `PaletteCategories.cpp` | UNREVIEWED | compare group by group |
| ED-0004 | Play-test -> stop -> edit -> save -> reload | — | done (completion plan §10) | UNREVIEWED | repeatable UI script |
| ED-0005 | Undo/redo across every edit including metadata | — | `WorldEditor` command history | UNREVIEWED | test object metadata edits |
| ED-0006 | Variant cycling of the BigDecor families (menu 1: trees, palms, plants, houses, machine and kid backgrounds, slime, palace) | `PlaceDecor1` entries 0-8 (random families) | BigDecor placement places the entry's icon | UNREVIEWED | the same rule on a BigDecor anchor |

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
