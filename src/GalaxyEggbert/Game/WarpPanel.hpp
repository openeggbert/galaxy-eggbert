#pragma once

// `warp` (MODERN, brief D of the owner's cheats, 2026-11-14): a panel of the worlds the player can go
// to directly -- the campaign's main hub, bonus world and showcase, then each world's hub and missions
// (in a player's galaxy, that galaxy's) -- chosen by a click or touch, or by the arrows (keyboard,
// gamepad d-pad) and A / Enter (FocusNavigator); Escape or B closes it. Engine-independent and
// driven from the recorded tick input, as the cheats' help is: a replay of the world's log opens,
// moves and closes it exactly as play did. The host draws it with InputPad::DrawPanel().

#include <GalaxyEggbert/Game/FocusNavigator.hpp>
#include <GalaxyEggbert/Game/QuadBatch.hpp>

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace GalaxyEggbert::Game
{
    struct WarpRow
    {
        std::string label;         // "World 3"; empty for the first row (the hub of hubs and the rest)
        std::vector<int> missions; // in order, a world's hub first
    };

    // A place the campaign or a player's galaxy numbers (Editor::IsGalaxyMission's rule): the main hub
    // 1, the world hubs 10 ... 140 and their missions 1-8, the bonus world 199; and the campaign's
    // showcase 999, the main hub's sign 13 (WorldRuntime::ComputeWorldSelectTarget). Not the gallery
    // 998, the test worlds or the multiplayer worlds 201 and up.
    [[nodiscard]] inline bool IsWarpMission(int mission, bool galaxy) noexcept
    {
        if (mission == 1 || mission == 199) return true;
        if (mission == 999) return !galaxy;
        const int world = mission / 10, place = mission % 10;
        return world >= 1 && world <= 14 && place <= 8;
    }

    // The name the game has for a place (Editor::GalaxyMissionName's, English as the cheats are); ""
    // where it has none of its own: a world's missions show their numbers under the world's name.
    [[nodiscard]] inline std::string WarpPlaceName(int mission)
    {
        if (mission == 1) return "Main hub";
        if (mission == 199) return "Bonus world";
        return {};
    }

    // The panel's rows from the places whose world file exists (@p exists, asked for each candidate).
    [[nodiscard]] inline std::vector<WarpRow> BuildWarpRows(const std::function<bool(int)>& exists, bool galaxy)
    {
        std::vector<WarpRow> rows;
        WarpRow first;
        for (const int mission : {1, 199, 999})
            if (IsWarpMission(mission, galaxy) && exists(mission)) first.missions.push_back(mission);
        if (!first.missions.empty()) rows.push_back(std::move(first));
        for (int world = 1; world <= 14; ++world)
        {
            WarpRow row;
            row.label = "World " + std::to_string(world);
            for (int place = 0; place <= 8; ++place)
                if (IsWarpMission(world * 10 + place, galaxy) && exists(world * 10 + place))
                    row.missions.push_back(world * 10 + place);
            if (!row.missions.empty()) rows.push_back(std::move(row));
        }
        return rows;
    }

    class WarpPanel
    {
    public:
        struct Button
        {
            QuadBatch::Rect rect{0.0f, 0.0f, 0.0f, 0.0f}; // screen pixels
            int mission = -1;                              // -1: Cancel
            std::string label;
        };
        struct Caption
        {
            std::string text;
            float x = 0.0f, y = 0.0f; // screen pixels, the left and the top
            float size = 0.34f;
        };
        struct Input
        {
            bool up = false, down = false, left = false, right = false; // held this tick
            bool confirm = false;                                       // Enter (a gamepad's A)
            bool cancel = false;                                        // Escape, Space (a gamepad's B)
            bool pointerDown = false;                                   // the mouse's left button or a finger
            float x = 0.0f, y = 0.0f;                                   // where it is (or was let go)
        };
        struct Output
        {
            int chosen = -1;     // the mission chosen this tick
            bool closed = false; // closed this tick without a choice
        };

        // Keys and a press already held as it opens (the `p` of `warp`, a word clicked in the help)
        // count only once they have been let go.
        void Open(std::vector<WarpRow> rows, std::string cancelLabel = "Cancel")
        {
            *this = {};
            rows_ = std::move(rows);
            cancelLabel_ = std::move(cancelLabel);
            open_ = true;
        }
        void Close() noexcept
        {
            open_ = false;
            pressed_ = -1;
            focus_.Reset();
        }
        [[nodiscard]] bool IsOpen() const noexcept { return open_; }
        [[nodiscard]] const std::vector<WarpRow>& Rows() const noexcept { return rows_; }

        // The buttons and captions for a viewport of @p w x @p h: the 640x480 reference panel scaled by
        // the height and centred (InputPad::PanelRect()), the worlds in two columns.
        void Layout(int w, int h)
        {
            buttons_.clear();
            captions_.clear();
            const float s = static_cast<float>(h) / 480.0f;
            const float cx = static_cast<float>(w) * 0.5f;
            const auto rect = [&](float x0, float y0, float x1, float y1) {
                return QuadBatch::Rect{cx + (x0 - 320.0f) * s, y0 * s, cx + (x1 - 320.0f) * s, y1 * s};
            };
            std::size_t first = 0;
            if (!rows_.empty() && rows_.front().label.empty())
            {
                first = 1;
                float x = 20.0f;
                for (const int mission : rows_.front().missions)
                {
                    const std::string name = WarpPlaceName(mission);
                    const std::string label = std::to_string(mission) + (name.empty() ? "" : "  " + name);
                    const float width = name.empty() ? 64.0f : 176.0f;
                    buttons_.push_back({rect(x, 84.0f, x + width, 118.0f), mission, label});
                    x += width + 10.0f;
                }
            }
            const int worlds = static_cast<int>(rows_.size() - first);
            const int perColumn = std::max(1, (worlds + 1) / 2);
            const float top = first ? 126.0f : 84.0f;
            const float rowHeight = std::min(52.0f, (418.0f - top) / static_cast<float>(perColumn));
            for (int i = 0; i < worlds; ++i)
            {
                const auto& row = rows_[first + static_cast<std::size_t>(i)];
                const float x0 = i < perColumn ? 16.0f : 324.0f;
                const float y0 = top + static_cast<float>(i % perColumn) * rowHeight;
                const auto caption = rect(x0, y0, x0, y0);
                captions_.push_back({row.label, caption.x0, caption.y0, 0.34f});
                for (std::size_t m = 0; m < row.missions.size(); ++m)
                {
                    const float bx = x0 + static_cast<float>(m) * (300.0f / 9.0f);
                    buttons_.push_back({rect(bx, y0 + 13.0f, bx + 31.0f, y0 + rowHeight - 3.0f), row.missions[m],
                                        std::to_string(row.missions[m])});
                }
            }
            buttons_.push_back({rect(260.0f, 426.0f, 380.0f, 460.0f), -1, cancelLabel_});
        }
        [[nodiscard]] const std::vector<Button>& Buttons() const noexcept { return buttons_; }
        [[nodiscard]] const std::vector<Caption>& Captions() const noexcept { return captions_; }
        // The button the pointer holds down, -1 none.
        [[nodiscard]] int Pressed() const noexcept { return pressed_; }
        // The arrows' choice, -1 while they are not in use.
        [[nodiscard]] int Focused() const noexcept { return focus_.Focused(); }

        // One recorded tick of the open panel.
        Output Update(const Input& in, int w, int h)
        {
            Output out;
            if (!open_) return out;
            Layout(w, h);
            unsigned now = 0;
            const bool held[] = {in.up, in.down, in.left, in.right, in.confirm, in.cancel};
            for (unsigned i = 0; i < std::size(held); ++i)
                if (held[i]) now |= 1u << i;
            const unsigned pressed = now & ~keysWas_;
            keysWas_ = now;
            const auto finish = [&](int at) {
                const int mission = buttons_[static_cast<std::size_t>(at)].mission;
                Close();
                if (mission >= 0) out.chosen = mission;
                else out.closed = true;
            };
            if (pressed & (1u << 5))
            {
                Close();
                out.closed = true;
                return out;
            }
            const bool moved = hasLast_ && (in.x != lastX_ || in.y != lastY_);
            lastX_ = in.x;
            lastY_ = in.y;
            hasLast_ = true;
            // A click: pressed and let go over the same button.
            const int over = ButtonAt(in.x, in.y);
            if (in.pointerDown && !pointerWas_) pressed_ = over;
            if (!in.pointerDown && pointerWas_)
            {
                const int at = pressed_;
                pressed_ = -1;
                pointerWas_ = false;
                if (at >= 0 && at == over)
                {
                    finish(at);
                    return out;
                }
            }
            pointerWas_ = in.pointerDown;
            std::vector<QuadBatch::Rect> rects;
            for (const auto& b : buttons_) rects.push_back(b.rect);
            FocusNavigator::Input nav;
            nav.up = pressed & 1u;
            nav.down = pressed & 2u;
            nav.left = pressed & 4u;
            nav.right = pressed & 8u;
            nav.confirm = pressed & 16u;
            nav.pointer = moved || in.pointerDown;
            const auto focus = focus_.Update(rects, nav);
            if (focus.pressed && focus.focused >= 0 && static_cast<std::size_t>(focus.focused) < buttons_.size())
                finish(focus.focused);
            return out;
        }

        // For the in-process replay check (GalaxyEggbertGame::ResumeCheckSignature()).
        [[nodiscard]] std::string StateSignature() const
        {
            std::ostringstream out;
            out << std::setprecision(9) << open_ << ' ' << rows_.size() << ' ' << keysWas_ << ' ' << pointerWas_
                << ' ' << pressed_ << ' ' << hasLast_ << ' ' << lastX_ << ' ' << lastY_ << '|'
                << focus_.StateSignature();
            return out.str();
        }

    private:
        [[nodiscard]] int ButtonAt(float x, float y) const noexcept
        {
            for (std::size_t i = 0; i < buttons_.size(); ++i)
            {
                const auto& r = buttons_[i].rect;
                if (x >= r.x0 && x < r.x1 && y >= r.y0 && y < r.y1) return static_cast<int>(i);
            }
            return -1;
        }

        std::vector<WarpRow> rows_;
        std::string cancelLabel_ = "Cancel";
        std::vector<Button> buttons_;
        std::vector<Caption> captions_;
        FocusNavigator focus_;
        bool open_ = false;
        unsigned keysWas_ = ~0u;  // held as it opens: not a press
        bool pointerWas_ = true; // likewise
        int pressed_ = -1;
        bool hasLast_ = false;
        float lastX_ = 0.0f, lastY_ = 0.0f;
    };
}
