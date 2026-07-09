# Real glTF 2.0 support for CNA — analysis

**Status: analysis only, nothing implemented.** Written 2026-07-09 at the user's request, after the
galaxy-eggbert third-person-model work (`NEXT.md` §3, 2026-07-09) surfaced that CNA's only glTF path
today is an offline Python converter. This document is about **CNA's own architecture**
(`../cna` relative to galaxy-eggbert) — galaxy-eggbert has no standing permission to modify `../cna`
(unlike `../easy-3d`), so nothing here should be implemented directly in this repository. It's
written here, in `galaxy-eggbert/gltf.md`, because that's where the user asked for it.

## 1. Executive summary

CNA can already **render** a real GPU-skinned, animated 3D model correctly — proven end-to-end this
session (`../cna/docs/avatar-real-rendering-ext.md`, `SkinnedModelEXT`/`SkinnedEffect`/
`AvatarRenderer`). What it cannot do is **import glTF 2.0 directly**: the only path today is an
offline Python script (`tools/avatar_asset_pipeline/convert_avatar.py`) that a human runs once,
producing a committed, hand-maintained binary bundle. CNA's own roadmap (`NOXNA.md`, tasks N60-N62)
already anticipates real glTF support but marks it "not started" (geometry loader) to "long term"
(skin/animation import).

The recommended shape of a fix is **not** "wait for the full NOXNA PBR/HDR/shadow pipeline" (most of
which N60-N62 nominally sit inside, but don't actually require) — it's a much smaller, self-contained
piece of work: a C++ glTF 2.0 parser (recommend **`cgltf`**, see §4.1) feeding **the runtime
representation CNA already has and has already proven correct** (`SkinnedModelEXT`), replacing the
Python step, not the render path.

## 2. Current state in CNA

- **`SkinnedModelEXT`** (`include/Microsoft/Xna/Framework/Graphics/SkinnedModelEXT.hpp`) — mesh +
  skeleton + named animation clips, GPU-skinnable, deliberately *not* built on XNA's `Model`/
  `ModelBone`/`ModelMesh` (which encode a rigid per-mesh-not-per-vertex bone hierarchy — the wrong
  shape for real skinning). `BoneCount`/`ParentBoneIndices`/`BindPoseLocal`/
  `InverseBindPoseGlobal`, named `Parts` (mesh + VertexBuffer/IndexBuffer/Texture2D), named `Clips`
  (`AnimationClipEXT` = duration + per-bone `BoneTrackEXT` keyframes), and
  `ComputeBoneTransformsEXT(clipName, position, loop, outWorldBones)` to sample a clip into skinning
  matrices.
- **`AvatarRenderer::EnableRealRenderingEXT`/`DrawRealEXT`** (`GamerServices`, a separate
  `CNA_GamerServices` static library gated behind `CNA_ENABLE_NET`) — the actual draw path,
  `World`/`View`/`Projection` properties + `DrawRealEXT(clipName, TimeSpan position, bool loop)`.
  Proven working (`examples/demo_avatar/`, real windowed screenshots) and, as of today, proven again
  independently by galaxy-eggbert's own third-person camera mode.
- **Content format**: `*.skinnedmodel.json` (manifest) + `*.skeleton.bin` + `*.clip.bin`, loaded via
  `ContentManager`'s `SkinnedModelTypeReader`. A small, deliberately hand-rolled JSON parser (no new
  JSON library dependency), matching the existing `ModelTypeReader`/`SpriteFontTypeReader`
  convention.
