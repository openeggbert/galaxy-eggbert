#pragma once

#include "GEPaletteCategories.hpp"
#include "Game/GEQuadBatch.hpp"
#include "Game/GETileAtlas.hpp"

#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace GalaxyEggbert::CNA
{
    // Left-edge toolbar + right-side categorized icon palette for the
    // world editor (plan.md EDITOR-106). Reuses GETileAtlas::GetTileUv()
    // and the caller's already-loaded object-m.png texture directly (no
    // separate asset load) for icon rendering, and the shared GEQuadBatch
    // primitives (extracted from GEInputPad this same milestone) for all
    // 2D quad drawing/hit-testing -- the "reuse the exact same drawing
    // primitive, don't invent a new one" instruction taken literally.
    //
    // Deliberately icon/flat-color only, no text labels this milestone:
    // GEInputPad's own label-rendering methods stay private (tightly
    // coupled to its own per-screen font-scale conventions, not worth
    // genericizing for this), and building an independent text renderer
    // is out of scope here. The toolbar's 5 buttons are visually identical
    // (same flat color, distinguished only by position: Undo/Redo/Save/
    // Back/Play-Test top-to-bottom) and the selected palette icon is
    // marked by a highlighted backing square, not a caption -- a
    // documented, later-refinable simplification, not a functional gap
    // (every tool the toolbar exposes already has a working keyboard/
    // mouse binding in GEWorldEditor; this is a discoverability
    // convenience on top).
    class GEEditorPalette
    {
    public:
        GEEditorPalette();

        enum class ToolbarAction { None, Undo, Redo, Save, Back, PlayTest };

        struct UpdateResult
        {
            ToolbarAction action = ToolbarAction::None;
            // True whenever this frame's click landed ANYWHERE on the
            // palette/toolbar (including a plain icon-selection click that
            // has no ToolbarAction of its own) -- same "inputPadClaimedMouse"
            // idiom GalaxyEggbertCnaGame.cpp's own Play on-screen D-pad
            // already uses: the caller (GEWorldEditor) must skip its own
            // 3D-world left-click place handling this frame when this is
            // true, so the two features don't fight over the same click.
            bool clickConsumed = false;
        };

        // Handles a mouse click against the toolbar/tab-toggle/paging/icon
        // grid (edge-triggered on release, matching GEInputPad's own
        // "!mouseDown && mouseWasDown_" click idiom). The caller
        // (GEWorldEditor) still owns actually performing Undo/Redo/Save,
        // this class only reports the request; clicking a palette icon
        // updates SelectedBlockType() internally.
        UpdateResult Update(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                           int viewportWidth, int viewportHeight);

        [[nodiscard]] std::uint16_t SelectedBlockType() const noexcept
        {
            return static_cast<std::uint16_t>(selectedBlockType_);
        }

        // @p terrainTexture is the SAME already-loaded object-m.png
        // texture GETerrainRenderer/GalaxyEggbertCnaGame already use --
        // palette icons are literally the real terrain atlas, just drawn
        // as flat 2D UI quads instead of 3D cubes.
        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                 Microsoft::Xna::Framework::Graphics::Texture2D& terrainTexture,
                 int viewportWidth, int viewportHeight);

    private:
        [[nodiscard]] const std::vector<int>& CurrentIconList() const noexcept
        {
            return showAllTab_ ? allIcons_ : confirmedFlat_;
        }
        [[nodiscard]] int PageCount() const noexcept;

        [[nodiscard]] GEQuadBatch::Rect ToolbarButtonRect(int index) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect TabToggleRect(int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect PagePrevRect(int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect PageNextRect(int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect PaletteCellRect(int indexOnPage, int viewportWidth,
                                                        int viewportHeight) const noexcept;

        std::vector<int> confirmedFlat_;
        std::vector<int> allIcons_;
        GETileAtlas tileAtlas_;

        bool showAllTab_ = false;
        int page_ = 0;
        int selectedBlockType_;
        bool mouseWasDown_ = false;

        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> paletteEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> paletteRenderer_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> flatEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> flatRenderer_;
    };
}
