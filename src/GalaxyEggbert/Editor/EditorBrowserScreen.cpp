#include "EditorBrowserScreen.hpp"

#include "CustomWorldStorage.hpp"
#include "PlatformFiles.hpp"

#include <GalaxyEggbert/Game/TextMetrics.hpp>

#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>

#include <algorithm>

namespace GalaxyEggbert::Editor
{
    namespace
    {
        // Laid out like the main menu (OR-0012), over the title scene whose tile letters fill
        // the top band: the ten slots as a grid of five by two below them on one of the menu's
        // translucent panels, each pad.png's blank button with the slot's number and the
        // world's name under it (its trash button on the corner); the older worlds in the
        // free corner right of the letters; Back is the Setup screen's return arrow.
        constexpr float kSlotButtonSize = 64.0f;
        constexpr float kCellWidth = 108.0f;
        constexpr float kCellHeight = 86.0f; // the button and its caption
        constexpr float kCellX0 = 18.0f;
        constexpr float kCellStepX = 114.0f;
        constexpr float kCellY0 = 226.0f;
        constexpr float kCellStepY = 124.0f;
        constexpr int kCellsPerRow = 5;
        constexpr float kDeleteButtonSize = 30.0f;
        constexpr float kListPanelX1 = 590.0f;
        constexpr float kListPanelY0 = 180.0f;
        constexpr float kOlderX0 = 604.0f;
        constexpr float kOlderWidth = 182.0f;
        constexpr float kOlderY0 = 62.0f;
        constexpr float kOlderRowHeight = 28.0f;
        constexpr float kOlderRowStep = 32.0f;
        constexpr int kOlderRowsShown = 9;

        // pad.png (InputPad's sheet: 140 px cells, 8 columns) and the icons used here.
        constexpr float kPadCellPx = 140.0f;
        constexpr int kPadCols = 8;
        constexpr int kIconReturn = 8;    // Setup's back button
        constexpr int kIconConfirm = 13;  // the tick: a second click deletes
        constexpr int kIconPanel = 15;    // the menu's translucent panel
        constexpr int kIconTrash = 20;
        constexpr int kIconBlank = 21;    // a button without a picture
        constexpr float kPanelOpacity = 0.3f;   // InputPad's kInitPanelOpacity
        constexpr float kPressedOpacity = 0.6f; // a button held down
        constexpr float kFaintOpacity = 0.5f;   // an empty slot's number and caption

        // Same text.png glyph convention InputPad.cpp's own label
        // methods already document (32px cells, 16 columns, glyph index ==
        // ASCII code) -- duplicated here deliberately rather than shared:
        // InputPad's own label methods stay private/un-genericized (see
        // plan.md EDITOR-106's own note), this is a small, self-contained
        // use of the same well-known asset convention, not an attempt to
        // build a shared text system.
        constexpr float kGlyphCellPx = 32.0f;
        constexpr int kGlyphCols = 16;
        constexpr float kTitleScale = 0.6f;
        constexpr float kCaptionScale = 0.5f;
        constexpr float kNumberScale = 0.9f;
        constexpr float kSmallScale = 0.4f;
        constexpr float kOlderScale = 0.45f;

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

        void PadIconUv(GalaxyEggbert::Game::QuadBatch::Quad& q, int icon, float sheetW, float sheetH)
        {
            const int col = icon % kPadCols;
            const int row = icon / kPadCols;
            q.u0 = (static_cast<float>(col) * kPadCellPx) / sheetW;
            q.v0 = (static_cast<float>(row) * kPadCellPx) / sheetH;
            q.u1 = (static_cast<float>(col + 1) * kPadCellPx) / sheetW;
            q.v1 = (static_cast<float>(row + 1) * kPadCellPx) / sheetH;
        }
    }

    void EditorBrowserScreen::Refresh(int gamerSlot)
    {
        gamerSlot_ = gamerSlot;
        slotUsed_.assign(kCustomWorldSlotCount, false);
        backupCount_.assign(kCustomWorldSlotCount, 0);
        std::error_code ec;
        for (int slot = 1; slot <= kCustomWorldSlotCount; ++slot)
        {
            slotUsed_[static_cast<std::size_t>(slot - 1)] = std::filesystem::exists(SlotWorldPath(gamerSlot, slot), ec);
            backupCount_[static_cast<std::size_t>(slot - 1)] = BackupCount(SlotWorldPath(gamerSlot, slot));
        }
        older_ = ListSharedWorlds();
        for (auto& path : ListOlderCustomWorlds(gamerSlot))
        {
            older_.push_back(std::move(path));
        }
        armedDeleteIndex_ = -1;
        thumbnailsStale_ = true;
    }