- **Import path today**: `tools/avatar_asset_pipeline/convert_avatar.py` (Python, `pygltflib` +
  `Pillow`), run once by a human, output committed to the repo as binary blobs. Real, proven,
  bug-fixed (three real bugs found and fixed getting `examples/demo_avatar/` working: manifest-
  relative path resolution, a C++ keyframe-read evaluation-order bug, and — most relevant here — a
  **bone-hierarchy/bind-pose remapping bug**: `build_node_hierarchy()`'s topological reordering
  (needed for `ComputeBoneTransformsEXT`'s `parent[i] < i` requirement) left `inverseBindMatrices`
  and every vertex's `JOINTS_0` indices in glTF's *original* `skin.joints` order, silently skinning
  with the wrong bind pose/vertex weights until fixed). **Any new importer, in any language, has to
  reproduce this exact fix** — it's not a Python-specific gotcha, it's inherent to
  `ComputeBoneTransformsEXT`'s topological-order requirement.
- **Non-indexed primitives are not handled.** Found live this session: Khronos's own "Fox" sample
  model (used as galaxy-eggbert's third-person placeholder) has a single mesh primitive with no
  index accessor at all (implicit sequential vertex order). `convert_avatar.py` crashed on it
  (`read_accessor` indexing `None`); worked around by patching a synthetic index buffer into the
  source `.glb` before conversion, not by fixing the converter. A real importer needs to handle this
  natively.
- **`NOXNA.md`'s own roadmap** already lists this gap:

  | # | Task | Status |
  |---|---|---|
  | N60 | glTF 2.0 loader → CNA `Model`/`Mesh`/`VertexBuffer`/`IndexBuffer` | not started |
  | N61 | glTF PBR material → `PbrMaterial` mapping | not started |
  | N62 | glTF skin / animation import | **long term** |

  N60/N61 target the XNA-shaped `Model` class and the not-yet-built `PbrMaterial`/`PbrEffect`
  system — neither is actually needed for what CNA can already render correctly today
  (`SkinnedModelEXT`/`SkinnedEffect`). N62 is the one item that matters for this analysis, and it's
  explicitly framed as depending on the other two, which this document argues is unnecessary.

## 3. Prior art: MonoScene + SharpGLTF (both MIT-licensed)

The user asked specifically about <https://github.com/vpenades/MonoScene> — whether it can be
"rewritten to C++" and whether it would need improving to support glTF animation.

**Correction on the premise: MonoScene already supports full animation.** Per its own README,
supported features include "PBR effect shaders, full skeleton animation, loading asset models at
runtime (No Pipeline required)" — up to 72 bones with MonoGame's standard `SkinnedEffect`-equivalent
shaders, 128 with its PBR shaders. It does *not* support morph target/vertex morphing ("exceeds
MonoGame's capabilities" — a `SkinnedEffect`-shape limitation, not a glTF-parsing one), WEBP/KTX2
textures, mipmapped textures, or shadows/glow. Development "has largely ceased" (last push 2023) per
the author's own note, but it's feature-complete for exactly the skinning+animation use case galaxy-
eggbert needs, and MIT-licensed (permissive, safe to study/reference).

**Can it be "rewritten to C++"?** Not as a mechanical code port — MonoScene is C#/.NET, built directly
on MonoGame's own class shapes (`GraphicsDevice`, `VertexBuffer`, `Effect`) and on **SharpGLTF**
(also MIT, <https://github.com/vpenades/SharpGLTF>, actively maintained — last push June 2026, 585
stars), a separate, much larger .NET library that does the actual glTF 2.0 parsing/data-model work.
There is no realistic 1:1 C# → C++ transliteration (different memory model, no direct .NET → C++
tooling for this, and SharpGLTF is a large, general-purpose library — bringing over its full surface
would be far more than CNA needs). What *is* directly reusable is the **architecture and the
algorithm**, the same way `convert_avatar.py`'s bug fixes are useful C++-independent knowledge even
though the script itself is Python:

- **SharpGLTF.Runtime's Template/Instance split** (`src/SharpGLTF.Runtime/Runtime/`):
  `ArmatureTemplate`/`ArmatureInstance`, `NodeTemplate`/`NodeInstance`, `DrawableTemplate`/
  `DrawableInstance`, `MaterialTemplate`/`MaterialInstance`, `SceneTemplate`/`SceneInstance`. A
  "Template" is the immutable, shared-once data loaded from the glTF file (mesh, skeleton hierarchy,
  material definitions, animation clips); an "Instance" is the lightweight, per-placed-entity mutable
  state (current world transform, current clip + playback time, computed bone matrices). This maps
  cleanly onto CNA's existing split: `SkinnedModelEXT` already *is* essentially a "Template" (shared,
  loaded once via `ContentManager`); `AvatarRenderer` already plays the "Instance" role for exactly
  one entity. A `GltfSkinnedModelEXT` importer wouldn't need to invent a new template/instance
  concept — it would just need to *fill in* the template CNA already has.
- **`MeshDecoder`/`MeshDecoder.Schema2`**: a dedicated abstraction for turning glTF's various vertex
  attribute encodings (different accessor component types — BYTE/UNSIGNED_BYTE/SHORT/
  UNSIGNED_SHORT/FLOAT, normalized-or-not, interleaved-or-separate buffer layouts, and — directly
  relevant to the Fox bug above — indexed vs non-indexed primitives) into one consistent output
  shape, instead of assuming the single common case. This is exactly the class of robustness
  `convert_avatar.py` is currently missing.
- **`VertexNormalsFactory`/`VertexTangentsFactory`**: computes missing vertex normals/tangents when
  the source glTF doesn't provide them (both are optional per spec) — a real quality detail, not
  just an edge case; plenty of real-world exported models omit tangents.
- **SharpGLTF.Core's extension coverage** (KHR_materials_pbrSpecularGlossiness/unlit/clearcoat/
  transmission/sheen/specular/anisotropy, KHR_texture_basisu, EXT_texture_webp, KHR_lights_punctual,
  EXT_mesh_gpu_instancing, KHR_animation_pointer, and graceful round-trip preservation of *unknown*
  extensions instead of failing) is a good reference for *how much* of the spec a "complete" importer
  eventually covers — not a target CNA needs to match immediately (§6 scopes this down deliberately).

