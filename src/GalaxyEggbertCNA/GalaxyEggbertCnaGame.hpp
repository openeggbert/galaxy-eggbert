#pragma once

#include "Game/GEWorldRuntime.hpp"
#include "Game/GETileAtlas.hpp"
#include "Game/GETerrainRenderer.hpp"
#include "Game/GEBlupiController.hpp"
#include "Game/GEObjectIcons.hpp"

#include <Easy3D/BillboardMeshRenderer.hpp>
#include <Easy3D/Camera3D.hpp>
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
    };
}
