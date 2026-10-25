#include "EditorBrowserScreen.hpp"

#include "CustomWorldStorage.hpp"

#include <GalaxyEggbert/Game/TextMetrics.hpp>

#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>

#include <algorithm>

namespace GalaxyEggbert::Editor
{
    namespace
    {
        constexpr float kRowHeight = 34.0f;
        constexpr float kRowWidth = 340.0f;
        constexpr float kRowX0 = 200.0f;
        constexpr float kRowY0 = 70.0f;
        constexpr float kRowGap = 4.0f;
        constexpr float kDeleteButtonSize = 26.0f;
        constexpr float kOlderX0 = 570.0f;
        constexpr float kOlderWidth = 210.0f;
        constexpr float kOlderY0 = 108.0f;
        constexpr int kOlderRowsShown = 9;

        // Same text.png glyph convention InputPad.cpp's own label
        // methods already document (32px cells, 16 columns, glyph index ==
        // ASCII code) -- duplicated here deliberately rather than shared:
        // InputPad's own label methods stay private/un-genericized (see
        // plan.md EDITOR-106's own note), this is a small, self-contained
        // use of the same well-known asset convention, not an attempt to
        // build a shared text system.
        constexpr float kGlyphCellPx = 32.0f;
        constexpr int kGlyphCols = 16;
        constexpr float kLabelScale = 0.5f;

        void AppendLabel(std::vector<GalaxyEggbert::Game::QuadBatch::Quad>& quads, const std::string& text,
                         float leftX, float topY, float scale, float textSheetW, float textSheetH)
        {
            const float cellPx = kGlyphCellPx * scale;
            float penX = leftX;
            for (const char c : text)
            {
                const int rank = GalaxyEggbert::Game::TextMetrics::GlyphIndex(c);
                const int col = rank % kGlyphCols;
                const int row = rank / kGlyphCols;
                GalaxyEggbert::Game::QuadBatch::Quad q;
                q.x0 = penX;
                q.y0 = topY;
                q.x1 = penX + cellPx;
                q.y1 = topY + cellPx;
                q.u0 = (static_cast<float>(col) * kGlyphCellPx) / textSheetW;
                q.v0 = (static_cast<float>(row) * kGlyphCellPx) / textSheetH;
                q.u1 = (static_cast<float>(col + 1) * kGlyphCellPx) / textSheetW;
                q.v1 = (static_cast<float>(row + 1) * kGlyphCellPx) / textSheetH;
                quads.push_back(q);
                penX += GalaxyEggbert::Game::TextMetrics::Advance(c) * scale;
            }
        }
    }

    void EditorBrowserScreen::Refresh(int gamerSlot)
    {
        gamerSlot_ = gamerSlot;
        slotUsed_.assign(kCustomWorldSlotCount, false);
        std::error_code ec;
        for (int slot = 1; slot <= kCustomWorldSlotCount; ++slot)
        {
            slotUsed_[static_cast<std::size_t>(slot - 1)] = std::filesystem::exists(SlotWorldPath(gamerSlot, slot), ec);
        }
        older_ = ListOlderCustomWorlds(gamerSlot);
        armedDeleteIndex_ = -1;
    }

    GalaxyEggbert::Game::QuadBatch::Rect EditorBrowserScreen::SlotRect(int slotIndex) noexcept
    {
        const float y0 = kRowY0 + static_cast<float>(slotIndex) * (kRowHeight + kRowGap);
        return {kRowX0, y0, kRowX0 + kRowWidth, y0 + kRowHeight};
    }

    GalaxyEggbert::Game::QuadBatch::Rect EditorBrowserScreen::DeleteButtonRect(int slotIndex) noexcept
    {
        const GalaxyEggbert::Game::QuadBatch::Rect row = SlotRect(slotIndex);
        const float y0 = row.y0 + (kRowHeight - kDeleteButtonSize) * 0.5f;
        return {row.x1 - kDeleteButtonSize - 4.0f, y0, row.x1 - 4.0f, y0 + kDeleteButtonSize};
    }

    GalaxyEggbert::Game::QuadBatch::Rect EditorBrowserScreen::OlderRect(int index) noexcept
    {
        const float y0 = kOlderY0 + static_cast<float>(index) * (kRowHeight + kRowGap);
        return {kOlderX0, y0, kOlderX0 + kOlderWidth, y0 + kRowHeight};
    }

    GalaxyEggbert::Game::QuadBatch::Rect EditorBrowserScreen::BackButtonRect() noexcept
    {
        return {20.0f, 20.0f, 170.0f, 20.0f + kRowHeight};
    }

