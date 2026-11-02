# For the owner (decisions, tools, files)

## MR-0001 — the 56 `.max` files need 3D Studio MAX (BLOCKED)

- Files: `originals/max/` (38), `originals/Blupi/max/` (12: `blupi1-3.max`, `blupi-marche*.max`,
  `blupi-stop.max`, `bug.max`), `originals/Element/` (6, e.g. `shield3.max`). Full list:
  `plans/audit/asset_sources.csv`, `ext == .max`.
- What they are: OLE2 compound files written by Kinetix 3D Studio MAX; 14 carry the stamp
  "3D Studio MAX: 3.1", 42 carry no readable version string (older R1/R2 files are likely).
  No open reader exists for this format; Blender does not import `.max`.
- What would unblock them: 3D Studio MAX able to open R1-R3 files (3.1 or a later release that
  still reads them; current Autodesk releases may refuse files this old — unverified). For each
  file: export every mesh with its materials and UVs as `.3ds` (or FBX/OBJ), and its animation
  (keyframes, at 20 fps) as FBX; units as stored, Z up as stored (the converter handles axes);
  texture paths relative. Put the exports in a new directory outside `speedyblupi-data` (e.g.
  `../speedyblupi-max-exports/`, mirroring the paths); Galaxy will read them with a converter
  like `tools/rd4_to_gltf/` and compare against the sprites as for the Ray Dream scenes.
- 2026-11-01: the owner offered `/rv/data/library/openeggbert/drive.openeggbert.com/other/max_studio`
  (`3dsmax3.1.iso`, R3.1 from Kinetix, and a key file). Setup was started in the XP VM (network
  still detached) and stopped at the licence agreement, unaccepted: the serial in the key file is
  the generic one that circulates with key generators and the ISO carries a `CRACKS` folder
  (`PATCHER.EXE`), so it looks like an unlicensed copy; nothing was installed, the VM was shut
  down and the ISO detached. Needed to continue: confirmation of a genuine licence (the serial and
  authorisation code Autodesk/Kinetix issued for it) or original media; the copy-protection crack
  will not be used in any case (the `SENTINEL` folder suggests a hardware lock may be required).
- Never upload them to an online converter (not authorised).
- Until then: no model built from a `.max` is claimed; the shields stay billboards (decision of
  2026-10-24) and Blupi comes from the Ray Dream scenes.

## MR-0002 — earlier questions still open (from `handoff.md`)

- D11: the list of models that look wrong.
- The charge (31) and the shield as billboards.
- BigDecor at the original's size (2-2.25 blocks) or one block.

## MR-0003 — how the original's "click again to cycle the variant" maps onto 3D placement — DECIDED 2026-11-01: the proposed default

- The original (ED-0001, traced in `editor_parity.md`): pressing the button on a cell that already
  holds the family's current variant replaces it with the next variant; dragging paints without
  cycling.
- Galaxy places a block in the empty cell *in front of* the clicked face, so the 2D "same cell"
  has two 3D readings that conflict: clicking the block just placed (which is what a second click
  without moving the mouse does) either cycles it (the original's feel) or stacks another block on
  it (today's behaviour, how towers and walls are built).
- Proposed default: a click whose target block is the one placed by the previous click, of a
  family with variants, and still holding that variant, cycles it (an undoable step); any other
  click places. Stacking the same family then means placing on a neighbouring face first, or
  Ctrl+click (the original's Ctrl places without cycling). Alternative: cycle only with a modifier
  (e.g. Alt+click) and keep plain clicks stacking.
- Owner, 2026-11-01: "ano klik na kostku polozenou predchozim kliknutim prepne jeji variantu" (yes,
  a click on the block placed by the previous click switches its variant). Implemented (ED-0001).

## MR-0004 — a human play pass of the 2026-11-01 changes (what to try, where)

Machine checks pass for each (tests, playthroughs, measured windows); only a person can judge feel.
- Crates (D60): mission 012 from its start (`--mission 12`): push, stop half way (it finishes its
  cell), pull with crouch + Down, jump onto a crate right after pushing it.
- Lifts (BD-0006): mission 061, the shuttle ladder in shaft A (jump west while the shuttles dwell
  at an end); mission 113's gallery lifts; 024, 062, 073.
- 066's blupit (LV-0040): room B1 on the middle storey, cross its line of fire.
- Editor (ED-0001/0006, MR-0003): place a fan, click it again (it turns), Ctrl+click to place
  another; a tree likewise.
- Looks (VIS-0069/0127/0077/0078): lava in 062 and 083, fans in 051 and 999, pillars in 023 and 094;
  from low angles too. Before/after images: `plans/evidence/lava-before-after.png`,
  `fan-before-after.png`.

## MR-0005 — the original editor's auto-tiling in 3D (ED-0007) — OPEN

- The original's editor re-tiles a placed cell and its four neighbours by which of them are
  "borders" (below, above, right, left), choosing each family's variant from
  `Tables::table_adapt_decor` (grass edges, rock edges, the bar's mounts, cheese, ...).
- In 3D, above and below are Y; right and left would be X or Z or both. Galaxy's cubes draw each
  face from the cell's icon, and the worlds are hand-built with chosen icons.
- Proposed: apply the vertical half only (above/below bits from Y, the horizontal bits as if
  bordered), so a block's top and underside variants follow what is over and under it, as the
  bar's mounts already do at draw time; keep every placed icon otherwise. Alternative: leave
  placement as is (today), the owner choosing variants by cycling (ED-0001).

