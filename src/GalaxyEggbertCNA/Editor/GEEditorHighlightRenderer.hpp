#pragma once

#include <Easy3D/Camera3D.hpp>
#include <Easy3D/CubeMeshRenderer.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>

#include <memory>

namespace GalaxyEggbert::CNA
{
    // Translucent overlay tracking either a single block the editor's
    // raycast currently aims at (plan.md EDITOR-102) or a box-fill
    // selection spanning two corners (plan.md EDITOR-105). Rebuilds its
    // mesh only when the shown bounds actually change -- matches this
    // codebase's "rebuild the whole mesh on change, no in-place update
    // API" convention (see GETerrainRenderer's own class comment).
    class GEEditorHighlightRenderer
    {
    public:
        // Shows a highlight cube centered at the given RENDER-SPACE
        // position -- matches GETerrainRenderer's own block-center
        // convention exactly (integer grid index N renders at N, spanning
        // [N-0.5, N+0.5), not [N, N+1)). A no-op rebuild if this is the
        // same cell already shown.
        void ShowCell(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                      float centerX, float centerY, float centerZ);

        // Shows a box spanning the two given RENDER-SPACE block centers
        // (inclusive on both ends, same block-center convention as
        // ShowCell -- a single-cell box has min==max). A no-op rebuild if
        // the same bounds are already shown. Drawn in a distinct color
        // from the single-cell crosshair so a box-fill selection reads as
        // clearly different from ordinary block picking.
        void ShowBox(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                     float minCenterX, float minCenterY, float minCenterZ,
                     float maxCenterX, float maxCenterY, float maxCenterZ);

        // Shows a highlight cube centered at the given RENDER-SPACE
        // position, marking whichever MoveObject the world editor's object
        // tool (G/T/Delete, plan.md EDITOR-110) currently has selected --
        // same block-center convention as ShowCell, a distinct color from
        // both ShowCell's crosshair and ShowBox's fill overlay. Callers use
        // a SEPARATE instance of this class for this (this class tracks
        // only one set of bounds at a time), so a selection highlight can
        // stay visible at the same time as the aim-crosshair/box overlay.
        void ShowSelectedObject(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                float centerX, float centerY, float centerZ);

        // Hides the highlight (Draw() becomes a no-op) until ShowCell()/ShowBox() is called again.
        void Hide() noexcept;

        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                  const Easy3D::Camera3D& camera);

    private:
        void Rebuild(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                    float minCenterX, float minCenterY, float minCenterZ,
                    float maxCenterX, float maxCenterY, float maxCenterZ,
                    float r, float g, float b);

        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        std::unique_ptr<Easy3D::CubeMeshRenderer> mesh_;
        bool visible_ = false;
        bool hasWireframeEdges_ = false;
        bool hasLastBounds_ = false;
        float lastMinX_ = 0.0f, lastMinY_ = 0.0f, lastMinZ_ = 0.0f;
        float lastMaxX_ = 0.0f, lastMaxY_ = 0.0f, lastMaxZ_ = 0.0f;
    };
}
