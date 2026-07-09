#pragma once

#include "Game/GEWorldRuntime.hpp"
#include "Game/GETileAtlas.hpp"
#include "Game/GETerrainRenderer.hpp"
#include "Game/GEBlupiController.hpp"
#include "Game/GEObjectIcons.hpp"

#include <Easy3D/BillboardMeshRenderer.hpp>
#include <Easy3D/Camera3D.hpp>
#include <Easy3D/CubeMeshRenderer.hpp>
#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace GalaxyEggbert::CNA
{
    // Direct CNA + Easy3D target: loads a hand-authored .vwr world, renders
    // real textured/animated 3D terrain, and moves an invisible,
    // collision-only Blupi placeholder through it (see NEXT.md for current
    // status). No 3D Blupi model exists yet (2026-07-05) — the camera is
    // first-person (nothing to show in third-person), and Blupi's current
    // animation state is signaled via a small 2D indicator in the
    // screen's bottom-right corner instead of an in-world sprite.
    class GalaxyEggbertCnaGame final : public Microsoft::Xna::Framework::Game
    {
    public:
        GalaxyEggbertCnaGame();

        void LoadContent() override;
        void Update(Microsoft::Xna::Framework::GameTime& gameTime) override;
        void Draw(const Microsoft::Xna::Framework::GameTime& gameTime) override;

        GetTypeNameHPP()

    private:
        // Drives terrainEffect_'s View/Projection every frame.
        Easy3D::Camera3D camera_;

        // Parses worlds/world001.txt (plan.md Phase 4).
        GEWorldRuntime worldRuntime_;

        // Real mobile-eggbert background image for worldRuntime_'s skyRegion
        // (2026-07-09, NEXT.md §3 -- mobile-eggbert-reference/
        // 05-backgrounds.md/15-3d-render-mapping-design.md §9.5). Drawn as a
        // single huge camera-facing billboard quad placed far behind the
        // scene (same BillboardBatch/BuildBillboardMesh/BillboardMeshRenderer
        // technique already used for MoveObjects/BigDecor, just one giant
        // quad sized to fill the whole view frustum at that distance) --
        // NOT a 2D SpriteBatch overlay. Tried SpriteBatch first (drawn
        // full-screen before the 3D scene, relying on it not writing depth);
        // that broke live (2026-07-09): EasyGLSpriteBatchBackend::Begin()
        // enables alpha blending and never restores it on End(), and more
        // fundamentally the SpriteBatch/BasicEffect draw paths turned out
        // not to compose safely when SpriteBatch runs BEFORE 3D draws in the
        // same frame (only ever exercised the other order before, e.g.
        // blupiIconBatch_ drawn last) -- the background ended up covering
        // the entire screen with no terrain visible at all. A real,
        // depth-tested 3D quad sidesteps this entirely: normal opaque 3D
        // terrain, being closer, correctly occludes it via the depth buffer
        // like any other geometry, no draw-order fragility. This
        // deliberately does NOT attempt a true wraparound 3D skybox --
        // §9.5 already found the source art (a flat 640x480 image designed
        // for a fixed 2D side-camera) a poor fit for that; one large flat
        // backdrop plane facing the camera is a different, smaller step
        // than projecting the image onto real skybox geometry.
        // `Content/backgrounds/decorNNN.png` is already copied next to this
        // binary (existing mobile-eggbert Content/ copy, CMakeLists.txt) --
        // no new asset-pipeline work needed. backgroundTexture_/Effect_ stay
        // default-constructed (unloaded) if the region's file doesn't exist
        // (e.g. one of the 4 ids no real level ever uses) or before
        // LoadContent() runs; Draw() falls back to the flat device.Clear()
        // color in that case.
        Microsoft::Xna::Framework::Graphics::Texture2D backgroundTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> backgroundEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> backgroundMeshRenderer_;
        bool backgroundLoaded_ = false;

        // Maps block types to object-m.png UV rects (plan.md E3D-MIG-053).
        GETileAtlas tileAtlas_;

        // Static (non-animated) terrain mesh for the loaded world (plan.md
        // E3D-MIG-054). Lazily constructed in LoadContent() since it needs a
        // live GraphicsDevice.
        std::unique_ptr<GETerrainRenderer> terrainRenderer_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> terrainEffect_;

        // object-m.png, loaded directly via CNA's own Texture2D (no
        // mobile-eggbert Pixmap reuse — that class is 2D SpriteBatch-coupled,
        // not usable for this 3D BasicEffect draw path; see easy3d.md §5.2).
        Microsoft::Xna::Framework::Graphics::Texture2D terrainTexture_;

        // Icon 107's grass-top overlay (NEXT.md §8 task 3) — a genuinely
        // separate, galaxy-eggbert-owned asset (textures3d/grass_top.png,
        // copied next to the binary like worlds3d/), NOT part of
        // object-m.png (which is fully re-copied from ../mobile-eggbert on
        // every build, so nothing can be added into it). Needs its own
        // BasicEffect since BasicEffect only binds one texture at a time;
        // drawn via GETerrainRenderer::DrawGrass() right after the main
        // terrain Draw() call.
        Microsoft::Xna::Framework::Graphics::Texture2D grassTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> grassEffect_;

        // Invisible, collision-only movement placeholder (plan.md
        // E3D-MIG-060) — arrow keys + Space, first-person camera follows
        // its facing. No 3D sprite yet (E3D-MIG-061..063) — see blupiIcon_
        // below for the interim 2D stand-in.
        GEBlupiController blupi_;

        // Interim 2D animation-state indicator (bottom-right corner) while
        // no 3D Blupi model exists (2026-07-05) — blupi.png, drawn via
        // SpriteBatch, not a 3D billboard. Lazily constructed in
        // LoadContent() since SpriteBatch needs a live GraphicsDevice.
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::SpriteBatch> blupiIconBatch_;
        Microsoft::Xna::Framework::Graphics::Texture2D blupiIconTexture_;

        // Billboard rendering for worldRuntime_'s parsed MoveObjects
        // (15-3d-render-mapping-design.md §5/§7) — element.png, static phase
        // (no animation yet). Rebuilt every frame since billboard vertex
        // positions depend on the camera (see Easy3D::BillboardMeshRenderer's
        // header comment) — objectMeshRenderer_ is reconstructed in Draw(),
        // not lazily cached like terrainRenderer_.
        Microsoft::Xna::Framework::Graphics::Texture2D objectTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> objectEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> objectMeshRenderer_;

        // Billboard rendering for the 5 confirmed object-m.png-sourced
        // MoveObjects (GEObjectIcons::IsObjectMPngSourced, NEXT.md §3,
        // 2026-07-09) -- same camera-facing billboard technique as
        // objectMeshRenderer_ above, but reuses terrainTexture_
        // (object-m.png) via its own dedicated effect (BasicEffect only
        // binds one texture at a time), same reason as bigDecorEffect_.
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> objectMPngEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> objectMPngMeshRenderer_;

        // Billboard rendering for the 12 confirmed explo.png-sourced
        // MoveObjects (GEObjectIcons::IsExploPngSourced, NEXT.md §3,
        // 2026-07-09) -- explosions/visual effects, a genuinely new texture
        // (not previously loaded anywhere in GalaxyEggbertCNA), same
        // camera-facing billboard technique as objectMeshRenderer_ above.
        Microsoft::Xna::Framework::Graphics::Texture2D exploTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> exploEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> exploMeshRenderer_;

        // Billboard rendering for the 4 confirmed Blupi-skin MoveObjects
        // (GEObjectIcons::IsBlupiPngSourced, NEXT.md §3, 2026-07-09) --
        // ObjectType200 sources blupi.png, ObjectType201/202/203 source
        // blupi1.png (GEObjectIcons::UsesBlupi1Texture) -- two separate
        // textures/effects/renderers since BasicEffect only binds one
        // texture at a time. blupiObjectTexture_/blupiObjectEffect_ are
        // deliberately separate from the existing blupiIconTexture_/
        // blupiIconBatch_ above (that pair is 2D SpriteBatch-coupled for
        // the HUD indicator, not reusable for this 3D BasicEffect path),
        // even though both load the same blupi.png file.
        Microsoft::Xna::Framework::Graphics::Texture2D blupiObjectTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> blupiObjectEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> blupiObjectMeshRenderer_;
        Microsoft::Xna::Framework::Graphics::Texture2D blupi1ObjectTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> blupi1ObjectEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> blupi1ObjectMeshRenderer_;

        // Billboard rendering for worldRuntime_'s parsed BigDecor: cells
        // (NEXT.md §8 task 3, mobile-eggbert-reference/
        // 01-world-file-format.md §2.3 / 15-3d-render-mapping-design.md
        // §9.2 — confirmed non-colliding, Billboard render mode). Unlike
        // MoveObjects, BigDecor uses the SAME icon vocabulary as the main
        // terrain grid (object-m.png via tileAtlas_), not element.png — so
        // this reuses terrainTexture_ through its own dedicated effect
        // (BasicEffect only binds one texture at a time). Only ever
        // non-empty when a world was loaded via LoadFromMobileEggbertFile()
        // (the default .vwr world has no BigDecor concept). bigDecorCells_
        // is the filtered (non-air) cell list, computed once in
        // LoadContent() from the fixed 100x100 grid so Draw() doesn't have
        // to re-scan all 10000 cells every frame — only the camera-facing
        // billboard mesh itself is rebuilt per frame, same reason as
        // objectMeshRenderer_ above.
        struct BigDecorCell
        {
            float worldX;
            float worldZ;
            std::uint16_t icon;
        };
        std::vector<BigDecorCell> bigDecorCells_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> bigDecorEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> bigDecorMeshRenderer_;

        // Platform-lift/crate UniformCube object path (NEXT.md §8 task 3,
        // mobile-eggbert-reference/15-3d-render-mapping-design.md §5's two
        // confirmed "render as a cube, not a billboard" exceptions:
        // ObjectType1/47/48 platform lifts and ObjectType12 crates, see
        // GEObjectIcons::IsUniformCubeObject). Reuses terrainTexture_
        // (object-m.png, the confirmed-correct sheet for these types) via
        // its own effect, same reason as bigDecorEffect_ above. Built once
        // in LoadContent() (static phase only, same simplification as the
        // MoveObject billboards — no per-instance animation timers yet), NOT
        // rebuilt per frame like the billboard renderers, since a
        // world-space cube's vertices don't depend on the camera.
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> objectCubeEffect_;
        std::unique_ptr<Easy3D::CubeMeshRenderer> objectCubeMeshRenderer_;
    };
}
