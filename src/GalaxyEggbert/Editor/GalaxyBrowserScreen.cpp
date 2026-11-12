#include "GalaxyBrowserScreen.hpp"
#include <GalaxyEggbert/Game/TextureScale.hpp>

#include "CustomWorldStorage.hpp"
#include "PlatformFiles.hpp"

#include <GalaxyEggbert/Game/Localization.hpp>
#include <GalaxyEggbert/Game/TextMetrics.hpp>

#include <algorithm>
#include <utility>

#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>

namespace GalaxyEggbert::Editor
{
    namespace
    {
        using GalaxyEggbert::Game::QuadBatch::Quad;
        using GalaxyEggbert::Game::QuadBatch::Rect;

        // pad.png and text.png as EditorBrowserScreen draws them (140 px cells in 8 columns; 32 px
        // glyphs in 16 columns).
        constexpr float kPadCellPx = 140.0f;
        constexpr int kPadCols = 8;
        constexpr int kIconReturn = 8;
        constexpr int kIconConfirm = 13; // the tick: a second click deletes
        constexpr int kIconPanel = 15;
        constexpr int kIconTrash = 20;
        constexpr int kIconBlank = 21;
        constexpr float kGlyphCellPx = 32.0f;
        constexpr int kGlyphCols = 16;
        constexpr float kPanelOpacity = 0.3f;
        constexpr float kPressedOpacity = 0.6f;
        constexpr float kFaintOpacity = 0.5f;

        void PadIconUv(Quad& q, int icon, float sheetW, float sheetH)
        {
            const int col = icon % kPadCols;
            const int row = icon / kPadCols;
            q.u0 = static_cast<float>(col) * kPadCellPx / sheetW;
            q.v0 = static_cast<float>(row) * kPadCellPx / sheetH;
            q.u1 = static_cast<float>(col + 1) * kPadCellPx / sheetW;
            q.v1 = static_cast<float>(row + 1) * kPadCellPx / sheetH;
        }

        void AppendLabel(std::vector<Quad>& quads, std::string_view text, float leftX, float topY, float scale,
                         float sheetW, float sheetH)
        {
            float penX = leftX;
            for (std::size_t at = 0; at < text.size();)
            {
                const int rank = GalaxyEggbert::Game::TextMetrics::NextGlyph(text, at);
                const int col = rank % kGlyphCols;
                const int row = rank / kGlyphCols;
                Quad q{penX, topY, penX + kGlyphCellPx * scale, topY + kGlyphCellPx * scale,
                       static_cast<float>(col) * kGlyphCellPx / sheetW, static_cast<float>(row) * kGlyphCellPx / sheetH,
                       static_cast<float>(col + 1) * kGlyphCellPx / sheetW,
                       static_cast<float>(row + 1) * kGlyphCellPx / sheetH};
                quads.push_back(q);
                penX += GalaxyEggbert::Game::TextMetrics::GlyphAdvance(rank) * scale;
            }
        }
    }

    Rect GalaxyBrowserScreen::MainHubRect() noexcept { return {22.0f, 52.0f, 86.0f, 116.0f}; }
    Rect GalaxyBrowserScreen::BonusRect() noexcept { return {130.0f, 52.0f, 194.0f, 116.0f}; }

    Rect GalaxyBrowserScreen::WorldRect(int world) noexcept
    {
        const int index = world - 1;
        const float x0 = 22.0f + static_cast<float>(index % 7) * 80.0f;
        const float y0 = index < 7 ? 172.0f : 254.0f;
        return {x0, y0, x0 + 60.0f, y0 + 60.0f};
    }

    Rect GalaxyBrowserScreen::PlaceRect(int place) noexcept
    {
        const float x0 = 22.0f + static_cast<float>(place) * 64.0f;
        return {x0, 366.0f, x0 + 56.0f, 422.0f};
    }

    // On the place's top-right corner, inside the gap to the next one (as the world screen's bin).
    Rect GalaxyBrowserScreen::DeleteRect(const Rect& place) noexcept
    {
        return {place.x1 - 20.0f, place.y0 - 6.0f, place.x1 + 4.0f, place.y0 + 18.0f};
    }

    // Left of Back, under the help (where the world screen has its Galaxy button).
    Rect GalaxyBrowserScreen::BackupsRect() noexcept { return {598.0f, 372.0f, 674.0f, 448.0f}; }

    Rect GalaxyBrowserScreen::BackButtonRect() noexcept { return {680.0f, 360.0f, 780.0f, 460.0f}; }

    // Top right of the places' panel, beside the bonus world.
    Rect GalaxyBrowserScreen::ExportRect() noexcept { return {380.0f, 60.0f, 480.0f, 96.0f}; }
    Rect GalaxyBrowserScreen::ImportRect() noexcept { return {488.0f, 60.0f, 588.0f, 96.0f}; }

