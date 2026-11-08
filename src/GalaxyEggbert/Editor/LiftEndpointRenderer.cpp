#include "LiftEndpointRenderer.hpp"
#include <GalaxyEggbert/Game/DirectionalCubeTiles.hpp>
#include <GalaxyEggbert/Game/TexturedMesh.hpp>
#include <GalaxyEggbert/Game/TileAtlas.hpp>
#include <GalaxyEggbert/Game/WorldRuntime.hpp>
#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp>

namespace GalaxyEggbert::Editor
{
void LiftEndpointRenderer::Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice &device,
                                const CnaEngine::Graphics::Camera &camera, const Worlds::World &world)
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Graphics;
    auto markers = BuildLiftEndpointMarkers(CollectMoveObjects(world));
    if (markers.empty())
    {
        markers_.clear();
        mesh_.reset();
        return;
    }
    if (!effect_)
    {
        texture_ = std::make_unique<Texture2D>("Content/icons/object-m.png", device);
        effect_ = std::make_unique<AlphaTestEffect>(device);
        effect_->setTextureProperty(texture_.get());
        effect_->setVertexColorEnabledProperty(false);
        effect_->setAlphaFunctionProperty(CompareFunction::GreaterEqual);
        effect_->setReferenceAlphaProperty(80);
    }
    if (!mesh_ || markers != markers_)
    {
        markers_ = std::move(markers);
        auto mesh = Game::MakeTexturedMeshBuilder();
        const Game::TileAtlas atlas;
        for (const auto &marker : markers_)
        {
            CnaEngine::Graphics::BoxFaces faces;
            Game::ConfigureDirectionalCubeFaces(marker.icon, atlas.GetTileUv(marker.icon),
                                                atlas.GetTileUv(107), faces);
            Game::AddDoubleSidedCenteredBox(mesh,
                                            {marker.x - Game::WorldRuntime::kWorldCenterX, marker.y,
                                             marker.z - Game::WorldRuntime::kWorldCenterZ},
                                            faces);
        }
        mesh_ = Game::UploadTexturedMesh(device, mesh.Mesh());
    }
    effect_->setWorldProperty(Matrix::getIdentityProperty());
    effect_->setViewProperty(camera.View());
    effect_->setProjectionProperty(camera.Projection());
    // Depth writes hide the rear paint behind opaque foreground paint, while alpha-test
    // holes show the rear edges. Every face is double-sided, including from inside.
    device.setBlendStateProperty(BlendState::Opaque);
    // WorldEditor draws after the host disables depth for its HUD. The explicit
    // CNA depth switch is independent of DepthStencilState::Default.
    device.SetDepthTestEnabled(true);
    device.setDepthStencilStateProperty(DepthStencilState::Default);
    mesh_->Draw(device, *effect_);
    device.SetDepthTestEnabled(false); // subsequent editor controls are screen overlays
}
} // namespace GalaxyEggbert::Editor
