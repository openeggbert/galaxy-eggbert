#include "GEEditorBrowserScreen.hpp"

#include "GECustomWorldStorage.hpp"

#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>

#include <algorithm>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        constexpr float kRowHeight = 36.0f;
        constexpr float kRowWidth = 320.0f;
        constexpr float kRowX0 = 200.0f;
        constexpr float kRowY0 = 20.0f;
        constexpr float kRowGap = 6.0f;
        constexpr float kDeleteButtonSize = 28.0f;

        // Same text.png glyph convention GEInputPad.cpp's own label
        // methods already document (32px cells, 16 columns, glyph index ==
        // ASCII code) -- duplicated here deliberately rather than shared:
        // GEInputPad's own label methods stay private/un-genericized (see
        // plan.md EDITOR-106's own note), this is a small, self-contained
        // use of the same well-known asset convention, not an attempt to
        // build a shared text system.
        constexpr float kGlyphCellPx = 32.0f;
        constexpr int kGlyphCols = 16;
        constexpr float kGlyphAdvance = 17.0f;
        constexpr float kLabelScale = 0.5f;

        void AppendLabel(std::vector<GEQuadBatch::Quad>& quads, const std::string& text,
                         float leftX, float topY, float scale, float textSheetW, float textSheetH)
        {
            const float cellPx = kGlyphCellPx * scale;
            const float advance = kGlyphAdvance * scale;
            float penX = leftX;
            for (const char c : text)
            {
                const int rank = static_cast<int>(static_cast<unsigned char>(c));
                const int col = rank % kGlyphCols;
                const int row = rank / kGlyphCols;
                GEQuadBatch::Quad q;
                q.x0 = penX;
                q.y0 = topY;
                q.x1 = penX + cellPx;
                q.y1 = topY + cellPx;
                q.u0 = (static_cast<float>(col) * kGlyphCellPx) / textSheetW;
                q.v0 = (static_cast<float>(row) * kGlyphCellPx) / textSheetH;
                q.u1 = (static_cast<float>(col + 1) * kGlyphCellPx) / textSheetW;
                q.v1 = (static_cast<float>(row + 1) * kGlyphCellPx) / textSheetH;
                quads.push_back(q);
                penX += advance;
            }
        }
    }

    void GEEditorBrowserScreen::Refresh(int gamerSlot)
    {
        gamerSlot_ = gamerSlot;
        worlds_ = ListCustomWorlds(gamerSlot);
        // Newest first -- last_write_time-derived recency, since there's
        // no free-text name to sort by (see this class's own header
        // comment on why renaming isn't supported yet).
        std::sort(worlds_.begin(), worlds_.end(), [](const std::filesystem::path& a, const std::filesystem::path& b)
        {
            std::error_code ecA, ecB;
            const auto timeA = std::filesystem::last_write_time(a, ecA);
            const auto timeB = std::filesystem::last_write_time(b, ecB);
            return timeA > timeB;
        });
        armedDeleteIndex_ = -1;
    }

    GEQuadBatch::Rect GEEditorBrowserScreen::RowRect(int index) const noexcept
    {
        const float y0 = kRowY0 + static_cast<float>(index) * (kRowHeight + kRowGap);
        return {kRowX0, y0, kRowX0 + kRowWidth, y0 + kRowHeight};
    }

    GEQuadBatch::Rect GEEditorBrowserScreen::DeleteButtonRect(int index) const noexcept
    {
        const GEQuadBatch::Rect row = RowRect(index);
        const float y0 = row.y0 + (kRowHeight - kDeleteButtonSize) * 0.5f;
        return {row.x1 - kDeleteButtonSize - 4.0f, y0, row.x1 - 4.0f, y0 + kDeleteButtonSize};
    }

    GEEditorBrowserScreen::UpdateResult GEEditorBrowserScreen::Update(
        const Microsoft::Xna::Framework::Input::MouseState& mouse, int /*viewportWidth*/, int /*viewportHeight*/)
    {
        using ButtonState = Microsoft::Xna::Framework::Input::ButtonState;
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;
        const float mx = static_cast<float>(mouse.getXProperty());
        const float my = static_cast<float>(mouse.getYProperty());

        UpdateResult result;
        if (!mouseDown && mouseWasDown_)
        {
            if (GEQuadBatch::InRect(mx, my, RowRect(0)))
            {
                result.action = Action::New;
                armedDeleteIndex_ = -1;
            }
            else
            {
                bool handled = false;
                for (std::size_t i = 0; i < worlds_.size(); ++i)
                {
                    const int rowIndex = static_cast<int>(i) + 1;
                    if (GEQuadBatch::InRect(mx, my, DeleteButtonRect(rowIndex)))
                    {
                        if (armedDeleteIndex_ == static_cast<int>(i))
                        {
                            std::error_code ec;
                            std::filesystem::remove(worlds_[i], ec);
                            Refresh(gamerSlot_);
                        }
                        else
                        {
                            armedDeleteIndex_ = static_cast<int>(i);
                        }
                        handled = true;
                        break;
                    }
                    if (GEQuadBatch::InRect(mx, my, RowRect(rowIndex)))
                    {
                        result.action = Action::Open;
                        result.path = worlds_[i];
                        armedDeleteIndex_ = -1;
                        handled = true;
                        break;
                    }
                }
                if (!handled)
                {
                    armedDeleteIndex_ = -1;
                }
            }
        }
        mouseWasDown_ = mouseDown;
        return result;
    }

    void GEEditorBrowserScreen::EnsureLoaded(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        if (loaded_)
        {
            return;
        }
        using Microsoft::Xna::Framework::Graphics::BasicEffect;
        using Microsoft::Xna::Framework::Graphics::Texture2D;

        if (!std::filesystem::exists("Content/icons/text.png"))
        {
            return; // degrades to no text, same graceful-failure shape as GEInputPad::LoadContent()
        }
        textTexture_ = Texture2D("Content/icons/text.png", device);
        textEffect_ = std::make_unique<BasicEffect>(device);
        textEffect_->VertexColorEnabled = false;
        textEffect_->setTextureEnabledProperty(true);
        textEffect_->setTextureProperty(&textTexture_);

        flatEffect_ = std::make_unique<BasicEffect>(device);
        flatEffect_->VertexColorEnabled = false;
        flatEffect_->setTextureEnabledProperty(false);
        flatEffect_->setDiffuseColorProperty(Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f));

        loaded_ = true;
    }

    void GEEditorBrowserScreen::Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                     int viewportWidth, int viewportHeight)
    {
        EnsureLoaded(device);
        if (!loaded_)
        {
            return;
        }

        using Microsoft::Xna::Framework::Graphics::BlendState;
        device.setBlendStateProperty(BlendState::NonPremultiplied);

        std::vector<GEQuadBatch::Quad> flatQuads;
        const auto addFlat = [&flatQuads](const GEQuadBatch::Rect& r)
        {
            flatQuads.push_back({r.x0, r.y0, r.x1, r.y1, 0.0f, 0.0f, 1.0f, 1.0f});
        };
        addFlat(RowRect(0)); // "+ New World" row
        for (std::size_t i = 0; i < worlds_.size(); ++i)
        {
            addFlat(RowRect(static_cast<int>(i) + 1));
            addFlat(DeleteButtonRect(static_cast<int>(i) + 1));
        }
        GEQuadBatch::FlushQuads(device, *flatEffect_, flatRenderer_, flatQuads, viewportWidth, viewportHeight, 0.5f);

        const float textSheetW = static_cast<float>(textTexture_.getWidthProperty());
        const float textSheetH = static_cast<float>(textTexture_.getHeightProperty());
        std::vector<GEQuadBatch::Quad> labelQuads;
        {
            const GEQuadBatch::Rect row0 = RowRect(0);
            AppendLabel(labelQuads, "+ New World", row0.x0 + 8.0f, row0.y0 + 8.0f, kLabelScale, textSheetW,
                       textSheetH);
        }
        for (std::size_t i = 0; i < worlds_.size(); ++i)
        {
            const GEQuadBatch::Rect row = RowRect(static_cast<int>(i) + 1);
            AppendLabel(labelQuads, worlds_[i].stem().string(), row.x0 + 8.0f, row.y0 + 8.0f, kLabelScale,
                       textSheetW, textSheetH);
            const GEQuadBatch::Rect del = DeleteButtonRect(static_cast<int>(i) + 1);
            const char* deleteLabel = (armedDeleteIndex_ == static_cast<int>(i)) ? "!" : "X";
            AppendLabel(labelQuads, deleteLabel, del.x0 + 6.0f, del.y0 + 2.0f, kLabelScale, textSheetW, textSheetH);
        }
        GEQuadBatch::FlushQuads(device, *textEffect_, textRenderer_, labelQuads, viewportWidth, viewportHeight, 1.0f);

        device.setBlendStateProperty(BlendState::Opaque);
    }
}
