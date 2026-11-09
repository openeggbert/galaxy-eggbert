#pragma once

// The 3D effects' shaders (the Setup's "3D effects"): the water's moving surface and the lava's glow,
// drawn on the terrain's own water and lava meshes in place of its BasicEffect. Lit and fogged as the
// BasicEffect lights the terrain (TerrainLighting.hpp), so the rest of the world matches.
//
// GLSL source compiled at runtime by CNA's ShaderEffect: desktop GLSL 3.30 or GLSL ES 3.00 (WebGL 2,
// Android), picked from the live device. Where neither compiles (another renderer), Ready() is false
// and the terrain keeps its own effect.

#include <GalaxyEggbert/Game/SceneFrame.hpp>
#include <GalaxyEggbert/Game/UltraAssets.hpp>

#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp>

#include <memory>
#include <string>

namespace GalaxyEggbert::Game
{
    // The GLSL version line (and ES precision) the live device wants; empty when it takes no GLSL.
    [[nodiscard]] std::string GlslPreamble(const Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);

    // A ShaderEffect from vertex and fragment bodies (no version line), or nullptr with the reason
    // written to the console once.
    [[nodiscard]] std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> CompileShaderEffect(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const char* name, const std::string& vertexBody,
        const std::string& fragmentBody);

    class SurfaceShaders
    {
    public:
        // Compiles both on first use; false when the device cannot (they are then never tried again).
        bool Prepare(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        // Takes the terrain effect's camera and texture and the frame's fog and clock; `ultra` (Ultra's
        // lending, nullptr otherwise) switches both to Ultra's programs for this frame.
        void Update(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                    const Microsoft::Xna::Framework::Graphics::BasicEffect& terrain, const SceneFrame& frame,
                    const UltraSceneAssets* ultra = nullptr);

        // Binds their textures again. CNA binds a ShaderEffect's texture when it is set, not when it draws:
        // the water draws last of all (after the models, each binding its own), so it sampled whatever
        // texture came last -- the water's colour changed with the view, opaque (reported).
        void BindTextures(const Microsoft::Xna::Framework::Graphics::BasicEffect& terrain,
                          const UltraSceneAssets* ultra = nullptr);

        [[nodiscard]] Microsoft::Xna::Framework::Graphics::ShaderEffect* Water() const noexcept
        {
            return m_ultraActive ? m_ultraWater.get() : m_water.get();
        }
        [[nodiscard]] Microsoft::Xna::Framework::Graphics::ShaderEffect* Lava() const noexcept
        {
            return m_ultraActive ? m_ultraLava.get() : m_lava.get();
        }

    private:
        void SetCommon(Microsoft::Xna::Framework::Graphics::ShaderEffect& effect,
                       const Microsoft::Xna::Framework::Graphics::BasicEffect& terrain, const SceneFrame& frame);

        bool m_tried = false;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_water;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_lava;
        bool m_ultraTried = false;
        bool m_ultraActive = false;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_ultraWater;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_ultraLava;
    };
}