    void EditorBrowserScreen::LoadThumbnails(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        thumbnailsStale_ = false;
        thumbnails_.clear();
        thumbnails_.resize(kCustomWorldSlotCount);
        std::error_code ec;
        for (int i = 0; i < kCustomWorldSlotCount; ++i)
        {
            const auto path = ThumbnailPath(SlotWorldPath(gamerSlot_, i + 1));
            if (!slotUsed_.empty() && slotUsed_[static_cast<std::size_t>(i)] && std::filesystem::exists(path, ec))
            {
                try
                {
                    thumbnails_[static_cast<std::size_t>(i)] =
                        std::make_unique<Microsoft::Xna::Framework::Graphics::Texture2D>(
                            std::filesystem::absolute(path, ec).string(), device);
                }
                catch (const std::exception&)
                {
                    // An unreadable thumbnail: the slot shows its number.
                }
            }
        }
    }

    GalaxyEggbert::Game::QuadBatch::Rect EditorBrowserScreen::SlotRect(int slotIndex) noexcept
    {
        const float x0 = kCellX0 + static_cast<float>(slotIndex % kCellsPerRow) * kCellStepX;
        const float y0 = kCellY0 + static_cast<float>(slotIndex / kCellsPerRow) * kCellStepY;
        return {x0, y0, x0 + kCellWidth, y0 + kCellHeight};
    }

    GalaxyEggbert::Game::QuadBatch::Rect EditorBrowserScreen::DeleteButtonRect(int slotIndex) noexcept
    {
        // On the number button's upper right corner.
        const GalaxyEggbert::Game::QuadBatch::Rect cell = SlotRect(slotIndex);
        const float buttonX1 = cell.x0 + (kCellWidth + kSlotButtonSize) * 0.5f;
        return {buttonX1 - 10.0f, cell.y0 - 8.0f, buttonX1 - 10.0f + kDeleteButtonSize,
                cell.y0 - 8.0f + kDeleteButtonSize};
    }

    GalaxyEggbert::Game::QuadBatch::Rect EditorBrowserScreen::OlderRect(int index) noexcept
    {
        const float y0 = kOlderY0 + static_cast<float>(index) * kOlderRowStep;
        return {kOlderX0, y0, kOlderX0 + kOlderWidth, y0 + kOlderRowHeight};
    }

    GalaxyEggbert::Game::QuadBatch::Rect EditorBrowserScreen::BackButtonRect() noexcept
    {
        return {680.0f, 360.0f, 780.0f, 460.0f};
    }

    GalaxyEggbert::Game::QuadBatch::Rect EditorBrowserScreen::RestoreModeRect() noexcept
    {
        // Right of the title, on the slots' panel.
        return {470.0f, 184.0f, 580.0f, 216.0f};
    }

    GalaxyEggbert::Game::QuadBatch::Rect EditorBrowserScreen::ExportModeRect() noexcept
    {
        return {355.0f, 184.0f, 460.0f, 216.0f};
    }

    GalaxyEggbert::Game::QuadBatch::Rect EditorBrowserScreen::ImportRect() noexcept
    {
        return {250.0f, 184.0f, 345.0f, 216.0f};
    }

    void EditorBrowserScreen::Export(int slotIndex)
    {
        const std::string name = "World " + std::to_string(slotIndex + 1);
        const std::filesystem::path exported = ExportWorld(gamerSlot_, slotIndex + 1);
        if (exported.empty())
        {
            message_ = slotUsed_[static_cast<std::size_t>(slotIndex)] ? name + " could not be exported."
                                                                       : "Slot " + std::to_string(slotIndex + 1) +
                                                                             " has no world.";
            return;
        }
        const std::string file = ExportName(gamerSlot_, slotIndex + 1);
        message_ = OfferDownload(exported, file) ? name + " exported: downloaded as " + file + "."
                                                 : name + " exported to " + exported.generic_string() + ".";
        const std::string kept = message_;
        Refresh(gamerSlot_);
        message_ = kept;
    }