**Bottom line on MonoScene/SharpGLTF**: don't port the code, adopt the architecture (Template/
Instance split, a dedicated mesh-decoding abstraction, missing-data recovery helpers) and treat
SharpGLTF's own source as a reference for glTF 2.0 spec edge cases when writing the C++ importer's
tests (§5).

## 4. Recommended C++ architecture

### 4.1 Parsing library: `cgltf`, not Assimp, not `tinygltf`

| Library | For | Against |
|---|---|---|
| **`cgltf`** (jkuhlmann/cgltf) | Single-header, MIT-licensed, pure C99, zero dependencies (own embedded JSON parser). The most common choice in C/C++ engines (bgfx, raylib, Sokol samples, ...). Fits CNA's own stated preference for minimal-dependency, hand-rolled parsing (the `.skinnedmodel.json` reader was written by hand specifically to avoid a new JSON library — `cgltf` is the equivalent move for glTF itself: already written, already correct, still zero new transitive dependencies beyond it). | Doesn't decode image data itself — fine, CNA's `Texture2D` already goes through SDL_image. |
| `tinygltf` | Header-only C++11, can decode images itself. | Depends on nlohmann/json + stb_image (two more transitive deps), exception-based API, heavier footprint than needed here. |
| `fastgltf` | Fastest (SIMD JSON via simdjson). | Requires C++20 + simdjson; no evidence CNA needs the extra speed for what is, at most, a handful of character models. |
| Assimp | — | Explicitly the wrong tool for this: it's the *cause* of MonoGame's own glTF quality problems (lags behind on glTF adoption, funnels everything through a rigid model shape that loses skinning fidelity — see the earlier session analysis in this conversation). Avoid. |

### 4.2 Where it plugs in — reuse `SkinnedModelEXT`, don't reinvent it

Two viable end states, in increasing order of ambition:

