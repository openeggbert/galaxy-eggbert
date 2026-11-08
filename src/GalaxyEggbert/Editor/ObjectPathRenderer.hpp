#pragma once
#include "ObjectPathMarkers.hpp"
#include <CnaEngine/Graphics/Camera.hpp>
#include <CnaEngine/Graphics/GpuMesh.hpp>
#include <Microsoft/Xna/Framework/Graphics/AlphaTestEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <memory>

namespace GalaxyEggbert::Editor
{
class ObjectPathRenderer
{
  public:
    void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice &device,
              const CnaEngine::Graphics::Camera &camera, const Worlds::World &world);

  private:
    std::vector<ObjectPathMarker> markers_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> texture_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::AlphaTestEffect> effect_;
    std::unique_ptr<CnaEngine::Graphics::GpuMesh> mesh_;
};
} // namespace GalaxyEggbert::Editor
