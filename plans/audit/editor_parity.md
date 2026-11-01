# Editor parity (ED-NNNN)

Speedy Blupi 1 / Speedy Blupi 2 editors (`../blupi_recon`, decompiled; `../free-eggbert`
`src/decdesign.cpp`, `src/event.cpp`) against Galaxy's 3D editor (`src/GalaxyEggbert/Editor/`).
Galaxy-only 3D UX is allowed where the original has no equivalent; it must not change gameplay.

| ID | Behaviour | Original evidence | Galaxy | Status | Next |
|---|---|---|---|---|---|
| ED-0001 | Repeated clicks cycle a placed item's variants | to trace: `PlaceItemFromMenu*`, `decdesign.cpp` | `EditorPalette::Update()` selects a type and closes the category; no cycle found yet | UNREVIEWED | trace the original's exact semantics before implementing |
| ED-0002 | Ten editable slots per gamer, three gamers; slot 1 seeded from the showcase | owner requirement (completion plan §10) | done `7eac7d5` | UNREVIEWED (re-verify: create/edit/save/play-test/reopen per slot) | end-to-end script |
| ED-0003 | Palette groups and order (ten groups) | Eggbert 2 editor | `PaletteCategories.cpp` | UNREVIEWED | compare group by group |
| ED-0004 | Play-test -> stop -> edit -> save -> reload | — | done (completion plan §10) | UNREVIEWED | repeatable UI script |
| ED-0005 | Undo/redo across every edit including metadata | — | `WorldEditor` command history | UNREVIEWED | test object metadata edits |
