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
- **Status: fixed** in libcna/cna `next` `41944dd8d` (2026-10-23, the owner chose full support;
  local commit, not pushed yet). The converter writes each part's `baseColorFactor` as
  `"diffuseColor": [r, g, b, a]` and its `baseColorTexture` as the part texture (white when there
  is none); `SkinnedModelEXT::PartEXT::DiffuseColor` (white by default) is loaded from the manifest
  but not applied automatically: a draw sets `setDiffuseColorProperty`/`setAlphaProperty` per part.
  Galaxy Eggbert does not use the field yet, because CI pins CNA `900d0db3b`, which lacks it;
  `tools/rd4_to_gltf/export_cna_avatar.py` keeps writing each part's colour into its texture, so
  the result is the same with either CNA (`avatars3d/blupi/`'s manifest already carries valid
  `diffuseColor` fields, which the older loader ignores).

## UB-003 — CNA `convert_avatar.py`: avatars keep glTF's winding and draw inside out

- **Repository / file:** libcna/cna (branch `next`, HEAD `8f08a82cc`),
  `tools/avatar_asset_pipeline/convert_avatar.py`, `_write_index_buffer()`.
- **Found:** 2026-10-24, from the user's report that the real Blupi's face "slides over his body"
  and that he walks backwards or sideways.
- **Problem:** glTF front faces are counter-clockwise; XNA's (and CNA's) default
  `RasterizerState.CullCounterClockwise` culls exactly those, treating clockwise triangles as
  front faces like XNA content. The converter copies glTF's index order unchanged, so a converted
  avatar drawn with the default state shows the inside of its far side: a face painted on the front
  appears mirrored from behind, the nose disappears behind it, and turning the model looks like
  the face sliding around the body. CNA's own avatar example sidesteps it with
  `RasterizerState::CullNone` (`modules/graphics/examples/skinned_model_attach_part_integration_test.cpp`,
  "needs CullNone").
- **Reproduction:** convert any closed glTF mesh and compute the signed volume of a part from its
  `.verts.bin`/`.idx.bin`: it is positive (counter-clockwise outward). Galaxy Eggbert's
  placeholder fox measured +66488 before the fix here.
- **Suggested fix:** swap the last two indices of every triangle when writing `.idx.bin` (glTF
  counter-clockwise to XNA clockwise), and drop the `CullNone` workaround from the example.
- **Galaxy Eggbert workaround:** `tools/rd4_to_gltf/export_cna_avatar.py` turns each part's
  triangles clockwise (and leaves them alone if a future converter already did), the placeholder
  fox's indices were turned once by hand, and the game sets `CullCounterClockwise` explicitly
  before drawing the model.
