#include "GalaxyBrowserScreen.hpp"

#include "CustomWorldStorage.hpp"

#include <GalaxyEggbert/Game/TextMetrics.hpp>

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
        constexpr int kIconPanel = 15;
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

        void AppendLabel(std::vector<Quad>& quads, const std::string& text, float leftX, float topY, float scale,
                         float sheetW, float sheetH)
        {
            float penX = leftX;
            for (const char c : text)
            {
                const int rank = GalaxyEggbert::Game::TextMetrics::GlyphIndex(c);
                const int col = rank % kGlyphCols;
                const int row = rank / kGlyphCols;
                Quad q{penX, topY, penX + kGlyphCellPx * scale, topY + kGlyphCellPx * scale,
                       static_cast<float>(col) * kGlyphCellPx / sheetW, static_cast<float>(row) * kGlyphCellPx / sheetH,
                       static_cast<float>(col + 1) * kGlyphCellPx / sheetW,
                       static_cast<float>(row + 1) * kGlyphCellPx / sheetH};
                quads.push_back(q);
                penX += GalaxyEggbert::Game::TextMetrics::Advance(c) * scale;
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

    Rect GalaxyBrowserScreen::BackButtonRect() noexcept { return {680.0f, 360.0f, 780.0f, 460.0f}; }

    void GalaxyBrowserScreen::Refresh(int gamerSlot)
    {
        gamerSlot_ = gamerSlot;
        exists_.assign(200, false);
        std::error_code ec;
        for (int mission = 1; mission < 200; ++mission)
        {
            if (IsGalaxyMission(mission))
                exists_[static_cast<std::size_t>(mission)] = std::filesystem::exists(GalaxyWorldPath(gamerSlot, mission), ec);
        }
    }

    bool GalaxyBrowserScreen::Exists(int mission) const noexcept
    {
        return mission > 0 && mission < static_cast<int>(exists_.size()) && exists_[static_cast<std::size_t>(mission)];
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
        const bool backTriggered = backPressed && !backWasDown_;
        backWasDown_ = backPressed;
        if (backTriggered)
        {
            result.action = Action::Back;
            mouseWasDown_ = down;
            return result;
        }
        if (!down && mouseWasDown_)
        {
            int mission = 0;
            if (InRect(x, y, BackButtonRect())) result.action = Action::Back;
            else if (InRect(x, y, MainHubRect())) mission = 1;
            else if (InRect(x, y, BonusRect())) mission = kGalaxyBonusMission;
            for (int world = 1; world <= kGalaxyWorldCount; ++world)
                if (InRect(x, y, WorldRect(world))) selectedWorld_ = world;
            for (int place = 0; place <= kGalaxyMissionsPerWorld; ++place)
                if (InRect(x, y, PlaceRect(place))) mission = selectedWorld_ * 10 + place;
            if (mission != 0)
            {
                result.action = Exists(mission) ? Action::Open : Action::New;
                result.mission = mission;
                result.path = GalaxyWorldPath(gamerSlot_, mission);
            }
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
        textTexture_ = Texture2D("Content/icons/text.png", device);
        textEffect_ = std::make_unique<BasicEffect>(device);
        textEffect_->VertexColorEnabled = false;
        textEffect_->setTextureEnabledProperty(true);
        textEffect_->setTextureProperty(&textTexture_);
        padTexture_ = Texture2D("Content/icons/pad.png", device);
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
        const float padW = static_cast<float>(padTexture_.getWidthProperty());
        const float padH = static_cast<float>(padTexture_.getHeightProperty());
        const float textW = static_cast<float>(textTexture_.getWidthProperty());
        const float textH = static_cast<float>(textTexture_.getHeightProperty());
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
        const auto text = [&](std::vector<Quad>& bucket, const std::string& line, float x, float y, float scale) {
            AppendLabel(bucket, line, x, y, scale, textW, textH);
        };
        const auto centred = [&](std::vector<Quad>& bucket, const std::string& line, const Rect& r, float y,
                                 float scale) { text(bucket, line, (r.x0 + r.x1 - Width(line) * scale) * 0.5f, y, scale); };

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        icon(panels, {10.0f, 10.0f, 590.0f, 470.0f}, kIconPanel);
        icon(panels, {598.0f, 10.0f, 792.0f, 350.0f}, kIconPanel);
        button(BackButtonRect(), kIconReturn);
        button(MainHubRect(), kIconBlank);
        button(BonusRect(), kIconBlank);
        for (int world = 1; world <= kGalaxyWorldCount; ++world)
        {
            const Rect r = WorldRect(world);
            if (world == selectedWorld_) icon(panels, {r.x0 - 6.0f, r.y0 - 6.0f, r.x1 + 6.0f, r.y1 + 22.0f}, kIconPanel);
            button(r, kIconBlank);
        }
        for (int place = 0; place <= kGalaxyMissionsPerWorld; ++place) button(PlaceRect(place), kIconBlank);
        FlushQuads(device, *padEffect_, padRenderer_, panels, viewportWidth, viewportHeight, kPanelOpacity);
        FlushQuads(device, *padEffect_, padRenderer_, buttons, viewportWidth, viewportHeight, 1.0f);
        FlushQuads(device, *padEffect_, padRenderer_, pressed, viewportWidth, viewportHeight, kPressedOpacity);

        text(labels, std::string("Galaxy - Player ") + static_cast<char>('A' + gamerSlot_), 22.0f, 18.0f, 0.6f);
        const auto place = [&](const Rect& r, int mission, const std::string& face, const std::string& caption) {
            const bool there = Exists(mission);
            auto& bucket = there ? labels : faint;
            centred(bucket, face, r, (r.y0 + r.y1 - kGlyphCellPx * 0.6f) * 0.5f, 0.6f);
            centred(bucket, there ? caption : "New", r, r.y1 + 3.0f, 0.4f);
        };
        place(MainHubRect(), 1, "Hub", "Main hub");
        place(BonusRect(), kGalaxyBonusMission, "199", "Bonus");
        text(labels, "Worlds (a click selects one)", 22.0f, 146.0f, 0.45f);
        for (int world = 1; world <= kGalaxyWorldCount; ++world)
        {
            const Rect r = WorldRect(world);
            auto& bucket = WorldHasAny(world) ? labels : faint;
            const std::string number = std::to_string(world);
            centred(bucket, number, r, (r.y0 + r.y1 - kGlyphCellPx * 0.8f) * 0.5f, 0.8f);
            centred(bucket, "World " + number, r, r.y1 + 3.0f, 0.38f);
        }
        text(labels, "World " + std::to_string(selectedWorld_) + ": its hub and missions", 22.0f, 340.0f, 0.45f);
        for (int p = 0; p <= kGalaxyMissionsPerWorld; ++p)
        {
            const int mission = selectedWorld_ * 10 + p;
            place(PlaceRect(p), mission, p == 0 ? "Hub" : std::to_string(p),
                  p == 0 ? "Hub " + std::to_string(mission) : std::to_string(mission));
        }
        const char* help[] = {
            "Your own galaxy, played",
            "from the main menu's",
            "Galaxy button.",
            "",
            "Main hub: world signs",
            "1-14 lead to the worlds.",
            "World hub: mission signs",
            "1-8, a door beside each",
            "but the first opens when",
            "the one before is won;",
            "the golden key ends the",
            "world. The return sign",
            "leads back.",
            "",
            "Faint: not made yet; a",
            "click makes it.",
        };
        float y = 20.0f;
        for (const char* line : help)
        {
            text(labels, line, 606.0f, y, 0.36f);
            y += 19.0f;
        }
        FlushQuads(device, *textEffect_, textRenderer_, labels, viewportWidth, viewportHeight, 1.0f);
        FlushQuads(device, *textEffect_, textRenderer_, faint, viewportWidth, viewportHeight, kFaintOpacity);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }
}