    Rect GalaxyBrowserScreen::ImportRowRect(int index) noexcept
    {
        const float y0 = 50.0f + static_cast<float>(index) * 32.0f;
        return {604.0f, y0, 786.0f, y0 + 28.0f};
    }

    Rect GalaxyBrowserScreen::ImportDeleteRect(int index) noexcept
    {
        const Rect row = ImportRowRect(index);
        return {row.x1 - 26.0f, row.y0 + 2.0f, row.x1 - 2.0f, row.y1 - 2.0f};
    }

    // The title, top left of the places' panel.
    Rect GalaxyBrowserScreen::NameRect() noexcept { return {16.0f, 10.0f, 366.0f, 44.0f}; }

    // Under Export and Import, on their dark ground.
    Rect GalaxyBrowserScreen::DeleteGalaxyRect() noexcept { return {380.0f, 102.0f, 588.0f, 132.0f}; }

    Rect GalaxyBrowserScreen::KeyboardRect() noexcept { return {22.0f, 150.0f, 582.0f, 436.0f}; }

    void GalaxyBrowserScreen::StartNaming()
    {
        naming_ = true;
        nameDraft_ = name_;
        keyboard_.Layout(KeyboardRect());
        keyboard_.Begin(nameDraft_);
        keyboardKey_ = -1;
        armedGalaxyDelete_ = false;
        message_ = Game::Tr("Type its name or use the keys: OK keeps it, Cancel leaves it as it was.");
    }

    std::vector<Rect> GalaxyBrowserScreen::FocusTargets() const
    {
        if (naming_) return keyboard_.Rects();
        std::vector<Rect> targets{NameRect(), ExportRect(), ImportRect(), DeleteGalaxyRect(), MainHubRect(), BonusRect()};
        if (!restoreMode_)
            for (const auto& s : std::vector<ShownPlace>{{1, MainHubRect()}, {kGalaxyBonusMission, BonusRect()}})
                if (Exists(s.mission)) targets.push_back(DeleteRect(s.rect));
        for (int world = 1; world <= kGalaxyWorldCount; ++world) targets.push_back(WorldRect(world));
        for (int place = 0; place <= kGalaxyMissionsPerWorld; ++place)
        {
            targets.push_back(PlaceRect(place));
            if (!restoreMode_ && Exists(selectedWorld_ * 10 + place)) targets.push_back(DeleteRect(PlaceRect(place)));
        }
        if (importMode_)
        {
            for (int i = 0; i < kImportRowsShown; ++i)
            {
                if (importPage_ * kImportRowsShown + i >= static_cast<int>(imports_.size())) break;
                targets.push_back(ImportRowRect(i));
                targets.push_back(ImportDeleteRect(i));
            }
            if (imports_.size() > static_cast<std::size_t>(kImportRowsShown))
            {
                targets.push_back(ImportPrevRect());
                targets.push_back(ImportNextRect());
            }
        }
        targets.push_back(BackupsRect());
        targets.push_back(BackButtonRect());
        return targets;
    }

    void GalaxyBrowserScreen::ListImports()
    {
        imports_ = ListSharedGalaxies();
        importNames_.clear();
        for (const auto& pack : imports_) importNames_.push_back(PeekGalaxyPackName(pack));
    }

    void GalaxyBrowserScreen::KeepName()
    {
        naming_ = false;
        const std::string kept = SanitizeGalaxyName(nameDraft_);
        if (kept == name_) return;
        message_ = SetGalaxyName(gamerSlot_, kept)
                       ? (kept.empty() ? std::string(Game::Tr("The galaxy has no name now.")) : Game::TrFormat("Named {0}.", {kept}))
                       : std::string(Game::Tr("The name could not be kept."));
        name_ = GalaxyName(gamerSlot_);
    }

    void GalaxyBrowserScreen::TypeKey(Microsoft::Xna::Framework::Input::Keys key, bool shift)
    {
        if (!naming_) return;
        if (GalaxyEggbert::Game::ScreenKeyboard::TypeKey(key, shift, nameDraft_, kGalaxyNameMax) ==
            GalaxyEggbert::Game::ScreenKeyboard::Result::Done)
            KeepName();
    }

    Rect GalaxyBrowserScreen::ImportPrevRect() noexcept { return {604.0f, 312.0f, 664.0f, 342.0f}; }
    Rect GalaxyBrowserScreen::ImportNextRect() noexcept { return {726.0f, 312.0f, 786.0f, 342.0f}; }

    void GalaxyBrowserScreen::Export()
    {
        const auto exported = ExportGalaxy(gamerSlot_);
        if (exported.empty())
        {
            message_ = Game::Tr("Nothing to export: the galaxy has no place yet.");
            return;
        }
        const std::string name = exported.filename().string();
        const auto recordings = GalaxyRecordings(gamerSlot_).size();
        const std::string with = recordings == 0 ? ""
                                 : Game::TrCount(" ({0} demo with it)", " ({0} demos with it)", recordings,
                                                  {std::to_string(recordings)});
        message_ = OfferDownload(exported, name) ? Game::TrFormat("Galaxy exported: downloaded as {0}{1}.", {name, with})
                                                 : Game::TrFormat("Galaxy exported to {0}{1}.", {exported.generic_string(), with});
    }