    EditorBrowserScreen::UpdateResult EditorBrowserScreen::Update(
        const Microsoft::Xna::Framework::Input::MouseState& mouse,
        int /*viewportWidth*/, int /*viewportHeight*/, bool backPressed)
    {
        using ButtonState = Microsoft::Xna::Framework::Input::ButtonState;
        using GalaxyEggbert::Game::QuadBatch::InRect;
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;
        const float mx = static_cast<float>(mouse.getXProperty());
        const float my = static_cast<float>(mouse.getYProperty());

        UpdateResult result;
        const bool backTriggered = backPressed && !backWasDown_;
        backWasDown_ = backPressed;
        if (backTriggered)
        {
            result.action = Action::Back;
            armedDeleteIndex_ = -1;
            mouseWasDown_ = mouseDown;
            return result;
        }
        if (!mouseDown && mouseWasDown_)
        {
            int armed = -1;
            if (InRect(mx, my, BackButtonRect()))
            {
                result.action = Action::Back;
            }
            for (int i = 0; i < kCustomWorldSlotCount && result.action == Action::None; ++i)
            {
                const auto path = SlotWorldPath(gamerSlot_, i + 1);
                if (slotUsed_[static_cast<std::size_t>(i)] && InRect(mx, my, DeleteButtonRect(i)))
                {
                    if (armedDeleteIndex_ == i)
                    {
                        std::error_code ec;
                        std::filesystem::remove(path, ec);
                        Refresh(gamerSlot_);
                    }
                    else
                    {
                        armed = i;
                    }
                    break;
                }
                if (InRect(mx, my, SlotRect(i)))
                {
                    result.action = slotUsed_[static_cast<std::size_t>(i)] ? Action::Open : Action::New;
                    result.path = path;
                }
            }
            for (int i = 0; i < static_cast<int>(older_.size()) && i < kOlderRowsShown &&
                            result.action == Action::None; ++i)
            {
                if (InRect(mx, my, OlderRect(i)))
                {
                    const auto copy = CopyIntoFreeSlot(gamerSlot_, older_[static_cast<std::size_t>(i)]);
                    slotsFull_ = copy.empty();
                    if (!copy.empty())
                    {
                        result.action = Action::Open;
                        result.path = copy;
                    }
                }
            }
            armedDeleteIndex_ = armed;
        }
        mouseWasDown_ = mouseDown;
        return result;
    }

    void EditorBrowserScreen::EnsureLoaded(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        if (loaded_)
        {
            return;
        }
        using Microsoft::Xna::Framework::Graphics::BasicEffect;
        using Microsoft::Xna::Framework::Graphics::Texture2D;

        if (!std::filesystem::exists("Content/icons/text.png"))
        {
            return; // degrades to no text, same graceful-failure shape as InputPad::LoadContent()
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

    void EditorBrowserScreen::Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                     int viewportWidth, int viewportHeight)
    {
        EnsureLoaded(device);
        if (!loaded_)
        {
            return;
        }

        using Microsoft::Xna::Framework::Graphics::BlendState;
        device.setBlendStateProperty(BlendState::NonPremultiplied);

        std::vector<GalaxyEggbert::Game::QuadBatch::Quad> flatQuads;
        const auto addFlat = [&flatQuads](const GalaxyEggbert::Game::QuadBatch::Rect& r)
        {
            flatQuads.push_back({r.x0, r.y0, r.x1, r.y1, 0.0f, 0.0f, 1.0f, 1.0f});
        };
        addFlat(BackButtonRect());
        for (int i = 0; i < kCustomWorldSlotCount; ++i)
        {
            addFlat(SlotRect(i));
            if (slotUsed_[static_cast<std::size_t>(i)])
            {
                addFlat(DeleteButtonRect(i));
            }
        }
        const int olderShown = std::min(static_cast<int>(older_.size()), kOlderRowsShown);
        for (int i = 0; i < olderShown; ++i)
        {
            addFlat(OlderRect(i));
        }
        GalaxyEggbert::Game::QuadBatch::FlushQuads(device, *flatEffect_, flatRenderer_, flatQuads, viewportWidth, viewportHeight, 0.5f);

        const float textSheetW = static_cast<float>(textTexture_.getWidthProperty());
        const float textSheetH = static_cast<float>(textTexture_.getHeightProperty());
        std::vector<GalaxyEggbert::Game::QuadBatch::Quad> labelQuads;
        const auto label = [&](const std::string& text, float x, float y)
        {
            AppendLabel(labelQuads, text, x, y, kLabelScale, textSheetW, textSheetH);
        };
        label("< Main Menu", BackButtonRect().x0 + 8.0f, BackButtonRect().y0 + 8.0f);
        label(std::string("3D World Editor - Player ") + static_cast<char>('A' + gamerSlot_), kRowX0, 28.0f);
        for (int i = 0; i < kCustomWorldSlotCount; ++i)
        {
            const auto row = SlotRect(i);
            const bool used = slotUsed_[static_cast<std::size_t>(i)];
            label(std::to_string(i + 1) + (used ? "   World " + std::to_string(i + 1) : "   (empty) - new world"),
                  row.x0 + 8.0f, row.y0 + 7.0f);
            if (used)
            {
                const auto del = DeleteButtonRect(i);
                label(armedDeleteIndex_ == i ? "!" : "X", del.x0 + 6.0f, del.y0 + 2.0f);
            }
        }
        if (!older_.empty())
        {
            label("Older worlds", kOlderX0, kOlderY0 - 30.0f);
            label(slotsFull_ ? "(no free slot)" : "(click: copy to a slot)", kOlderX0, kOlderY0 - 16.0f);
        }
        for (int i = 0; i < olderShown; ++i)
        {
            const auto row = OlderRect(i);
            label(older_[static_cast<std::size_t>(i)].stem().string(), row.x0 + 8.0f, row.y0 + 7.0f);
        }
        GalaxyEggbert::Game::QuadBatch::FlushQuads(device, *textEffect_, textRenderer_, labelQuads, viewportWidth, viewportHeight, 1.0f);

        device.setBlendStateProperty(BlendState::Opaque);
    }
}
