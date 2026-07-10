#pragma once

#include <Easy3D/BillboardMesh.hpp>
#include <Easy3D/BillboardMeshRenderer.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>

#include <memory>
#include <string>
#include <vector>

namespace GalaxyEggbert::CNA
{
    // Real mobile-eggbert in-game HUD for GalaxyEggbertCNA (2026-07-10),
    // replicating Decor::DrawInfo()'s actual bottom-of-screen layout in the
    // same 640x480 reference space (verified directly against
    // mobile-eggbert Decor.cpp:1185-1249): one blupi.png icon 48 per life
    // at (210,417) advancing X+=16, held keys as element.png icons
    // 215/222/229 at (520/530/540, 418), and the treasure counter -- a
    // pad.png icon-15 panel stretched over (410,445)-(510,480) at 0.6
    // opacity with centered "N/M" text at (460,450). Text glyphs come from
    // Content/icons/text.png, where the glyph's sheet index IS the
    // character's ASCII code for the printable range -- read directly off
    // the asset's own layout (row 2 of the 16-column/32px grid starts with
    // ' ' at index 32, row 3 with '0' at 48), NOT transcribed from
    // mobile-eggbert's table_char (per CLAUDE.md's no-table-copying rule;
    // the per-character advance widths in table_char_width are NOT
    // reproduced -- a fixed digit advance is used instead, a documented
    // approximation). The interim bottom-right animation-state indicator
    // (not part of the real mobile-eggbert HUD -- a debug aid until a real
    // Blupi model exists) also renders here.
    //
    // Deliberately drawn as REAL 3D QUADS (BasicEffect + an orthographic
    // pixel-space projection, via the same Easy3D::BillboardMeshRenderer
    // upload/draw path the game's billboards already use) and NOT via CNA's
    // SpriteBatch: CNA's Vulkan backend records ALL SpriteBatch batches
    // BEFORE all 3D draws inside each frame's render pass, with the sprite
    // pipeline's depth test/write disabled
    // (VulkanGraphicsBackend::RecordCommandBuffer -- `drawSpritesFor(...)`
    // runs before `draw3DFor(...)`), so any SpriteBatch HUD is painted over
    // by the 3D scene no matter what the game does -- the exact
    // "HUD/animation icon visible for a second, then gone" bug reported
    // live 2026-07-10 (EasyGL draws in submission order and was fine).
    // Genuine 3D draws ARE recorded in submission order on both backends,
    // so quads submitted last in Draw() reliably render on top. This also
    // repeats this codebase's own earlier lesson: the background quad
    // switched from SpriteBatch to a real 3D quad for a closely-related
    // composability reason (see GalaxyEggbertCnaGame.hpp's
    // backgroundTexture_ comment).
    //
    // The 640x480 reference layout is mapped to the real viewport the same
    // way mobile-eggbert's own origin handling works: uniform scale by
    // viewportHeight/480, horizontally centered (originX-style offset), so
    // HUD proportions are identical at any window size.
    class GEHud
    {
    public:
        // Loads this class's own texture instances (blupi.png/element.png
        // again, plus text.png/pad.png which nothing else loads) -- kept
        // separate from GalaxyEggbertCnaGame's own instances of the same
        // files, matching the established one-instance-per-draw-path
        // pattern (see blupiObjectTexture_ vs blupiIconTexture_ there).
        void LoadContent(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);

        // Draws the full HUD. Call as the LAST draw of the frame (see the
        // class comment for why order matters), with depth testing already
        // disabled by the caller. Sets AlphaBlend for its own draws and
        // restores Opaque before returning.
        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                  int viewportW, int viewportH,
                  int lives, bool key1, bool key2, bool key3,
                  int treasures, int totalTreasures,
                  int animIcon);

    private:
        struct Quad
        {
            float x0, y0, x1, y1; // pixel-space corners (reference 640x480 layout, pre-scaled by caller)
            float u0, v0, u1, v1;
        };

        // Accumulates quads per texture, then uploads/draws each batch via
        // BillboardMeshRenderer (positions are pixel coordinates; the
        // orthographic projection does the rest).
        void FlushQuads(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                        Microsoft::Xna::Framework::Graphics::BasicEffect& effect,
                        std::unique_ptr<Easy3D::BillboardMeshRenderer>& renderer,
                        const std::vector<Quad>& quads, int viewportW, int viewportH,
                        float alpha);

        Microsoft::Xna::Framework::Graphics::Texture2D blupiTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D elementTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D textTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D padTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> blupiEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> elementEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> textEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> padEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> blupiRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> elementRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> textRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> padRenderer_;
        bool loaded_ = false;
    };
}