    void GalaxyBrowserScreen::ToggleImport()
    {
        importMode_ = !importMode_;
        armedImport_ = -1;
        armedImportDelete_ = -1;
        importPage_ = 0;
        if (!importMode_)
        {
            message_.clear();
            return;
        }
        restoreMode_ = false;
        ListImports();
        message_ = RequestUpload(SharedWorldsDir(), kGalaxyPackExtension)
                       ? std::string(Game::Tr("Choose a .galaxy file: it is listed on the right."))
                       : Game::TrFormat("Put .galaxy files in {0}/: they are listed on the right.",
                                        {SharedWorldsDir().generic_string()});
    }

    void GalaxyBrowserScreen::Refresh(int gamerSlot)
    {
        gamerSlot_ = gamerSlot;
        name_ = GalaxyName(gamerSlot);
        exists_.assign(200, false);
        backups_.assign(200, 0);
        std::error_code ec;
        for (int mission = 1; mission < 200; ++mission)
        {
            if (!IsGalaxyMission(mission)) continue;
            const auto path = GalaxyWorldPath(gamerSlot, mission);
            exists_[static_cast<std::size_t>(mission)] = std::filesystem::exists(path, ec);
            backups_[static_cast<std::size_t>(mission)] = BackupCount(path);
        }
        armedDelete_ = 0;
    }

    bool GalaxyBrowserScreen::Exists(int mission) const noexcept
    {
        return mission > 0 && mission < static_cast<int>(exists_.size()) && exists_[static_cast<std::size_t>(mission)];
    }

    int GalaxyBrowserScreen::Backups(int mission) const noexcept
    {
        return mission > 0 && mission < static_cast<int>(backups_.size()) ? backups_[static_cast<std::size_t>(mission)]
                                                                          : 0;
    }

    std::vector<GalaxyBrowserScreen::ShownPlace> GalaxyBrowserScreen::Shown() const
    {
        std::vector<ShownPlace> shown{{1, MainHubRect()}, {kGalaxyBonusMission, BonusRect()}};
        for (int place = 0; place <= kGalaxyMissionsPerWorld; ++place)
            shown.push_back({selectedWorld_ * 10 + place, PlaceRect(place)});
        return shown;
    }

    // As EditorBrowserScreen::Restore: the first restore of a place counts the versions it steps
    // through, so the line can say when it is back as it was.
    void GalaxyBrowserScreen::Restore(int mission)
    {
        const auto path = GalaxyWorldPath(gamerSlot_, mission);
        const std::string name = GalaxyMissionLabel(mission);
        if (restoreMission_ != mission)
        {
            restoreMission_ = mission;
            restoreSteps_ = 0;
            restoreCycle_ = Exists(mission) ? Backups(mission) + 1 : 0;
        }
        if (!RestoreBackup(path))
        {
            message_ = Game::TrFormat("{0} has no backup.", {name});
            return;
        }
        ++restoreSteps_;
        Refresh(gamerSlot_);
        // A deleted place comes back as its newest backup; one that was there steps through them.
        message_ = Game::TrFormat(restoreCycle_ == 0                   ? "{0} is back."
                                  : restoreSteps_ % restoreCycle_ == 0 ? "{0} is back as it was."
                                                                       : "{0}: backup restored. Again: an older one.",
                                  {name});
    }

    bool GalaxyBrowserScreen::WorldHasAny(int world) const noexcept
    {
        for (int place = 0; place <= kGalaxyMissionsPerWorld; ++place)
            if (Exists(world * 10 + place)) return true;
        return false;
    }

