#pragma once

#include <Easy3D/Camera3D.hpp>
#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>

namespace GalaxyEggbert::CNA
{
    // Minimal skeleton for the planned Direct CNA + Easy3D target: opens a
    // window and clears the screen. No content, assets, world loading, or
    // gameplay yet — see plan.md, "Direct CNA + Easy3D Migration", for what's next.
    class GalaxyEggbertCnaGame final : public Microsoft::Xna::Framework::Game
    {
    public:
        GalaxyEggbertCnaGame();

        void Draw(const Microsoft::Xna::Framework::GameTime& gameTime) override;

        GetTypeNameHPP()

    private:
        // Proves Easy3D::Camera3D compiles and links against CNA's math from
        // this target. Not used for rendering yet.
        Easy3D::Camera3D camera_;
    };
}
