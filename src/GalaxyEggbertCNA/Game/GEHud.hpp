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
    // 215/222/229 at (520/530/540, 418), the treasure counter -- a
    // pad.png icon-15 panel stretched over (410,445)-(510,480) at 0.6
    // opacity with centered "N/M" text at (460,450) -- and (2026-07-13,
    // plan.md HUD-015/016) element.png icon 176 x bullet count at (570,442)
    // advancing X+=4, and element.png icon 252 at (505,414) shown only
    // while carrying dynamite. Text glyphs come from
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
        //
        // bullets/dynamite added 2026-07-13 (plan.md HUD-015/016), verified
        // directly against the same real `Decor::DrawInfo` (Decor.cpp
        // 1197-1201, 1212-1217): element.png icon 176 at (570,442),
        // X+=4 per bullet held (a heavily-overlapping "fanned" row, same
        // real look as the life icons); element.png icon 252 at (505,414),
        // a single icon shown only while `dynamite > 0` (matches this
        // engine's own dynamite cap of 1, but checked as `> 0` to mirror
        // the real source's own gate exactly rather than assume the cap).
        //
        // waterGauge*/powerGauge* added 2026-07-13 (plan.md HUD-008/019):
        // the real `Jauge` HUD widget (`Decor.cpp`'s `m_jauges[0]`/`[1]`),
        // verified directly against `Jauge.hpp`'s own fully-documented
        // class comment plus every real `Decor.cpp` call site. `m_jauges[0]`
        // (position (90,450)) is the water/Nage breath gauge -- Blue while
        // `level > 25`, switching to Red at `<= 25` (a real low-air warning
        // color change, `Decor.cpp:4621-4623`); `m_jauges[1]` (position
        // (90,428), always Yellow) is shared by Shield/Power/Cloud/Hide's
        // real countdown timer (all 4 reuse the SAME `m_blupiTimeShield`
        // variable and gauge widget in the real source, confirmed via
        // `Decor.cpp:5071-5137` -- not 4 separate gauges). Real Mirror/
        // Invert, Balloon, and Ecrase also reuse this same gauge -- NOT
        // wired in here since those aren't modeled by this engine's
        // `SecretPower` enum (Balloon/Ecrase have their own separate,
        // un-gauged timers already; Mirror/Invert isn't modeled at all).
        // perso added 2026-07-13 (plan.md HUD-017), verified directly
        // against `Decor.cpp:1202-1211`: button.png icon 108 (40px tiles,
        // `Pixmap.cpp:592-600`) at (0,438), plus "= N" text at (32,452) at
        // real scale 0.7 (smaller than the treasure counter's scale-1.0
        // text), shown only while `perso > 0`.
        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                  int viewportW, int viewportH,
                  int lives, bool key1, bool key2, bool key3,
                  int treasures, int totalTreasures,
                  int bullets, int dynamite, int perso,
                  bool waterGaugeVisible, int waterGaugeLevel,
                  bool powerGaugeVisible, int powerGaugeLevel,
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
        Microsoft::Xna::Framework::Graphics::Texture2D jaugeTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D buttonTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> blupiEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> elementEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> textEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> padEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> jaugeEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> buttonEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> blupiRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> elementRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> textRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> padRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> jaugeRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> buttonRenderer_;
        bool loaded_ = false;
    };
}
