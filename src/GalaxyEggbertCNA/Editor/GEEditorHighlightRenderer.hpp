#pragma once

#include <Easy3D/Camera3D.hpp>
#include <Easy3D/CubeMeshRenderer.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>

#include <memory>

namespace GalaxyEggbert::CNA
{
    // Translucent cube overlay tracking whichever block the editor's
    // raycast currently aims at (plan.md section 6, EDITOR-102). Rebuilds
    // its mesh only when the highlighted cell actually changes -- matches
    // this codebase's "rebuild the whole mesh on change, no in-place
    // update API" convention (see GETerrainRenderer's own class comment).
    class GEEditorHighlightRenderer
    {
    public:
        // Shows a highlight cube centered at the given RENDER-SPACE
        // position -- matches GETerrainRenderer's own block-center
        // convention exactly (integer grid index N renders at N, spanning
        // [N-0.5, N+0.5), not [N, N+1)). A no-op rebuild if this is the
        // same center already shown.
        void ShowCell(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                      float centerX, float centerY, float centerZ);

        // Hides the highlight (Draw() becomes a no-op) until ShowCell() is called again.
        void Hide() noexcept;

        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                  const Easy3D::Camera3D& camera);

    private:
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        std::unique_ptr<Easy3D::CubeMeshRenderer> mesh_;
        bool visible_ = false;
        bool hasLastCenter_ = false;
        float lastCenterX_ = 0.0f;
        float lastCenterY_ = 0.0f;
        float lastCenterZ_ = 0.0f;
    };
}
