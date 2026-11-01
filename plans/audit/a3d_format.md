# The `.a3d` sources (Amapi 3D) — what is known (2026-11-01)

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