1. **C++ CLI replacement for `convert_avatar.py`** — same offline shape (still produces
   `.skinnedmodel.json`/`.skeleton.bin`/`.clip.bin`, still a build-time step a human runs), but
   written in C++ against `cgltf` instead of Python against `pygltflib`. Lower risk, removes the
   Python/pygltflib/Pillow dependency, and is a natural place to port `convert_avatar.py`'s already-
   proven fixes (topological bone reorder + joint-index remap + inverse-bind-pose-derived bind pose)
   without re-discovering them.
2. **Real runtime import** — a new `ContentManager` type reader (alongside, not replacing, the
   existing `SkinnedModelTypeReader`) that parses `.gltf`/`.glb` **directly** into an in-memory
   `SkinnedModelEXT`, with no intermediate files at all. This is what "quality glTF support" should
   mean in the end: drop a `.glb` into `Content/`, `Load<std::shared_ptr<SkinnedModelEXT>>()` it,
   done — matching what SharpGLTF+MonoScene already give C#/MonoGame users, and what N60/N62 already
   intend. Committed binary blobs like `galaxy-eggbert/avatars3d/blupi_placeholder/*.bin` would
   become unnecessary — the source `.glb` would be the only asset needed.

Recommendation: build (1) first (small, low-risk, immediately removes a real dependency pain point),
then (2) once (1)'s parsing/conversion logic is proven against a real test suite (§5) — (2) is
mostly "call the same conversion code at load time instead of from a CLI `main()`."

### 4.3 New types (`namespace CNA::Graphics`, matching `NOXNA.md` §6's conventions)

Whether gated behind `CNA_NOXNA` or not is a real open question (see §6) — shown here without the
`#ifdef` for clarity, using NOXNA's own plain-setter style (`setX`, not XNA's `setXProperty`, per
`NOXNA.md`'s own code-style example):

```
include/CNA/Graphics/
    GltfImportResult.hpp   — success/error + human-readable diagnostics from a glTF import attempt
    GltfImportOptions.hpp  — e.g. "which node to treat as the skeleton root", unit-scale override

src/CNA/Graphics/
    GltfImporter.cpp       — cgltf-backed: parses a .gltf/.glb, builds a SkinnedModelEXT in memory
                              (or writes .skinnedmodel.json/.skeleton.bin/.clip.bin, for the CLI path)
```

```cpp
namespace CNA::Graphics
{
    struct GltfImportOptions
    {
        // Root node to treat as the skeleton (auto-detected from the first glTF `skin` if unset).
        std::optional<std::string> skeletonRootNodeName;
        // Overrides glTF's mandated meters-per-unit convention if a source file doesn't follow it
        // (some exporters don't) -- see §5's unit-convention item.
        float unitScale = 1.0f;
    };

    struct GltfImportResult
    {
        bool success = false;
        std::string errorMessage; // empty on success
        std::vector<std::string> warnings; // e.g. "primitive 2 has no TANGENT, computed one"
    };

    // Parses a glTF 2.0 file (.gltf or .glb) directly into an existing SkinnedModelEXT -- the same
    // target shape AvatarRenderer::EnableRealRenderingEXT already knows how to draw. Does NOT
    // touch AvatarRenderer/ContentManager itself; a separate ContentManager type reader (§4.2 item
    // 2) would call this to implement real Load<>() support.
    GltfImportResult ImportGltfIntoSkinnedModelEXT(
        const std::string& path,
        const GltfImportOptions& options,
        Microsoft::Xna::Framework::Graphics::SkinnedModelEXT& outModel);
}
```

