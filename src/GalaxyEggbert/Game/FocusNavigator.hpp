#pragma once

#include <GalaxyEggbert/Game/QuadBatch.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace GalaxyEggbert::Game
{
    // Choosing a screen's buttons with the arrows (keyboard, gamepad d-pad) and pressing the chosen
    // one with A or Enter (2026-11-10, the owner: the demo positions, the import lists and the
    // galaxy screen were mouse and touch only). The screen gives the rects of what can be pressed
    // (its targets, in the space of the mouse it reads); the navigator keeps one of them chosen and
    // hands the screen a mouse at that target's centre, pressed for one update when it is
    // confirmed and released the next, so the screen does what a click there does. An arrow
    // starts it (the first press only shows the choice); moving the mouse, a touch or the
    // gamepad's cursor ends it, and the screen sees the real mouse again.
    class FocusNavigator
    {
    public:
        struct Input
        {
            bool up = false, down = false, left = false, right = false; // pressed this update
            bool confirm = false;                                       // A or Enter, pressed this update
            bool pointer = false;                                       // the mouse, a touch or a cursor moved
        };
        struct Output
        {
            bool active = false;  // a target is chosen: the screen reads the mouse below
            int focused = -1;     // its index in the targets
            float x = 0.0f, y = 0.0f;
            bool pressed = false; // the left button, held for the update after a confirm
        };

        Output Update(const std::vector<QuadBatch::Rect>& targets, const Input& in)
        {
            Output out;
            if (clickPhase_ == 1)
            {
                // The release of the click pressed last update, where it was pressed.
                clickPhase_ = 0;
                out.active = true;
                out.focused = focused_;
                out.x = clickX_;
                out.y = clickY_;
                return out;
            }
            if (in.pointer)
            {
                active_ = false;
                return out;
            }
            const bool arrow = in.up || in.down || in.left || in.right;
            if (targets.empty())
            {
                active_ = false;
                focused_ = -1;
                return out;
            }
            if (focused_ < 0 || static_cast<std::size_t>(focused_) >= targets.size() || targets.size() != count_)
                focused_ = hasLast_ ? Nearest(targets, CentreX(last_), CentreY(last_)) : 0; // the screen's first
            count_ = targets.size();
            if (!active_)
            {
                if (!arrow && !in.confirm) return out;
                active_ = true; // the first press shows where the choice is
            }
            else if (arrow)
            {
                const int dx = (in.right ? 1 : 0) - (in.left ? 1 : 0);
                const int dy = (in.down ? 1 : 0) - (in.up ? 1 : 0);
                focused_ = Neighbour(targets, focused_, dx, dy);
            }
            else if (in.confirm)
            {
                clickPhase_ = 1;
                out.pressed = true;
            }
            last_ = targets[static_cast<std::size_t>(focused_)];
            hasLast_ = true;
            out.active = true;
            out.focused = focused_;
            out.x = CentreX(last_);
            out.y = CentreY(last_);
            clickX_ = out.x;
            clickY_ = out.y;
            return out;
        }

        [[nodiscard]] bool Active() const noexcept { return active_ || clickPhase_ == 1; }
        [[nodiscard]] int Focused() const noexcept { return active_ ? focused_ : -1; }
        // A screen left or opened: the next arrow starts from its first target.
        void Reset() noexcept
        {
            active_ = false;
            focused_ = -1;
            hasLast_ = false;
            clickPhase_ = 0;
            count_ = 0;
        }

        // The target an arrow (dx, dy: -1, 0 or 1) leads to from @p from: of those whose centre lies
        // that way, the nearest of those in its row (or column: overlapping it across the arrow),
        // else the nearest, the gap across the arrow counting thrice; @p from itself when there is
        // none.
        [[nodiscard]] static int Neighbour(const std::vector<QuadBatch::Rect>& targets, int from, int dx, int dy)
        {
            if (from < 0 || static_cast<std::size_t>(from) >= targets.size() || (dx == 0 && dy == 0)) return from;
            const QuadBatch::Rect& a = targets[static_cast<std::size_t>(from)];
            const float ax = CentreX(a);
            const float ay = CentreY(a);
            int best = from;
            float bestScore = std::numeric_limits<float>::max();
            for (std::size_t i = 0; i < targets.size(); ++i)
            {
                if (static_cast<int>(i) == from) continue;
                const float ddx = CentreX(targets[i]) - ax;
                const float ddy = CentreY(targets[i]) - ay;
                const float along = ddx * static_cast<float>(dx) + ddy * static_cast<float>(dy);
                if (along <= 1.0f) continue;
                const QuadBatch::Rect& b = targets[i];
                const float gap = dx != 0 ? std::max(0.0f, std::max(a.y0, b.y0) - std::min(a.y1, b.y1))
                                          : std::max(0.0f, std::max(a.x0, b.x0) - std::min(a.x1, b.x1));
                const float across = std::fabs(ddx * static_cast<float>(dy) - ddy * static_cast<float>(dx));
                // A target in the same row (or column) always before one off it, however near.
                const float score = along + (gap > 0.0f ? 100000.0f + gap * 3.0f : 0.0f) + across * 0.1f;
                if (score < bestScore)
                {
                    bestScore = score;
                    best = static_cast<int>(i);
                }
            }
            return best;
        }

    private:
        [[nodiscard]] static float CentreX(const QuadBatch::Rect& r) noexcept { return (r.x0 + r.x1) * 0.5f; }
        [[nodiscard]] static float CentreY(const QuadBatch::Rect& r) noexcept { return (r.y0 + r.y1) * 0.5f; }
        [[nodiscard]] static int Nearest(const std::vector<QuadBatch::Rect>& targets, float x, float y)
        {
            int best = 0;
            float bestDistance = std::numeric_limits<float>::max();
            for (std::size_t i = 0; i < targets.size(); ++i)
            {
                const float ddx = CentreX(targets[i]) - x, ddy = CentreY(targets[i]) - y;
                const float distance = ddx * ddx + ddy * ddy;
                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    best = static_cast<int>(i);
                }
            }
            return best;
        }

        bool active_ = false;
        int focused_ = -1;
        QuadBatch::Rect last_{0.0f, 0.0f, 0.0f, 0.0f};
        bool hasLast_ = false;
        std::size_t count_ = 0;
        int clickPhase_ = 0; // 1: pressed last update, released this one
        float clickX_ = 0.0f, clickY_ = 0.0f;
    };
}
