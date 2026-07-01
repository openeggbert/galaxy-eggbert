#include "GalaxyEggbertCnaGame.hpp"

namespace GalaxyEggbert::CNA
{
    GalaxyEggbertCnaGame::GalaxyEggbertCnaGame()
    {
        Game::getWindowProperty().setTitleProperty("Galaxy Eggbert (CNA)");

        // Touch Easy3D::Camera3D to prove its headers compile and its math
        // links against CNA from this target.
        camera_.SetPosition(Easy3D::Camera3D::Vector3(0.0f, 2.0f, 5.0f));
        const auto view = camera_.GetViewMatrix();
        (void)view;
    }

    void GalaxyEggbertCnaGame::Draw(const Microsoft::Xna::Framework::GameTime& gameTime)
    {
        (void)gameTime;
        getGraphicsDeviceProperty().Clear(0.392f, 0.584f, 0.929f, 1.0f);
    }

    GetTypeNameCPP(GalaxyEggbertCnaGame, "GalaxyEggbertCnaGame")
}
