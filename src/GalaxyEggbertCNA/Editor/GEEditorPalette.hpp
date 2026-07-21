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
    // Single vertical column of green buttons at the left edge of the
    // world editor screen (plan.md EDITOR-106, redesigned 2026-07-19 to
    // match free-eggbert's own editor palette style -- user-supplied
    // reference screenshot: one column, real per-item icons on solid green
    // squares, no separate grid elsewhere on screen). Top to bottom: 7
    // fixed action buttons (Undo/Redo/Save/Back/Play-Test/mode-toggle/
    // box-fill), the Confirmed/All tab toggle, a sky-region Prev/Next
    // stepper (EDITOR-111 -- same Left/Right arrow-key idiom as
    // GEWorldEditor's own binding, see ToolbarAction::SkyRegionPrev/Next),
    // a paging row (split into Prev/Next halves), then the current page's
    // block or object icons,
    // one per row. Reuses GETileAtlas::GetTileUv() and the caller's
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
    // Still no text labels: GEInputPad's own label-rendering methods stay
    // private (tightly coupled to its own per-screen font-scale
    // conventions, not worth genericizing for this), and building an
    // independent text renderer remains out of scope. The action buttons
    // are visually identical solid green (distinguished only by position);
    // the mode-toggle/tab-toggle buttons brighten when their "on" state is
    // active, since that state isn't inferable from position. The selected
    // palette icon is marked by a highlighted (gold) backing square, not a
    // caption -- a documented, later-refinable simplification, not a
    // functional gap (every tool already has a working keyboard binding in
    // GEWorldEditor; this is a discoverability convenience on top).
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
        enum class ToolbarAction { None, Undo, Redo, Save, Back, PlayTest, BoxFill, SkyRegionPrev, SkyRegionNext };

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
                           int viewportWidth, int viewportHeight);

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
                 int viewportWidth, int viewportHeight, bool backConfirmArmed = false);

    private:
        [[nodiscard]] const std::vector<int>& CurrentIconList() const noexcept
        {
            if (mode_ == PaletteMode::Objects)
            {
                return showAllTab_ ? allObjects_ : confirmedObjectsFlat_;
            }
            return showAllTab_ ? allIcons_ : confirmedFlat_;
        }
        // How many content rows (block/object icons) fit below the fixed
        // action rows, given the real viewport height -- unlike the old
        // 8x4 grid this replaces, page size is no longer a compile-time
        // constant, since a single column has much less room per page.
        [[nodiscard]] int ItemsPerPage(int viewportHeight) const noexcept;
        [[nodiscard]] int PageCount(int viewportHeight) const noexcept;

        // True when (x,y) lands on ANY palette control -- used to claim
        // the mouse on the press frame (see Update()'s own comment).
        [[nodiscard]] bool HitsAnyControl(float x, float y, int viewportWidth,
                                          int viewportHeight) const noexcept;

        // Rect for fixed action button @p index (0=Undo, 1=Redo, 2=Save,
        // 3=Back, 4=PlayTest, 5=ModeToggle, 6=BoxFill, 7=TabToggle,
        // 8=SkyRegionPrev, 9=SkyRegionNext) -- buttons pair up 2-per-row
        // (index/2 = row, index%2 = left/right half) so 10 actions + the
        // paging row fit in 6 rows instead of 11, leaving real room for
        // content below at this engine's actual (fairly short) default
        // window height. PaletteCellRect uses the same row geometry but
        // full-width, one content item per row.
        [[nodiscard]] GEQuadBatch::Rect ToolbarButtonRect(int index) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect TabToggleRect(int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect PagePrevRect(int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect PageNextRect(int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect PaletteCellRect(int indexOnPage, int viewportWidth,
                                                        int viewportHeight) const noexcept;

        std::vector<int> confirmedFlat_;
        std::vector<int> allIcons_;
        std::vector<int> confirmedObjectsFlat_;
        std::vector<int> allObjects_;
        GETileAtlas tileAtlas_;

        PaletteMode mode_ = PaletteMode::Blocks;
        bool showAllTab_ = false;
        int page_ = 0;
        int selectedBlockType_;
        int selectedObjectType_;
        bool mouseWasDown_ = false;
        bool pressStartedOnPalette_ = false;

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
    };
}