    void EditorBrowserScreen::Restore(int slotIndex)
    {
        const auto path = SlotWorldPath(gamerSlot_, slotIndex + 1);
        const std::string name = "World " + std::to_string(slotIndex + 1);
        std::error_code ec;
        if (restoreSlot_ != slotIndex)
        {
            restoreSlot_ = slotIndex;
            restoreSteps_ = 0;
            restoreCycle_ = std::filesystem::exists(path, ec) ? BackupCount(path) + 1 : 0;
        }
        if (!RestoreBackup(path))
        {
            message_ = name + " has no backup.";
            return;
        }
        ++restoreSteps_;
        message_ = restoreCycle_ > 0 && restoreSteps_ % restoreCycle_ == 0
                       ? name + " is back as it was."
                       : name + ": backup restored. Again: an older one.";
        const int steps = restoreSteps_;
        Refresh(gamerSlot_);
        restoreSlot_ = slotIndex;
        restoreSteps_ = steps;
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
        mouseX_ = mx;
        mouseY_ = my;

        UpdateResult result;
        if (ConsumeUploadDone())
        {
            Refresh(gamerSlot_);
            message_ = "Imported: on the right; a click copies it into a slot.";
        }
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
            else if (InRect(mx, my, RestoreModeRect()) || InRect(mx, my, ExportModeRect()))
            {
                const Mode clicked = InRect(mx, my, RestoreModeRect()) ? Mode::Restore : Mode::Export;
                mode_ = mode_ == clicked ? Mode::Open : clicked;
                message_ = mode_ == Mode::Restore  ? "Click a world to bring back its backup."
                           : mode_ == Mode::Export ? "Click a world to export it."
                                                   : "";
                restoreSlot_ = -1;
                armedDeleteIndex_ = -1;
                mouseWasDown_ = mouseDown;
                return result;
            }
            else if (InRect(mx, my, ImportRect()))
            {
                Refresh(gamerSlot_);
                message_ = RequestUpload(SharedWorldsDir())
                               ? "Choose a .vwr file: it is listed on the right."
                               : "Put .vwr files in " + SharedWorldsDir().generic_string() +
                                     "/: they are listed on the right.";
                mouseWasDown_ = mouseDown;
                return result;
            }
            for (int i = 0; i < kCustomWorldSlotCount && result.action == Action::None; ++i)
            {
                const auto path = SlotWorldPath(gamerSlot_, i + 1);
                if (slotUsed_[static_cast<std::size_t>(i)] && InRect(mx, my, DeleteButtonRect(i)))
                {
                    if (armedDeleteIndex_ == i)
                    {
                        // Into its backups: Backups brings it back.
                        (void)DeleteSlotWorld(path);
                        Refresh(gamerSlot_);
                        message_ = "World " + std::to_string(i + 1) + " deleted (Backups brings it back).";
                    }
                    else
                    {
                        armed = i;
                    }
                    break;
                }
                if (InRect(mx, my, SlotRect(i)))
                {
                    if (mode_ == Mode::Restore)
                    {
                        Restore(i);
                        break;
                    }
                    if (mode_ == Mode::Export)
                    {
                        Export(i);
                        break;
                    }
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

        if (!std::filesystem::exists("Content/icons/pad.png"))
        {
            return;
        }
        padTexture_ = Texture2D("Content/icons/pad.png", device);
        padEffect_ = std::make_unique<BasicEffect>(device);
        padEffect_->VertexColorEnabled = false;
        padEffect_->setTextureEnabledProperty(true);
        padEffect_->setTextureProperty(&padTexture_);
        thumbnailEffect_ = std::make_unique<BasicEffect>(device);
        thumbnailEffect_->VertexColorEnabled = false;
        thumbnailEffect_->setTextureEnabledProperty(true);

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
        if (thumbnailsStale_)
        {
            LoadThumbnails(device);
        }

        using Microsoft::Xna::Framework::Graphics::BlendState;
        using GalaxyEggbert::Game::QuadBatch::InRect;
        using GalaxyEggbert::Game::QuadBatch::Quad;
        using GalaxyEggbert::Game::QuadBatch::Rect;
        using GalaxyEggbert::Game::TextMetrics::Width;
        device.setBlendStateProperty(BlendState::NonPremultiplied);

        const float padSheetW = static_cast<float>(padTexture_.getWidthProperty());
        const float padSheetH = static_cast<float>(padTexture_.getHeightProperty());
        std::vector<Quad> panels;
        std::vector<Quad> buttons;
        std::vector<Quad> pressedButtons;
        const auto addIcon = [&](std::vector<Quad>& bucket, const Rect& r, int icon)
        {
            Quad q{r.x0, r.y0, r.x1, r.y1, 0.0f, 0.0f, 0.0f, 0.0f};
            PadIconUv(q, icon, padSheetW, padSheetH);
            bucket.push_back(q);
        };
        // A button held down is drawn faded, as the menu's are.
        const auto addButton = [&](const Rect& r, int icon)
        {
            addIcon(mouseWasDown_ && InRect(mouseX_, mouseY_, r) ? pressedButtons : buttons, r, icon);
        };

        const int olderShown = std::min(static_cast<int>(older_.size()), kOlderRowsShown);
        addIcon(panels, {10.0f, kListPanelY0, kListPanelX1, 470.0f}, kIconPanel);
        if (!older_.empty())
        {
            addIcon(panels, {kOlderX0 - 6.0f, 10.0f, kOlderX0 + kOlderWidth + 6.0f,
                             kOlderY0 + kOlderRowsShown * kOlderRowStep + 4.0f},
                    kIconPanel);
            for (int i = 0; i < olderShown; ++i)
            {
                addIcon(panels, OlderRect(i), kIconPanel);
            }
        }
        addButton(BackButtonRect(), kIconReturn);
        addIcon(mode_ == Mode::Restore ? buttons : panels, RestoreModeRect(), mode_ == Mode::Restore ? kIconBlank : kIconPanel);
        addIcon(mode_ == Mode::Export ? buttons : panels, ExportModeRect(), mode_ == Mode::Export ? kIconBlank : kIconPanel);
        addIcon(mouseWasDown_ && InRect(mouseX_, mouseY_, ImportRect()) ? buttons : panels, ImportRect(),
                mouseWasDown_ && InRect(mouseX_, mouseY_, ImportRect()) ? kIconBlank : kIconPanel);
        for (int i = 0; i < kCustomWorldSlotCount; ++i)
        {
            const Rect cell = SlotRect(i);
            const float buttonX0 = cell.x0 + (kCellWidth - kSlotButtonSize) * 0.5f;
            const Rect number{buttonX0, cell.y0, buttonX0 + kSlotButtonSize, cell.y0 + kSlotButtonSize};
            const bool overDelete = slotUsed_[static_cast<std::size_t>(i)] && InRect(mouseX_, mouseY_, DeleteButtonRect(i));
            addIcon(mouseWasDown_ && InRect(mouseX_, mouseY_, cell) && !overDelete ? pressedButtons : buttons, number,
                    kIconBlank);
            if (slotUsed_[static_cast<std::size_t>(i)])
            {
                addButton(DeleteButtonRect(i), armedDeleteIndex_ == i ? kIconConfirm : kIconTrash);
            }
        }
        GalaxyEggbert::Game::QuadBatch::FlushQuads(device, *padEffect_, padRenderer_, panels, viewportWidth,
                                                   viewportHeight, kPanelOpacity);
        GalaxyEggbert::Game::QuadBatch::FlushQuads(device, *padEffect_, padRenderer_, buttons, viewportWidth,
                                                   viewportHeight, 1.0f);
        GalaxyEggbert::Game::QuadBatch::FlushQuads(device, *padEffect_, padRenderer_, pressedButtons, viewportWidth,
                                                   viewportHeight, kPressedOpacity);
        const auto thumbnailAt = [&](int i) -> Microsoft::Xna::Framework::Graphics::Texture2D* {
            return static_cast<std::size_t>(i) < thumbnails_.size() ? thumbnails_[static_cast<std::size_t>(i)].get()
                                                                    : nullptr;
        };
        for (int i = 0; i < kCustomWorldSlotCount; ++i)
        {
            auto* thumbnail = thumbnailAt(i);
            if (thumbnail == nullptr)
            {
                continue;
            }
            const Rect cell = SlotRect(i);
            const float x0 = cell.x0 + (kCellWidth - kSlotButtonSize) * 0.5f + 5.0f;
            const float y0 = cell.y0 + 5.0f;
            const std::vector<Quad> image{
                Quad{x0, y0, x0 + kSlotButtonSize - 10.0f, y0 + kSlotButtonSize - 10.0f, 0.0f, 0.0f, 1.0f, 1.0f}};
            thumbnailEffect_->setTextureProperty(thumbnail);
            GalaxyEggbert::Game::QuadBatch::FlushQuads(device, *thumbnailEffect_, thumbnailRenderer_, image,
                                                       viewportWidth, viewportHeight,
                                                       mouseWasDown_ && InRect(mouseX_, mouseY_, cell) ? kPressedOpacity
                                                                                                      : 1.0f);
        }

        const float textSheetW = static_cast<float>(textTexture_.getWidthProperty());
        const float textSheetH = static_cast<float>(textTexture_.getHeightProperty());
        std::vector<Quad> labels;
        std::vector<Quad> faintLabels;
        const auto label = [&](std::vector<Quad>& bucket, const std::string& text, float x, float y, float scale)
        {
            AppendLabel(bucket, text, x, y, scale, textSheetW, textSheetH);
        };
        // Text vertically centred on a band [y0, y1].
        const auto middle = [](float y0, float y1, float scale) { return (y0 + y1 - kGlyphCellPx * scale) * 0.5f; };

        label(labels, std::string("3D World Editor - Player ") + static_cast<char>('A' + gamerSlot_), 22.0f,
              kListPanelY0 + 8.0f, kTitleScale);
        for (int i = 0; i < kCustomWorldSlotCount; ++i)
        {
            const Rect cell = SlotRect(i);
            const float centreX = (cell.x0 + cell.x1) * 0.5f;
            const bool used = slotUsed_[static_cast<std::size_t>(i)];
            auto& bucket = used ? labels : faintLabels;
            const std::string number = std::to_string(i + 1);
            if (thumbnailAt(i) == nullptr) // a thumbnail stands for the number (the caption names it)
            {
                label(bucket, number, centreX - Width(number) * kNumberScale * 0.5f,
                      middle(cell.y0, cell.y0 + kSlotButtonSize, kNumberScale), kNumberScale);
            }
            std::string caption = !used                   ? "New world"
                                  : armedDeleteIndex_ == i ? "Delete?"
                                                           : "World " + number;
            auto* captionBucket = &bucket;
            if (mode_ == Mode::Restore)
            {
                const bool has = backupCount_[static_cast<std::size_t>(i)] > 0;
                caption = has ? "Restore" : "No backup";
                captionBucket = has ? &labels : &faintLabels;
            }
            else if (mode_ == Mode::Export && used)
            {
                caption = "Export";
            }
            label(*captionBucket, caption, centreX - Width(caption) * kCaptionScale * 0.5f,
                  cell.y0 + kSlotButtonSize + 4.0f, kCaptionScale);
        }
        for (const auto& [rect, text] : {std::pair<Rect, std::string>{RestoreModeRect(), "Backups"},
                                         std::pair<Rect, std::string>{ExportModeRect(), "Export"},
                                         std::pair<Rect, std::string>{ImportRect(), "Import"}})
        {
            label(labels, text, (rect.x0 + rect.x1 - Width(text) * kCaptionScale) * 0.5f,
                  middle(rect.y0, rect.y1, kCaptionScale), kCaptionScale);
        }
        if (!message_.empty())
        {
            label(labels, message_, 22.0f, 444.0f, kSmallScale);
        }
        if (!older_.empty())
        {
            label(labels, "Other worlds", kOlderX0, 14.0f, kCaptionScale);
            label(labels, slotsFull_ ? "(no free slot)" : "(click: copy to a slot)", kOlderX0, 36.0f, kSmallScale);
        }
        for (int i = 0; i < olderShown; ++i)
        {
            const Rect row = OlderRect(i);
            std::string name = older_[static_cast<std::size_t>(i)].stem().string();
            while (!name.empty() && Width(name) * kOlderScale > kOlderWidth - 16.0f)
            {
                name.pop_back();
            }
            label(labels, name, row.x0 + 8.0f, middle(row.y0, row.y1, kOlderScale), kOlderScale);
        }
        GalaxyEggbert::Game::QuadBatch::FlushQuads(device, *textEffect_, textRenderer_, labels, viewportWidth,
                                                   viewportHeight, 1.0f);
        GalaxyEggbert::Game::QuadBatch::FlushQuads(device, *textEffect_, textRenderer_, faintLabels, viewportWidth,
                                                   viewportHeight, kFaintOpacity);

        device.setBlendStateProperty(BlendState::Opaque);
    }
}
