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
- **Status: fixed** in libcna/cna `next` `6068c1ec4` (2026-10-23; local commit, not pushed yet,
  pushes are the CNA owner's decision). A named mesh with one primitive keeps its name; otherwise
  the primitive index is appended (`BLUPI_0`, `BLUPI_1`) and names duplicated across meshes get a
  further suffix. CNA added `tools/avatar_asset_pipeline/test_convert_avatar.py`. Re-verified here
  the same day: a `.glb` whose mesh `BLUPI` has two primitives now converts to `BLUPI_0` (5374
  vertices) and `BLUPI_1` (280 vertices). The one-mesh-per-material layout stays anyway: its mesh
  names carry each part's colour, which the converted asset otherwise loses (UB-002).

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
- **Status: open, waiting for a decision** (2026-10-23). Reported to the CNA session, which
  declined it for now: it needs a new manifest field and new `SkinnedModelEXT` loading and drawing,
  and the CNA owner currently limits CNA work to defect fixes. Passed to the owner for a decision.

## Recorded elsewhere

These upstream problems were found earlier and are documented in other files; they are listed
here only so this file can serve as the index:

- `easy-gl-resource-smoke-tests` failed inside `../easy-gl` (root cause in `NEXT.md` §5).
  **Fixed** in easy-gl `0b46d35` (2026-07-19: `Texture::bind()` activates texture unit 0), which
  the pinned v0.1.1 contains. Re-verified 2026-10-23 in easy-gl's own `build/` and in
  `build-cna-opengl3`; the CI exclusion and the documented quarantine were removed.
- CNA Vulkan backend: `BasicEffect` with `Alpha < 1` did not render (`build-cna-vulkan`). See
  `NEXT.md`, same section. **Obsolete:** CNA retired the `VULKAN` renderer on 2026-10-13 (it is
  refused at configure time). Vulkan is now reached through `SDL_GPU`. If the alpha problem shows
  up there, send CNA a reproducer for `SDL_GPU`.
- Editor highlight wireframe on OpenGL ES contexts without `GL_NV_polygon_mode`. Recorded as
  UP-014 in libcna/cna-engine's `upstream-issues.md`; see `editor-wireframe-gles-bug.md`. Not an
  upstream bug after all: the defect was Galaxy Eggbert's own `EditorHighlightRenderer`. **Fixed**
  here on 2026-10-23 (`NEXT.md` §3); UP-014 in cna-engine was marked resolved.
