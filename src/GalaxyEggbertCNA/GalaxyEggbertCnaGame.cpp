#include "GalaxyEggbertCnaGame.hpp"

#include <iostream>

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

    void GalaxyEggbertCnaGame::LoadContent()
    {
        // Parse-only sanity check (plan.md E3D-MIG-041) — nothing here renders
        // the loaded world yet; see plan.md Phase 5/6 for that.
        const bool loaded = worldRuntime_.LoadFromMobileEggbertFile("worlds/world001.txt");
        if (!loaded)
        {
            std::cout << "GalaxyEggbertCNA: worlds/world001.txt not found or failed to parse "
                         "(run the binary from its own build directory)." << std::endl;
            return;
        }

        const auto& world = worldRuntime_.GetWorld();
        int nonAirBlocks = 0;
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        for (int z = 0; z < blocksPerAxis; ++z)
        {
            for (int x = 0; x < blocksPerAxis; ++x)
            {
                if (!world.getBlock(static_cast<std::uint16_t>(x), 0,
                                     static_cast<std::uint16_t>(z)).isAir())
                {
                    ++nonAirBlocks;
                }
            }
        }

        std::cout << "GalaxyEggbertCNA: loaded worlds/world001.txt — "
                  << "spawn tile (" << worldRuntime_.GetSpawnTileX() << ", "
                  << worldRuntime_.GetSpawnTileZ() << "), sky region "
                  << worldRuntime_.GetSkyRegion() << ", " << nonAirBlocks
                  << " non-air blocks." << std::endl;
    }

    void GalaxyEggbertCnaGame::Draw(const Microsoft::Xna::Framework::GameTime& gameTime)
    {
        (void)gameTime;
        getGraphicsDeviceProperty().Clear(0.392f, 0.584f, 0.929f, 1.0f);
    }

    GetTypeNameCPP(GalaxyEggbertCnaGame, "GalaxyEggbertCnaGame")
}
