# The `.a3d` sources (Amapi 3D) — format decoded, reader `tools/a3d/a3d.py` (2026-11-01)

105 files under `speedyblupi-data/originals` (Decor/smooth 42, tentacule 17, palais 15, goutte 14,
lave 6, Blupi 6, Blupi/max 5: `plans/audit/asset_sources.csv`), most with a `.2D` beside them: a
41x41 24-bit BMP preview. They are Amapi 3D files, not a proprietary blob: nothing installed is
needed to read them, only a reader to write. Observed in `Decor/lave/lave-1.a3d` (7564 bytes) with a
throwaway probe (not committed):

- Header `AMAP` + u32 1 (big-endian), then a sequence of chunks: an 8-byte ASCII tag, a big-endian
  u32 size, the body.
- `BLOKSHAP` (one per object) contains `blokgeom`, `blokname` (e.g. "Form1", "Courbe1"),
  `blokinfo` (`laynname` "Scène" / "Courbes", `rnd flip`) and `blokmatx` (60 bytes: 15 floats, a
  transform).
- `blokgeom`: 3 bytes (`00 00 01`), then big-endian u32 counts — Form1: 132, 175, 80, 400; the
  curves: 30, 30 and 4, 4 — then 20 zero bytes, a u32 (an id?), then big-endian floats: vertex
  coordinates in inches (-78.74 = -2 m, -39.37 = -1 m). 400 = 80 x 5 suggests 80 faces stored as
  a count and four indices (quads); 175 is not yet explained (V - E + F = 37 for 132/175/80).
- `Courbe*` shapes in the "Courbes" layer are construction curves (profiles), not render geometry.

Next (Wave D): a reader in `tools/` (like `tools/rd4_to_gltf/`), checked by rendering each
shape from the `.2D` preview's view and comparing silhouettes, then the lava frames
(`Decor/lave/lave-1..5.a3d`, `lave-base.a3d`) and the columns (`Decor/smooth/colonne1/3.a3d`,
`colonnr2.a3d`) as the first candidates for VIS-0069 (lava) and the pillars.

## Decoded (2026-11-01, `tools/a3d/a3d.py`)

- `blokgeom` = 3 bytes, u32 V, u32 E, u32 F, u32 I, 16 zero bytes, u32 (an id), then V vertices of
  four big-endian floats (x, y, z, w; inches, Y up), E edges of two u32 vertex indices, and I u32
  face references, each face a count n and n vertex indices. The sizes add up exactly in every file.
- `blokinfo` starts `layrname` + u32 size + the layer's name ("Scéne", "Courbes").
- All 105 files read: 451 shapes, 203 with faces (15 353 faces); the rest are "Courbes"
  construction curves. `python3 tools/a3d/a3d.py survey ../speedyblupi-data/originals`.
- Seen against their own `.2D` previews (Amapi's front wireframe thumbnails): the lava frames
  `lave-1..5` are a 4 m x 2 m slab whose top is an 11 x 6 height field (0.4 m pitch, 1.0-1.4 m high),
  the wave the original's lava tiles were rendered from, plus a box behind (`Form3`); the columns
  `colonne1/3` are profiled shafts with a capital and a base, 4 m tall.
- Next: per object, compare with the tile sprites (object-m.png 68-72 for lava; 76/375/399 for
  pillars) and with the `.rds` of the same name where one exists (it carries the materials and
  the camera the sprites were rendered with).
