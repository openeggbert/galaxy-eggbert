# Upstream bugs found while working on Galaxy Eggbert

This file lists defects and limitations found in **other** repositories (CNA, cna-engine,
easy-gl, sharp-runtime, data repositories, ...) while working on Galaxy Eggbert. Galaxy Eggbert
does not modify those repositories unless a task explicitly scopes them in, so each entry records
what was observed, how to reproduce it, and how Galaxy Eggbert works around it until it is fixed
upstream.

Add new entries at the end with the next `UB-NNN` number. When an entry is fixed upstream, keep
it and add the fixing commit; remove the workaround only after re-verifying.

## UB-001 — CNA `convert_avatar.py`: primitives of one mesh overwrite each other

- **Repository:** libcna/cna (local checkout `/rv/data/development/github.com/libcna/cna`, branch
  `next`, HEAD `a24c75b2e`; file last changed in `13dfe0499`, 2026-09-28).
- **File:** `tools/avatar_asset_pipeline/convert_avatar.py`, `convert_body()`, line 207:
  `part_name = mesh.name or f"part{mesh_i}_{prim_i}"`.
- **Found:** 2026-10-23, while converting the first Blupi model produced by
  `tools/rd4_to_gltf/` (see its README).
- **Problem:** every primitive of a named mesh gets the mesh's name as its part name. A mesh
  with several primitives (for example one per material) therefore writes all of them to the same
  `<name>.verts.bin`, `<name>.idx.bin` and `<name>.png`; each primitive overwrites the previous
  one, and the manifest lists several parts that point at the same (last) files. The fallback name
  that does include the primitive index is used only for unnamed meshes.
- **Reproduction:** convert a skinned `.glb` whose single mesh `BLUPI` has two primitives
  (yellow body parts, black eyelashes). The converter reports "21 bones, 2 parts", but
  `BLUPI.verts.bin` holds only the 280 vertices of the second primitive; the 5374 vertices of the
  first are lost.
- **Suggested fix:** include the primitive index whenever a mesh has more than one primitive,
  e.g. `f"{mesh.name}_{prim_i}"` (or the material name).
- **Galaxy Eggbert workaround:** `tools/rd4_to_gltf/rd4_to_gltf.py` writes one mesh with a
  unique name per material, all sharing one skin. This is valid glTF and converts correctly.

## UB-002 — CNA `convert_avatar.py`: material colours and textures are dropped (limitation)

- **Repository / file:** as UB-001, `_write_placeholder_texture()` (line 135) and the part
  entries written by `convert_body()`.
- **Found:** 2026-10-23, same conversion as UB-001.
- **Problem:** every part gets a 4x4 white placeholder texture, and the part entries in
  `avatar.skinnedmodel.json` have no colour or tint field. glTF `baseColorFactor` and
  `baseColorTexture` are therefore lost. The function's own comment says a per-part tint is
  applied at draw time, but nothing in the converted asset says which tint. This is a design
  limitation rather than a bug, but it blocks a textured Blupi (his face is a texture map in the
  original Ray Dream scenes).
- **Galaxy Eggbert workaround:** none yet. The game would have to know the tint for each part
  name (for example `BLUPI_rd_ffff00`, which encodes its colour) until the converter carries
  material data through.

## Recorded elsewhere

These upstream problems were found earlier and are documented in other files; they are listed
here only so this file can serve as the index:

- `easy-gl-resource-smoke-tests` fails inside `../easy-gl`. Quarantined in CI. See `NEXT.md`,
  "Known upstream failures, quarantined", and the root cause in `NEXT.md` §5.
- CNA Vulkan backend: `BasicEffect` with `Alpha < 1` does not render (`build-cna-vulkan`). See
  `NEXT.md`, same section.
- Editor highlight wireframe on OpenGL ES contexts without `GL_NV_polygon_mode`. Recorded as
  UP-014 in libcna/cna-engine's `upstream-issues.md`; see `editor-wireframe-gles-bug.md`.
