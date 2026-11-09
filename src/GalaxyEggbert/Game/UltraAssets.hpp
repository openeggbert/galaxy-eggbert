#pragma once

// What the 3D effects' Ultra level lends the world's shaders each frame (the game owns it; see
// UltraLogic.hpp for how each is made): the sky's light, the terrain atlas's surface maps, and the
// screen's occlusion (UltraFinish.hpp's, of this frame's depth and normals).

#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>

#include <array>

namespace GalaxyEggbert::Game
{
    struct UltraSceneAssets
    {
        // The background picture as the sky's light: its levels, ever softer (reflections, by roughness)...
        Microsoft::Xna::Framework::Graphics::Texture2D* environment = nullptr;
        int environmentLevels = 1;
        // ...and its irradiance (UltraLogic.hpp SkyEnvironment::irradiance).
        std::array<Microsoft::Xna::Framework::Vector3, 9> irradiance{};
        // The terrain atlas's surface (roughness, metalness, cavity) and normals, texel for texel with its
        // colours; nullptr: an even surface.
        Microsoft::Xna::Framework::Graphics::Texture2D* terrainSurface = nullptr;
        Microsoft::Xna::Framework::Graphics::Texture2D* terrainNormals = nullptr;
        // This frame's screen-space occlusion (r ambient, g the sun's contact shadow) at the scene target's
        // size; nullptr: none.
        Microsoft::Xna::Framework::Graphics::Texture2D* screenOcclusion = nullptr;
    };
}