    GalaxyBrowserScreen::UpdateResult GalaxyBrowserScreen::Update(
        const Microsoft::Xna::Framework::Input::MouseState& mouse, bool backPressed)
    {
        using GalaxyEggbert::Game::QuadBatch::InRect;
        const bool down = mouse.getLeftButtonProperty() == Microsoft::Xna::Framework::Input::ButtonState::Pressed;
        const float x = static_cast<float>(mouse.getXProperty());
        const float y = static_cast<float>(mouse.getYProperty());
        mouseX_ = x;
        mouseY_ = y;
        UpdateResult result;
        if (importMode_ && ConsumeUploadDone())
        {
            ListImports();
            armedImport_ = armedImportDelete_ = -1;
            importPage_ = 0; // the file just chosen is the newest
            message_ = Game::Tr("The file is listed on the right: two clicks make it this galaxy.");
        }
        const bool backTriggered = backPressed && !backWasDown_;
        backWasDown_ = backPressed;
        if (backTriggered && naming_)
        {
            naming_ = false; // Escape: the name as it was
            message_.clear();
            mouseWasDown_ = down;
            return result;
        }
        if (naming_)
        {
            // The keyboard's keys type (pressed and released on the same key); a click elsewhere
            // keeps the name, as Enter does.
            if (down && !mouseWasDown_) keyboardKey_ = keyboard_.KeyAt(x, y);
            if (!down && mouseWasDown_)
            {
                const int key = keyboard_.KeyAt(x, y);
                if (keyboardKey_ >= 0 && key == keyboardKey_)
                {
                    const auto typed = keyboard_.Press(key, nameDraft_, kGalaxyNameMax);
                    if (typed == GalaxyEggbert::Game::ScreenKeyboard::Result::Done) KeepName();
                    else if (typed == GalaxyEggbert::Game::ScreenKeyboard::Result::Cancel)
                    {
                        naming_ = false;
                        message_.clear();
                    }
                }
                else if (keyboardKey_ < 0 && !keyboard_.Holds(x, y))
                {
                    KeepName();
                }
                keyboardKey_ = -1;
            }
            mouseWasDown_ = down;
            return result;
        }
        if (backTriggered)
        {
            result.action = Action::Back;
            armedDelete_ = 0;
            mouseWasDown_ = down;
            return result;
        }
        if (!down && mouseWasDown_ && InRect(x, y, NameRect()))
        {
            StartNaming();
            mouseWasDown_ = down;
            return result;
        }
        if (!down && mouseWasDown_)
        {
            int armed = 0;
            bool handled = false;
            const bool galaxyDeleteWasArmed = std::exchange(armedGalaxyDelete_, false);
            if (InRect(x, y, DeleteGalaxyRect()))
            {
                handled = true;
                bool any = false;
                for (int mission = 1; mission < 200 && !any; ++mission) any = Exists(mission);
                if (!any)
                {
                    message_ = Game::Tr("The galaxy has no place to delete.");
                }
                else if (!galaxyDeleteWasArmed)
                {
                    armedGalaxyDelete_ = true;
                    message_ = Game::Tr("Delete the whole galaxy? Click again (Import brings it back).");
                }
                else
                {
                    const auto kept = DeleteGalaxy(gamerSlot_);
                    Refresh(gamerSlot_);
                    restoreMode_ = false;
                    if (kept.empty())
                    {
                        message_ = Game::Tr("The galaxy could not be kept first: nothing was deleted.");
                    }
                    else
                    {
                        result.deleted = true;
                        if (importMode_) ListImports();
                        message_ = Game::TrFormat("Galaxy deleted, kept as {0} (Import brings it back).", {kept.filename().string()});
                    }
                }
            }
            else if (InRect(x, y, BackButtonRect()))
            {
                result.action = Action::Back;
                handled = true;
            }
            else if (InRect(x, y, BackupsRect()))
            {
                restoreMode_ = !restoreMode_;
                restoreMission_ = 0;
                importMode_ = false;
                message_ = restoreMode_ ? Game::Tr("Click a place to bring back its backup.") : std::string_view();
                handled = true;
            }
            else if (InRect(x, y, ExportRect()))
            {
                Export();
                handled = true;
            }
            else if (InRect(x, y, ImportRect()))
            {
                ToggleImport();
                handled = true;
            }
            int armedRow = -1;
            int armedBin = -1;
            const int pages = std::max(1, (static_cast<int>(imports_.size()) + kImportRowsShown - 1) / kImportRowsShown);
            if (importMode_ && !handled && (InRect(x, y, ImportPrevRect()) || InRect(x, y, ImportNextRect())))
            {
                const int step = InRect(x, y, ImportPrevRect()) ? -1 : 1;
                importPage_ = std::clamp(importPage_ + step, 0, pages - 1);
                handled = true;
            }
            for (int i = 0; importMode_ && !handled && i < kImportRowsShown; ++i)
            {
                const int index = importPage_ * kImportRowsShown + i;
                if (index >= static_cast<int>(imports_.size()) || !InRect(x, y, ImportRowRect(i))) continue;
                handled = true;
                const auto pack = imports_[static_cast<std::size_t>(index)];
                const std::string name = pack.filename().string();
                if (InRect(x, y, ImportDeleteRect(i)))
                {
                    if (armedImportDelete_ != index)
                    {
                        armedBin = index;
                        message_ = Game::TrFormat("Delete {0}? Click its bin again.", {name});
                        break;
                    }
                    std::error_code removed;
                    std::filesystem::remove(pack, removed);
                    ListImports();
                    const int left = std::max(1, (static_cast<int>(imports_.size()) + kImportRowsShown - 1) / kImportRowsShown);
                    importPage_ = std::min(importPage_, left - 1);
                    message_ = Game::TrFormat(removed ? "{0} could not be deleted." : "{0} deleted.", {name});
                    break;
                }
                if (armedImport_ != index)
                {
                    armedRow = index;
                    message_ = Game::TrFormat("Replace this galaxy with {0}? Click it again.", {name});
                    break;
                }
                int demos = 0;
                const int places = ImportGalaxy(gamerSlot_, pack, &demos);
                if (places < 0)
                {
                    message_ = Game::TrFormat("{0} is not a galaxy this game can read.", {name});
                    break;
                }
                Refresh(gamerSlot_);
                importMode_ = false;
                result.imported = true;
                const std::string placesText =
                    Game::TrCount("{0} place", "{0} places", places, {std::to_string(places)});
                message_ = demos == 0 ? Game::TrFormat("{0}: {1} imported (Backups brings back the old ones).", {name, placesText})
                                      : Game::TrFormat("{0}: {1} and {2} (Pause > Record > Import) imported.",
                                                       {name, placesText,
                                                        Game::TrCount("{0} demo", "{0} demos", demos,
                                                                       {std::to_string(demos)})});
            }
            armedImport_ = armedRow;
            armedImportDelete_ = armedBin;
            for (const auto& shown : Shown())
            {
                if (handled || restoreMode_ || !Exists(shown.mission) || !InRect(x, y, DeleteRect(shown.rect)))
                    continue;
                handled = true;
                if (armedDelete_ != shown.mission)
                {
                    armed = shown.mission;
                    continue;
                }
                // Into its backups: Backups brings it back.
                (void)DeleteSlotWorld(GalaxyWorldPath(gamerSlot_, shown.mission));
                Refresh(gamerSlot_);
                message_ = Game::TrFormat("{0} deleted (Backups brings it back).", {GalaxyMissionLabel(shown.mission)});
            }
            int mission = 0;
            if (!handled)
            {
                if (InRect(x, y, MainHubRect())) mission = 1;
                else if (InRect(x, y, BonusRect())) mission = kGalaxyBonusMission;
                for (int world = 1; world <= kGalaxyWorldCount; ++world)
                    if (InRect(x, y, WorldRect(world))) selectedWorld_ = world;
                for (int place = 0; place <= kGalaxyMissionsPerWorld; ++place)
                    if (InRect(x, y, PlaceRect(place))) mission = selectedWorld_ * 10 + place;
            }
            if (mission != 0 && restoreMode_)
            {
                Restore(mission);
            }
            else if (mission != 0)
            {
                result.action = Exists(mission) ? Action::Open : Action::New;
                result.mission = mission;
                result.path = GalaxyWorldPath(gamerSlot_, mission);
            }
            armedDelete_ = armed;
        }
        mouseWasDown_ = down;
        return result;
    }