- **Status: fixed** in libcna/cna `next` `f8e2b8cc6` (2026-10-24; local commit, not pushed yet,
  pushes are the CNA owner's decision). `_write_index_buffer` writes every triangle as (a, c, b)
  and refuses non-triangle-list primitives; `test_convert_avatar.py` pins the order. Re-verified
  here the same day: with it, `export_cna_avatar.py` reports every part "already clockwise" and
  produces files byte-identical to `avatars3d/blupi/`. Kept as is in CNA, by design: the example
  quad built in code still draws with `CullNone`, and CNA's glTF `Model` importer deliberately keeps
  glTF's counter-clockwise order (its `docs/gltf-conventions.md`, GLTF-423), so only the avatar
  converter flips.

## UB-004 — CNA: `.skinnedmodel.json` part file names are read without decoding JSON escapes

- **Repository:** `../../libcna/cna` (seen at `ba707082e`, 2026-10-24).
- **Observed:** a part named after a Ray Dream texture "Cône" made the game abort at load:
  `ContentLoadException: Cannot open binary file: .../bd_enfant01_enfant01/bd_enfant01_rd_tex_p04_C/u00f4ne.verts.bin`.
  CNA's own `tools/avatar_asset_pipeline/convert_avatar.py` writes the manifest with Python's
  `json.dump` defaults, so non-ASCII characters become `\u00f4` escapes, while the files on
  disk keep the real name (`..._Cône.verts.bin`). The skinned-model reader takes the part's
  `vertices`/`indices`/`texture` strings with `ExtractJsonStringField`
  (`modules/content/src/Xna/BuiltinContentLoaders.cpp`), which returns the raw characters
  between the quotes without decoding escapes; the backslash then reads as a path separator.
  CNA's real JSON parser (`CNA/Internal/Json.hpp`) does decode `\uXXXX`.
- **Reproduce:** convert any glTF whose mesh name has a non-ASCII character with
  `convert_avatar.py`, load the directory with `SkinnedModelEXT`.
- **Workaround here:** the converter gives materials, and so meshes and part files, ASCII-only
  names (`shading.ascii_name`: "Cône" -> "Cone"), 2026-10-24.
- **Fix upstream (suggested):** read those fields with `ParseJson` (or decode escapes in
  `ExtractJsonStringField`), or have `convert_avatar.py` write `ensure_ascii=False`.
- **Status: fixed** in libcna/cna `next` `4bfe6a740` (2026-11-02; local commit, not pushed yet,
  pushes are the CNA owner's decision). The `.skinnedmodel.json` reader parses the whole manifest
  with `ParseJson` and reads every field (`skeleton`, part `name`/`vertices`/`indices`/`texture`,
  animation `name`/`clip`) decoded; a malformed manifest now raises `ContentLoadException`.
  CNA's `ContentManagerSkinnedModelTest.EscapedNonAsciiPartNamesAndFilesAreDecoded` loads a part
  spelled `"Cône"` from `Cône.verts.bin`. Not re-verified here with a real avatar yet, so the
  ASCII-name workaround stays; it can go once CI pins a CNA that contains the fix.

## UB-005 — cna-engine: `AddPlate` and `AddCrossedPlates` draw their region upside down, one side mirrored

- **Repository:** `../../libcna/cna-engine` (branch `develop`, HEAD `f68e7c3`; `MeshBuilder.cpp` last
  changed in `e56fa0f`).
- **File:** `src/Graphics/MeshBuilder.cpp`, `MeshBuilder::AddPlate` and `AddCrossedPlates`.
- **Found:** 2026-10-26, from a report of world 7's author that the digits of the hub signs read
  mirrored; live, the signs also stood upside down (the post on top).
- **Problem:** both build the corners bottom-left, bottom-right, top-right, top-left and give them
  `(u0, v0)`, `(u1, v0)`, `(u1, v1)`, `(u0, v1)`. In XNA's convention v grows downwards (as
  `AddBox` uses it: its top-left corner takes `(u0, v0)`), so the region's top edge lands at the
  plate's bottom. And `AddDoubleSidedQuad` gives both sides the same texture coordinates, so the
  side seen from the other way shows the region mirrored left to right, although the header says
  "the texture is never mirrored". Seen from the side that reads unmirrored, a plate is turned
  upside down; from the other side it is rotated 180 degrees.
- **Reproduction:** `AddPlate(center, 1, 1, PlateAxis::Z, region)`; on its triangles the vertex
  with the larger Y had the larger v.
- **Fixed upstream:** cna-engine `2a93b7e` (2026-10-27, at the owner's request: no workaround in
  Galaxy): the top corners take v0, a horizontal plate shows its top edge towards -Z from above,
  and `AddDoubleSidedQuad`'s reverse copy takes the coordinates mirrored back; its
  `MeshBuilderTests.PlatesReadUprightAndUnmirroredFromBothSides`. Galaxy's interim workaround
  (`AddUprightPlate`, `AddUprightCrossedPlates` in `TexturedMesh.hpp`, D48 `5fbc237`) was
  removed again; `tools/VerifyTileFaces.cpp` checks cna-engine's plates directly. Re-verified
  live: the hub signs of 060 and 040, the showcase's bolts, the saws of 092 and 999.

## UB-006 — CNA on Android: each content name missed lists the APK's assets through JNI

- **Repository:** libcna/cna (`/rv/data/development/github.com/libcna/cna`, HEAD `5a754159d`):
  `modules/content/src/Xna/ContentManager.cpp` (`TryReadAssetBytes`, the `__ANDROID__` branch for
  relative paths) and `modules/platform/src/Sdl3/Sdl3SystemServices.cpp`
  (`Sdl3FileSystem::TryLoadFileIgnoringCase`, `ResolveAndroidAssetPathIgnoringCase`).
- **What happens:** on Android a relative content path goes to `TryLoadFileIgnoringCase`; when the
  exact name is missing, `ResolveAndroidAssetPathIgnoringCase` calls `AssetManager.list()` through
  JNI once per path component (the APK's asset root first), and each call walks the APK's zip
  entries (`ZipAssetsProvider::ForEachFile`). `Load<Texture2D>("x")` always misses first on
  `x.xnb` before it finds `x.png`, so every texture of a model built from raw images costs several
  listings of the whole APK, even when the file sits in internal storage and SDL would open it
  directly.
- **Seen:** Galaxy Eggbert's APK (2,563 assets, ~620 model textures) in an x86_64 API 35 emulator,
  2026-11-03: after the assets were copied, `LoadContent` ran for more than 15 minutes of CPU with a
  black screen; `simpleperf` put 75 % of the game thread under
  `ContentManager::Load<Texture2D>` -> `TryLoadFileIgnoringCase` -> `AssetManager.list` ->
  `ZipAssetsProvider::ForEachFile`. The emulator ran without KVM (software emulation), which makes
  it extreme; on a phone it is a cost per texture that grows with the APK's asset count.
- **Reproduce:** an Android app with many assets and a `ContentManager` whose `RootDirectory` is
  relative; load textures that exist only as `.png`.
- **Possible fix (CNA's call):** list each asset directory once and cache it, or try the raw image
  extensions with SDL's exact path before the case-insensitive APK walk.
- **Reported** to the cna Claude session on 2026-11-06 at the owner's request (still present at cna
  `be933add6`: `TryLoadFileIgnoringCase` still calls `ResolveAndroidAssetPathIgnoringCase` per
  miss, uncached).
- **Status: fixed** in libcna/cna `next` `0b8424e9b` (2026-11-06: one case-folded
  `AssetManager.list()` per APK directory, cached for the process, `ImmutableAssetPathCache`).
  **Confirmed on Galaxy's APK** the same day: the x86_64 debug APK built at cna `65c1ea4d6` with
  the relative `avatars3d` root (the workaround removed) in the API 35 emulator (software,
  `-accel off`): `simpleperf record` over 90 s of LoadContent finds no sample in
  `libandroidfw.so` (`ZipAssetsProvider::ForEachFile`, 75 % before the fix) and JNI calls under
  0.2 %; the 121 object models loaded in 17 min 21 s from the terrain texture (20 min 35 s with the
  workaround, the same software emulation) and the title scene draws them
  (`plans/evidence/android-emulator-ub006-relative-root.jpg`).
- **Galaxy Eggbert's workaround (removed 2026-11-06):** `avatars3d`'s `ContentManager` pointed at
  the internal-storage copy by an absolute path (`GalaxyEggbertGame::LoadContent`, an
  `__ANDROID__` branch); the root is relative on every platform again. The copy itself
  (`src/GalaxyEggbertCNA/AndroidAssets.cpp`) stays: the game reads `worlds3d/`, `textures3d/` and
  its save with `std::ifstream`, which never sees APK assets.

## UB-007 — CNA EasyGL: the skinned shader needs more vertex uniforms than OpenGL ES 3.0 guarantees

- **Repository:** libcna/cna (HEAD `5a754159d`; file last changed in `cfab65534`):
  `modules/renderers/easygl/include/CNA/Internal/Renderers/EasyGL/GlStockShaderSources.hpp`, the
  skinned vertex shaders (lines ~1049, ~1212, ~1605): `uniform mat4 uBones[72];` beside `uWVP`,
  `uWorld` (mat4), `uNormalMatrix` (mat3), `uFogVector` and the rest.
- **What happens:** 72 bones as `mat4` are 288 uniform vectors on their own; OpenGL ES 3.0 only
  guarantees `GL_MAX_VERTEX_UNIFORM_VECTORS` = 256, and many Android GPUs report exactly that. There
  the program does not link and every `SkinnedEffect` draw is skipped: no skinned model is drawn.
  XNA's own `SkinnedEffect` keeps its 72 bones as 4x3 matrices (216 vectors) for this reason.
- **Seen:** Galaxy Eggbert's APK in the x86_64 API 35 emulator (SwiftShader, 2026-11-03), logcat:
  `[CNA EasyGL 3D] skinned (vertex-lit) link failed: Vertex shader active uniforms exceed
  GL_MAX_VERTEX_UNIFORM_VECTORS (256)`. Blupi and all 120 object models (every one is a
  `SkinnedModelEXT`) are missing; terrain, cubes, sprites and the HUD draw. Desktop OpenGL 3.3 and
  WebGL 2 (1024+ vectors) are not affected.
- **Reproduce:** any `SkinnedEffect` draw on a GLES 3 context whose vertex uniform limit is 256.
- **Possible fix (CNA's call):** bones as 3 `vec4` rows each (`mat4x3`/`vec4 uBones[216]`, as XNA),
  or the array sized from the context's limit.
- **Galaxy Eggbert's workaround:** none possible in the game (the shader is CNA's); on such devices
  Android shows no 3D models until CNA changes. OR-0011, Q-009.
- **Status: fixed** in libcna/cna `next` `22739f89b` (2026-11-03 23:14, CNA-ISLAND-002: the stock
  skinned programs keep their bones as `vec4 uBones[216]`, three rows a bone, and refuse an
  unlinked program). **Confirmed on Galaxy's APK** 2026-11-06: the x86_64 debug APK built at cna
  `be933add6` in the API 35 emulator (SwiftShader, `GL_MAX_VERTEX_UNIFORM_VECTORS` 256) loads the
  121 object models and Blupi, logs no link failure, and draws Blupi, the plants and the objects
  in 3D on the title scene (`plans/evidence/android-emulator-models-ub007.jpg`: 2026-11-03 without
  models, left; 2026-11-06, right).

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
