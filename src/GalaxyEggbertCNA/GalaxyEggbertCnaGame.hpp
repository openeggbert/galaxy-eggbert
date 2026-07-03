#pragma once

#include "Game/GEWorldRuntime.hpp"
#include "Game/GETileAtlas.hpp"
#include "Game/GETerrainRenderer.hpp"
#include "Game/GEBlupiController.hpp"

#include <Easy3D/Camera3D.hpp>
#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>

#include <memory>

namespace GalaxyEggbert::CNA
{
    // Minimal skeleton for the planned Direct CNA + Easy3D target: opens a
    // window, parses one world file (no rendering yet), and clears the screen.
    // No assets are drawn, no terrain/Blupi rendering, no gameplay yet — see
    // plan.md, "Direct CNA + Easy3D Migration", for what's next.
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

        // Invisible, collision-only movement placeholder (plan.md
        // E3D-MIG-060) — arrow keys + Space, camera follows it. No sprite
        // yet (E3D-MIG-061..063).
        GEBlupiController blupi_;
    };
}