This deliberately does **not** introduce a new `GltfModel`/`GltfScene` class hierarchy — the whole
point is that `SkinnedModelEXT` is already the correct, proven target shape. A new hierarchy would
only be justified if CNA needed to represent glTF features `SkinnedModelEXT` structurally can't hold
(multiple materials with different shading models per part, morph targets, non-skinned static
scenery with a node graph) — worth deferring until there's a real need, not building speculatively
(this mirrors galaxy-eggbert's own "no premature abstraction" rule, and NOXNA's Model/Mesh path
already covers plain static geometry, so there's no gap there today).

## 5. Concrete quality checklist (what to actually test)

Grounded in the one real bug already found this session (non-indexed primitives) plus SharpGLTF's
own feature surface as a reference for what "complete" looks like:

- [ ] **Non-indexed primitives** (no `indices` accessor — implicit sequential order). Confirmed real:
      Khronos's own Fox sample model has this.
- [ ] **Sparse accessors** (`accessor.sparse` — a base buffer + a list of overridden indices/values,
      used to compress mostly-static vertex data).
- [ ] **All accessor component types**, not just `float`: `BYTE`/`UNSIGNED_BYTE`/`SHORT`/
      `UNSIGNED_SHORT` (normalized or not) alongside `FLOAT`, for positions, normals, UVs, joint
      indices/weights alike.
- [ ] **Animation interpolation modes**: `LINEAR` (already handled, per
      `avatar-real-rendering-ext.md`'s "Lerp translation/scale, `Quaternion::Slerp` rotation"),
      `STEP`, and `CUBICSPLINE` (a fundamentally different keyframe layout — in-tangent, value,
      out-tangent per key, not a single value) — worth explicitly confirming today's Python converter
      even detects which mode a given animation channel uses, since `avatar-real-rendering-ext.md`
      doesn't mention interpolation-mode handling at all.
- [ ] **Multiple primitives per mesh and multiple meshes per model** — the male/female avatar demo
      content already exercises 5 parts; worth a test case with a genuinely multi-primitive single
      mesh too (not currently exercised by either existing content bundle).
- [ ] **Missing normals/tangents** — compute them (face-normal averaging / MikkTSpace-style tangent
      generation) rather than rejecting the file, matching SharpGLTF.Runtime's
      `VertexNormalsFactory`/`VertexTangentsFactory`.
- [ ] **sRGB vs linear texture color space** for `baseColorTexture` specifically (glTF mandates
      sRGB-encoded base color; sampling it as linear washes out/darkens colors) — a common, easy-to-
      miss correctness bug, not a hypothetical one.
- [ ] **Unit/coordinate convention** — glTF mandates +Y-up, right-handed, meters. Confirm CNA's own
      world convention matches (galaxy-eggbert's placeholder-model integration today just hand-picked
      a scale constant empirically, `0.02`, rather than reading/trusting any unit information from
      the source file — a real importer should use the file's own scale where derivable, e.g. from
      node transforms, rather than requiring every consumer to eyeball a constant).
- [ ] **`asset.version` validation** — reject (with a clear message) anything that isn't `"2.0"`
      rather than silently misinterpreting glTF 1.0 (a materially different, dead spec version —
      confirmed in the earlier session discussion that no modern export tooling produces 1.0 anymore,
      so this is a validation/error-message quality bar, not a real compatibility need) or an
      unrecognized future version.
- **Test corpus**: Khronos's own `glTF-Sample-Assets` repo (the same repo the Fox placeholder came
  from) exists specifically to exercise cases like these — sparse accessors, multiple UV sets, morph
  targets, varied interpolation modes are all represented by dedicated sample models there. A real
  importer's test suite should run against a meaningful subset of that corpus, not just whatever
  model happens to be used for a given feature (the Fox's non-indexed-primitive bug would have been
  caught before shipping if this had been done from the start).

## 6. Scope recommendation

**In scope now** (matches what `SkinnedModelEXT`/`AvatarRenderer` can already render correctly):
geometry import (arbitrary accessor encodings, indexed and non-indexed), skin/joint import (with the
already-solved topological-reorder + bind-pose fix carried over), animation clip import (all three
interpolation modes), a single base-color texture per part. This alone would already be strictly
better than what `convert_avatar.py` does today, and needs none of NOXNA's PBR/HDR/shadow/IBL work.

**Explicitly out of scope for this phase** (real, but separate, much larger investments — matches
`NOXNA.md`'s own framing of N61 as depending on `PbrMaterial`/`PbrEffect`, neither built yet): full
PBR material mapping (metallic/roughness, normal/AO/emissive maps, KHR_materials_* extensions),
morph targets (not even MonoScene supports these, for the same "wrong effect shape" reason
`SkinnedEffect` can't either), Draco mesh compression, KTX2/Basis Universal textures, GPU
instancing (`EXT_mesh_gpu_instancing` — ties into `NOXNA.md`'s own separate, also-not-started N50/
N51). None of these block a real, quality glTF-to-`SkinnedModelEXT` importer from being genuinely
useful today.

**Open question, not resolved here**: should this live behind `#ifdef CNA_NOXNA`, or in the always-
built XNA-compatibility layer? Arguments for NOXNA-gating: `SkinnedModelEXT`/`AvatarRenderer` are
already `NOXNA`-tagged (documentation-marker, not compile-guard) CNA-original extensions, not real
XNA 4.0 API surface, so a glTF importer for them is naturally adjacent. Arguments against: unlike the
rest of `NOXNA.md`'s catalog, this needs no PBR/HDR/shadow infrastructure and could be useful to any
CNA game wanting real skinned characters regardless of whether they ever enable `CNA_NOXNA` — this
is really a decision for whoever owns CNA's own roadmap (`../cna`, actively worked on in a sibling
`cna_graphics` checkout per this session's earlier context), not something to presume here.

## 7. Summary answer to "can MonoScene be rewritten in C++, improved to support animations"

Not as a code port (different language/ecosystem, and MonoScene depends on the much larger SharpGLTF
library for the actual parsing). It doesn't need improving for animation support specifically —
MonoScene already does full skeleton animation via SharpGLTF + its own runtime glue, and is MIT-
licensed, so its **architecture** (Template/Instance split, dedicated mesh-decoding abstraction,
missing-normal/tangent recovery) is safe to study and adapt as a design reference. The actual
recommended path for CNA is narrower and lower-risk than "port MonoScene": write a `cgltf`-backed C++
importer that targets CNA's own already-correct `SkinnedModelEXT`/`AvatarRenderer`, carrying forward
the specific bugs `convert_avatar.py` already found and fixed, rather than re-deriving them — first
as a CLI replacement for the Python converter, then, once proven against a real glTF conformance test
corpus, as a true `ContentManager`-integrated runtime loader.

## References

- [MonoScene (vpenades/MonoScene)](https://github.com/vpenades/MonoScene) — MIT, runtime glTF
  loading + full skeleton animation + PBR for MonoGame, no content pipeline. Development
  largely dormant since 2023 but feature-complete for this use case.
- [SharpGLTF (vpenades/SharpGLTF)](https://github.com/vpenades/SharpGLTF) — MIT, actively
  maintained, the actual glTF 2.0 parsing/data-model library MonoScene is built on.
  `src/SharpGLTF.Runtime/Runtime/` is the architecture reference cited in §3.
- [cgltf (jkuhlmann/cgltf)](https://github.com/jkuhlmann/cgltf) — recommended C++ parsing library,
  §4.1.
- [Khronos glTF-Sample-Assets](https://github.com/KhronosGroup/glTF-Sample-Assets) — source of the
  Fox placeholder model and the recommended test corpus for §5.
- `../cna/docs/avatar-real-rendering-ext.md` — the existing `SkinnedModelEXT`/`AvatarRenderer`
  architecture this analysis builds on.
- `../cna/NOXNA.md` — CNA's own NOXNA roadmap, tasks N60-N62.
- `../cna/tools/avatar_asset_pipeline/convert_avatar.py` — the current offline converter and its
  `README.md`'s documented bug history.