    void GalaxyBrowserScreen::EnsureLoaded(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        if (loaded_) return;
        using Microsoft::Xna::Framework::Graphics::BasicEffect;
        using Microsoft::Xna::Framework::Graphics::Texture2D;
        if (!std::filesystem::exists("Content/icons/text.png") || !std::filesystem::exists("Content/icons/pad.png"))
            return;
        {
            int scale = 1;
            textTexture_ = Texture2D(Game::ScaledContentPath("Content/icons/text.png", &scale), device);
            Game::SetTextureScale(&textTexture_, scale);
        }
        textEffect_ = std::make_unique<BasicEffect>(device);
        textEffect_->VertexColorEnabled = false;
        textEffect_->setTextureEnabledProperty(true);
        textEffect_->setTextureProperty(&textTexture_);
        {
            int scale = 1;
            padTexture_ = Texture2D(Game::ScaledContentPath("Content/icons/pad.png", &scale), device);
            Game::SetTextureScale(&padTexture_, scale);
        }
        padEffect_ = std::make_unique<BasicEffect>(device);
        padEffect_->VertexColorEnabled = false;
        padEffect_->setTextureEnabledProperty(true);
        padEffect_->setTextureProperty(&padTexture_);
        loaded_ = true;
    }

    void GalaxyBrowserScreen::Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportWidth,
                                   int viewportHeight)
    {
        EnsureLoaded(device);
        if (!loaded_) return;
        using GalaxyEggbert::Game::QuadBatch::FlushQuads;
        using GalaxyEggbert::Game::QuadBatch::InRect;
        using GalaxyEggbert::Game::TextMetrics::Width;
        const float padW = Game::LogicalWidth(padTexture_);
        const float padH = Game::LogicalHeight(padTexture_);
        const float textW = Game::LogicalWidth(textTexture_);
        const float textH = Game::LogicalHeight(textTexture_);
        std::vector<Quad> panels, buttons, pressed, labels, faint;
        const auto icon = [&](std::vector<Quad>& bucket, const Rect& r, int index) {
            Quad q{r.x0, r.y0, r.x1, r.y1, 0.0f, 0.0f, 0.0f, 0.0f};
            PadIconUv(q, index, padW, padH);
            bucket.push_back(q);
        };
        const bool held = mouseWasDown_;
        const auto button = [&](const Rect& r, int index) {
            icon(held && InRect(mouseX_, mouseY_, r) ? pressed : buttons, r, index);
        };
        const auto text = [&](std::vector<Quad>& bucket, std::string_view line, float x, float y, float scale) {
            AppendLabel(bucket, line, x, y, scale, textW, textH);
        };
        // Centred on @p r, smaller when a translation is wider than the button and a little more.
        const auto centred = [&](std::vector<Quad>& bucket, std::string_view line, const Rect& r, float y, float scale) {
            const float room = r.x1 - r.x0 + 36.0f;
            const float width = Width(line) * scale;
            const float fitted = width > room && width > 0.0f ? scale * room / width : scale;
            text(bucket, line, (r.x0 + r.x1 - Width(line) * fitted) * 0.5f, y + (scale - fitted) * kGlyphCellPx * 0.5f, fitted);
        };
        // @p paragraph broken into lines no wider than @p room at @p scale, from @p y down; the next line's y.
        const auto wrapped = [&](std::vector<Quad>& bucket, std::string_view paragraph, float x, float y, float scale,
                                 float room, float step) {
            std::string line;
            std::size_t at = 0;
            while (at < paragraph.size())
            {
                const std::size_t space = paragraph.find(' ', at);
                const std::string_view word =
                    paragraph.substr(at, space == std::string_view::npos ? std::string_view::npos : space - at);
                at = space == std::string_view::npos ? paragraph.size() : space + 1;
                const std::string longer = line.empty() ? std::string(word) : line + " " + std::string(word);
                if (!line.empty() && Width(longer) * scale > room)
                {
                    text(bucket, line, x, y, scale);
                    y += step;
                    line = std::string(word);
                }
                else line = longer;
            }
            if (!line.empty())
            {
                text(bucket, line, x, y, scale);
                y += step;
            }
            return y;
        };

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        icon(panels, {10.0f, 10.0f, 590.0f, 470.0f}, kIconPanel);
        if (naming_)
            for (int layer = 0; layer < 2; ++layer) icon(panels, NameRect(), kIconPanel); // the field being typed in
        icon(panels, {598.0f, 10.0f, 792.0f, 350.0f}, kIconPanel);
        button(BackButtonRect(), kIconReturn);
        icon(restoreMode_ ? buttons : panels, BackupsRect(), restoreMode_ ? kIconBlank : kIconPanel);
        // Export and Import on a dark ground of their own: they lie over the title scene's logo.
        const Rect ground{ExportRect().x0 - 8.0f, ExportRect().y0 - 6.0f, ImportRect().x1 + 6.0f, DeleteGalaxyRect().y1 + 6.0f};
        for (int layer = 0; layer < 3; ++layer) icon(panels, ground, kIconPanel);
        icon(held && InRect(mouseX_, mouseY_, ExportRect()) ? buttons : panels, ExportRect(),
             held && InRect(mouseX_, mouseY_, ExportRect()) ? kIconBlank : kIconPanel);
        icon(importMode_ ? buttons : panels, ImportRect(), importMode_ ? kIconBlank : kIconPanel);
        {
            const bool lit = armedGalaxyDelete_ || (held && InRect(mouseX_, mouseY_, DeleteGalaxyRect()));
            icon(lit ? buttons : panels, DeleteGalaxyRect(), lit ? kIconBlank : kIconPanel);
        }
        const int importPages = std::max(1, (static_cast<int>(imports_.size()) + kImportRowsShown - 1) / kImportRowsShown);
        if (importMode_)
        {
            for (int i = 0; i < kImportRowsShown; ++i)
            {
                const int index = importPage_ * kImportRowsShown + i;
                if (index >= static_cast<int>(imports_.size())) break;
                icon(armedImport_ == index ? buttons : panels, ImportRowRect(i), armedImport_ == index ? kIconBlank : kIconPanel);
                button(ImportDeleteRect(i), armedImportDelete_ == index ? kIconConfirm : kIconTrash);
            }
            if (importPages > 1)
            {
                button(ImportPrevRect(), kIconBlank);
                button(ImportNextRect(), kIconBlank);
            }
        }
        button(MainHubRect(), kIconBlank);
        button(BonusRect(), kIconBlank);
        for (int world = 1; world <= kGalaxyWorldCount && !naming_; ++world)
        {
            const Rect r = WorldRect(world);
            if (world == selectedWorld_) icon(panels, {r.x0 - 6.0f, r.y0 - 6.0f, r.x1 + 6.0f, r.y1 + 22.0f}, kIconPanel);
            button(r, kIconBlank);
        }
        for (int place = 0; place <= kGalaxyMissionsPerWorld && !naming_; ++place) button(PlaceRect(place), kIconBlank);
        const auto shown = Shown();
        if (!restoreMode_)
        {
            for (const auto& s : shown)
                if (Exists(s.mission) && (!naming_ || s.mission == 1 || s.mission == kGalaxyBonusMission))
                    button(DeleteRect(s.rect), armedDelete_ == s.mission ? kIconConfirm : kIconTrash);
        }
        // While naming, the keyboard lies over the worlds and the places, on a dark ground.
        std::vector<Quad> keyGround, keys;
        if (naming_)
        {
            const Rect area = KeyboardRect();
            for (int layer = 0; layer < 4; ++layer)
                icon(keyGround, {area.x0 - 8.0f, area.y0 - 8.0f, area.x1 + 8.0f, area.y1 + 8.0f}, kIconPanel);
            for (std::size_t i = 0; i < keyboard_.Keys().size(); ++i)
            {
                const auto& key = keyboard_.Keys()[i];
                const bool lit = (held && keyboardKey_ == static_cast<int>(i)) ||
                                 (key.kind == GalaxyEggbert::Game::ScreenKeyboard::Kind::Shift && keyboard_.Shifted());
                icon(lit ? pressed : keys, key.rect, kIconBlank);
            }
        }
        // The arrows' choice: lit over the button, a little larger.
        std::vector<Quad> focus;
        if (focus_)
            icon(focus, {focus_->x0 - 4.0f, focus_->y0 - 4.0f, focus_->x1 + 4.0f, focus_->y1 + 4.0f}, kIconBlank);
        FlushQuads(device, *padEffect_, padRenderer_, panels, viewportWidth, viewportHeight, kPanelOpacity);
        FlushQuads(device, *padEffect_, padRenderer_, buttons, viewportWidth, viewportHeight, 1.0f);
        FlushQuads(device, *padEffect_, padRenderer_, keyGround, viewportWidth, viewportHeight, kPanelOpacity);
        FlushQuads(device, *padEffect_, padRenderer_, keys, viewportWidth, viewportHeight, 1.0f);
        FlushQuads(device, *padEffect_, padRenderer_, pressed, viewportWidth, viewportHeight, kPressedOpacity);
        FlushQuads(device, *padEffect_, padRenderer_, focus, viewportWidth, viewportHeight, kPressedOpacity);

        const std::string player = Game::TrFormat(" - Player {0}", {std::string(1, static_cast<char>('A' + gamerSlot_))});
        if (naming_)
            text(labels, Game::TrFormat("Name: {0}", {nameDraft_ + "_"}), 22.0f, 18.0f, 0.6f);
        else if (name_.empty())
        {
            const std::string title = std::string(Game::Tr("Galaxy")) + player;
            text(labels, title, 22.0f, 18.0f, 0.6f);
            text(faint, Game::Tr("(click: name it)"), 22.0f + Width(title) * 0.6f + 8.0f, 24.0f, 0.36f);
        }
        else
            text(labels, name_ + player, 22.0f, 18.0f, 0.6f);
        const auto place = [&](const Rect& r, int mission, std::string_view face, std::string_view caption) {
            const bool there = Exists(mission);
            auto& bucket = there ? labels : faint;
            centred(bucket, face, r, (r.y0 + r.y1 - kGlyphCellPx * 0.6f) * 0.5f, 0.6f);
            if (restoreMode_)
            {
                const bool has = Backups(mission) > 0;
                centred(has ? labels : faint, has ? Game::Tr("Restore") : Game::Tr("No backup"), r, r.y1 + 3.0f, 0.4f);
                return;
            }
            centred(bucket, !there ? Game::Tr("New") : armedDelete_ == mission ? Game::Tr("Delete?") : caption, r, r.y1 + 3.0f,
                    0.4f);
        };
        place(MainHubRect(), 1, Game::Tr("Hub"), Game::Tr("Main hub"));
        place(BonusRect(), kGalaxyBonusMission, "199", Game::Tr("Bonus"));
        if (naming_)
        {
            for (const auto& key : keyboard_.Keys())
            {
                const std::string face = keyboard_.Label(key);
                const float scale = face.size() == 1 ? 0.6f : 0.42f;
                centred(labels, face, key.rect, (key.rect.y0 + key.rect.y1 - kGlyphCellPx * scale) * 0.5f, scale);
            }
        }
        else
        {
            text(labels, Game::Tr("Worlds (a click selects one)"), 22.0f, 146.0f, 0.45f);
            for (int world = 1; world <= kGalaxyWorldCount; ++world)
            {
                const Rect r = WorldRect(world);
                auto& bucket = WorldHasAny(world) ? labels : faint;
                const std::string number = std::to_string(world);
                centred(bucket, number, r, (r.y0 + r.y1 - kGlyphCellPx * 0.8f) * 0.5f, 0.8f);
                centred(bucket, Game::TrFormat("World {0}", {number}), r, r.y1 + 3.0f, 0.38f);
            }
            text(labels, Game::TrFormat("World {0}: its hub and missions", {std::to_string(selectedWorld_)}), 22.0f, 340.0f,
                 0.45f);
            for (int p = 0; p <= kGalaxyMissionsPerWorld; ++p)
            {
                const int mission = selectedWorld_ * 10 + p;
                const std::string number = std::to_string(p);
                place(PlaceRect(p), mission, p == 0 ? std::string(Game::Tr("Hub")) : number,
                      p == 0 ? Game::TrFormat("Hub {0}", {std::to_string(mission)}) : std::to_string(mission));
            }
        }
        // The help beside the worlds, its paragraphs broken into the column's lines (a translation's
        // words fall where they fall).
        const char* help[] = {
            Game::TrMark("Your own galaxy, played from the main menu's Galaxy button."),
            "",
            Game::TrMark("Main hub: world signs 1-14 lead to the worlds."),
            Game::TrMark("World hub: mission signs 1-8, a door beside each but the first opens when the one before is won; "
                         "the golden key ends the world. The return sign leads back."),
            Game::TrMark("Faint: not made yet; a click makes it. The bin deletes (two clicks); Backups brings it back."),
        };
        if (importMode_)
        {
            text(labels, Game::Tr("Import a galaxy"), 606.0f, 18.0f, 0.45f);
            if (imports_.empty())
                wrapped(faint, Game::Tr("No .galaxy file to import yet."), 606.0f, 54.0f, 0.36f, 180.0f, 19.0f);
            for (int i = 0; i < kImportRowsShown; ++i)
            {
                const int index = importPage_ * kImportRowsShown + i;
                if (index >= static_cast<int>(imports_.size())) break;
                const Rect row = ImportRowRect(i);
                const std::string& packName = importNames_[static_cast<std::size_t>(index)];
                std::string name = armedImport_ == index         ? std::string(Game::Tr("Replace? Click again"))
                                   : armedImportDelete_ == index ? std::string(Game::Tr("Delete? Its bin again"))
                                   : !packName.empty()           ? packName
                                                                 : imports_[static_cast<std::size_t>(index)].stem().string();
                while (!name.empty() && Width(name) * 0.36f > row.x1 - row.x0 - 40.0f) Game::TextMetrics::PopBack(name);
                text(labels, name, row.x0 + 6.0f, (row.y0 + row.y1 - kGlyphCellPx * 0.36f) * 0.5f, 0.36f);
            }
            if (importPages > 1)
            {
                const float mid = (ImportPrevRect().y0 + ImportPrevRect().y1 - kGlyphCellPx * 0.5f) * 0.5f;
                centred(importPage_ > 0 ? labels : faint, "<", ImportPrevRect(), mid, 0.5f);
                centred(importPage_ + 1 < importPages ? labels : faint, ">", ImportNextRect(), mid, 0.5f);
                centred(labels, std::to_string(importPage_ + 1) + " / " + std::to_string(importPages),
                        Rect{ImportPrevRect().x1, ImportPrevRect().y0, ImportNextRect().x0, ImportNextRect().y1},
                        (ImportPrevRect().y0 + ImportPrevRect().y1 - kGlyphCellPx * 0.4f) * 0.5f, 0.4f);
            }
        }
        else
        {
            float y = 20.0f;
            for (const char* paragraph : help)
                y = *paragraph == '\0' ? y + 19.0f : wrapped(labels, Game::Tr(paragraph), 606.0f, y, 0.36f, 180.0f, 19.0f);
        }
        for (const auto& [rect, caption] : {std::pair<Rect, std::string_view>{ExportRect(), Game::Tr("Export")},
                                            std::pair<Rect, std::string_view>{ImportRect(), Game::Tr("Import")}})
            centred(labels, caption, rect, (rect.y0 + rect.y1 - kGlyphCellPx * 0.45f) * 0.5f, 0.45f);
        centred(labels, Game::Tr("Backups"), BackupsRect(), (BackupsRect().y0 + BackupsRect().y1 - kGlyphCellPx * 0.4f) * 0.5f,
                0.4f);
        centred(labels, armedGalaxyDelete_ ? Game::Tr("Delete all? Again") : Game::Tr("Delete galaxy"), DeleteGalaxyRect(),
                (DeleteGalaxyRect().y0 + DeleteGalaxyRect().y1 - kGlyphCellPx * 0.4f) * 0.5f, 0.4f);
        if (!message_.empty()) text(labels, message_, 22.0f, 446.0f, 0.4f);
        FlushQuads(device, *textEffect_, textRenderer_, labels, viewportWidth, viewportHeight, 1.0f);
        FlushQuads(device, *textEffect_, textRenderer_, faint, viewportWidth, viewportHeight, kFaintOpacity);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }
}
