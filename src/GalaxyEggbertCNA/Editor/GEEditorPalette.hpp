#pragma once

#include "GEPaletteCategories.hpp"
#include "Game/GEObjectIcons.hpp"
#include "Game/GEQuadBatch.hpp"
#include "Game/GETileAtlas.hpp"

#include <GalaxyEggbert/def/ObjectType.hpp>

#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace GalaxyEggbert::CNA
{
    // Free-Eggbert-style left category rail at the edge of the world editor
    // screen. Each persistent green square represents one category with a
    // real representative icon. Clicking it opens that category's actual
    // icon choices as a compact strip/grid immediately to its right; it does
    // NOT replace the rail with a long anonymous column. Galaxy Eggbert keeps
    // its own extra controls (including the sky/background Prev/Next pair)
    // in a separate horizontal toolbar above the rail. Reuses
    // GETileAtlas::GetTileUv() and the caller's
    // already-loaded object-m.png texture directly (no separate asset
    // load) for Blocks-mode icons, and the shared GEQuadBatch primitives
    // (extracted from GEInputPad) for all 2D quad drawing/hit-testing.
    //
    // Object-mode icons reuse GEObjectIcons::GetObjIcon() plus its
    // atlas-selection predicates (IsUniformCubeObject/IsObjectMPngSourced/
    // IsExploPngSourced/IsBlupiPngSourcedAtPhase/UsesBlupi1Texture) -- the
    // SAME lookup already used to render real MoveObject billboards during
    // gameplay -- so a page of object cells can draw the real per-type
    // sprite from whichever of object-m.png/element.png/explo.png/
    // blupi.png/blupi1.png it actually lives on, batched per atlas.
    //
    // The Eggbert menu itself remains icon-only. Galaxy's additional
    // placement pad is deliberately labelled X-/X+, Y-/Y+, Z-/Z+ and
    // PLACE through the shared text.png glyph sheet: unlike the source
    // menu icons, those new 3D-axis actions have no unambiguous button.png
    // equivalents. The selected palette icon is marked by a highlighted
    // gold backing square.
    class GEEditorPalette
    {
    public:
        GEEditorPalette();

        // BoxFill reports the click; GEWorldEditor::Update() treats it as
        // an F-key press through the exact same box-fill state machine
        // (first click marks corner A, second click fills) -- no separate
        // state lives in this class for it. SkyRegionPrev/Next (EDITOR-111)
        // are the same idiom as BoxFill: this class only reports the click,
        // GEWorldEditor::Update() treats it identically to its own Left/
        // Right arrow-key stepper.
        enum class ToolbarAction
        {
            None,
            Undo,
            Redo,
            Save,
            Back,
            PlayTest,
            BoxFill,
            SkyRegionPrev,
            SkyRegionNext,
            PlacementXMinus,
            PlacementXPlus,
            PlacementYMinus,
            PlacementYPlus,
            PlacementZMinus,
            PlacementZPlus,
            PlaceSelection
        };

        // Which kind of thing the icon grid is currently selecting
        // (plan.md EDITOR-109). Toggled by the toolbar's 6th button,
        // handled entirely inside Update() -- there's no ToolbarAction for
        // it, same as the Confirmed/All tab toggle, because the caller has
        // nothing to do about it.
        enum class PaletteMode { Blocks, Objects };

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
                           int viewportWidth, int viewportHeight, float elapsedSeconds = 0.0f);

        [[nodiscard]] bool IsNotYetImplementedNoticeVisible() const noexcept
        {
            return notYetImplementedSeconds_ > 0.0f;
        }

        [[nodiscard]] std::uint16_t SelectedBlockType() const noexcept
        {
            return static_cast<std::uint16_t>(selectedBlockType_);
        }

        [[nodiscard]] bool IsObjectMode() const noexcept { return mode_ == PaletteMode::Objects; }

        // The MoveObject type a left-click places while IsObjectMode() is
        // true (plan.md EDITOR-109). Kept independently of
        // SelectedBlockType() so toggling back and forth between the two
        // modes doesn't lose either selection.
        [[nodiscard]] ObjectType SelectedObjectType() const noexcept
        {
            return ToObjectType(selectedObjectType_);
        }

        // @p terrainTexture is the SAME already-loaded object-m.png
        // texture GETerrainRenderer/GalaxyEggbertCnaGame already use --
        // Blocks-mode icons (and the object-m.png-sourced object types,
        // IsUniformCubeObject/IsObjectMPngSourced) are literally the real
        // terrain atlas, just drawn as flat 2D UI quads instead of 3D
        // cubes. @p elementTexture/exploTexture/blupiTexture/blupi1Texture
        // are the other 4 already-loaded object sprite sheets
        // (GalaxyEggbertCnaGame's own objectTexture_/exploTexture_/
        // blupiObjectTexture_/blupi1ObjectTexture_) -- lent here so
        // Objects mode can draw each type's real icon from whichever sheet
        // GEObjectIcons says it actually lives on.
        // @p backConfirmArmed (EDITOR-112) brightens the Back button the
        // same way the mode-toggle/tab-toggle "on" state already does --
        // GEWorldEditor's own unsaved-changes guard armed a 2nd-tap
        // confirm, and this is the only visual signal for it (no text
        // labels anywhere in this class, see its own class comment).
        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                 Microsoft::Xna::Framework::Graphics::Texture2D& terrainTexture,
                 Microsoft::Xna::Framework::Graphics::Texture2D& elementTexture,
                 Microsoft::Xna::Framework::Graphics::Texture2D& exploTexture,
                 Microsoft::Xna::Framework::Graphics::Texture2D& blupiTexture,
                 Microsoft::Xna::Framework::Graphics::Texture2D& blupi1Texture,
                 int viewportWidth, int viewportHeight, bool backConfirmArmed = false,
                 bool hasPlacementPreview = false);

    private:
        [[nodiscard]] const std::vector<PaletteCategory>& CurrentCategories() const noexcept
        {
            // Eggbert 2 has one mixed menu, not separate block/object
            // catalogues. An entry switches mode automatically when chosen.
            return blockCategories_;
        }

        [[nodiscard]] bool IsCategoryOverview() const noexcept
        {
            return !showAllTab_ && openCategory_ < 0;
        }

        [[nodiscard]] const std::vector<int>& CurrentIconList() const noexcept;
        [[nodiscard]] int ContentItemCount() const noexcept;
        [[nodiscard]] int ContentItemId(int index) const noexcept;
        [[nodiscard]] int ContentObjectTypeId(int index) const noexcept;
        [[nodiscard]] bool IsContentItemImplemented(int index) const noexcept;
        // The expanded menu wraps right of the category rail. The available
        // page size follows the actual viewport rather than a fixed grid.
        [[nodiscard]] int ItemsPerPage(int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] int PageCount(int viewportWidth, int viewportHeight) const noexcept;

        // True when (x,y) lands on ANY palette control -- used to claim
        // the mouse on the press frame (see Update()'s own comment).
        [[nodiscard]] bool HitsAnyControl(float x, float y, int viewportWidth,
                                          int viewportHeight) const noexcept;

        // Fixed actions live in one top row, separate from the permanent
        // category rail. This mirrors Free Eggbert's visual hierarchy:
        // categories stay readable at the left while their choices open
        // beside them.
        [[nodiscard]] GEQuadBatch::Rect ToolbarButtonRect(int index) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect FreeDeleteToolRect() const noexcept;
        [[nodiscard]] GEQuadBatch::Rect PlayTestRect(int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect StopRect(int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect TabToggleRect(int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect PagePrevRect(int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect PageNextRect(int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect CategoryButtonRect(int categoryIndex) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect PlacementTouchButtonRect(int index, int viewportWidth,
                                                                 int viewportHeight) const noexcept;
        [[nodiscard]] int PopupColumnCount(int viewportWidth) const noexcept;
        [[nodiscard]] float PopupOriginY() const noexcept;
        [[nodiscard]] GEQuadBatch::Rect PaletteCellRect(int indexOnPage, int viewportWidth,
                                                        int viewportHeight) const noexcept;

        std::vector<PaletteCategory> blockCategories_;
        std::vector<int> allIcons_;
        std::vector<PaletteCategory> objectCategories_;
        std::vector<int> allObjects_;
        GETileAtlas tileAtlas_;

        PaletteMode mode_ = PaletteMode::Blocks;
        bool showAllTab_ = false;
        int openCategory_ = -1;
        int page_ = 0;
        int selectedBlockType_;
        int selectedObjectType_;
        bool mouseWasDown_ = false;
        bool pressStartedOnPalette_ = false;
        float notYetImplementedSeconds_ = 0.0f;

        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> paletteEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> paletteRenderer_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> flatEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> flatRenderer_;

        // One more effect/renderer pair per additional object-icon sheet
        // (object-m.png reuses paletteEffect_/paletteRenderer_ above --
        // same atlas Blocks mode already draws from).
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> elementEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> elementRenderer_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> exploEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> exploRenderer_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> blupiEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> blupiRenderer_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> blupi1Effect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> blupi1Renderer_;
        // Free Eggbert's original menu glyph sheet. Loaded lazily with the
        // rest of the editor palette because the game already owns the
        // GraphicsDevice at Draw() time.
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> buttonTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> buttonEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> buttonRenderer_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> textTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> textEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> textRenderer_;
    };
}
