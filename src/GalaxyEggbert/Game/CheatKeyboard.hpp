#pragma once

#include <GalaxyEggbert/Game/FocusNavigator.hpp>
#include <Microsoft/Xna/Framework/Input/KeyboardState.hpp>

#include <algorithm>
#include <deque>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace GalaxyEggbert::Game
{
    // Mobile MODERN InputPad.cpp:274-330,456-575 supplies the behavior, not copied code.
    // Galaxy's owner: centre circle, radius 11% of the smaller side, 120 recorded ticks.
    // Keys fire on a fresh press, never on drag/release/repeat. The opening press is consumed.
    class CheatKeyboard
    {
    public:
        using Keys = Microsoft::Xna::Framework::Input::Keys;
        using KeyboardState = Microsoft::Xna::Framework::Input::KeyboardState;
        static constexpr Keys kHoldKey = Keys::OemPipe; // LT; unused by play, menus and demo filters
        static constexpr int kHoldTicks = 120;
        struct Pointer { int id; float x, y; bool claimed = false; }; // active only; mouse id -2
        struct Key { Keys code; QuadBatch::Rect rect; };
        struct Output { bool capture = false; Keys key = Keys::None; };

        void Layout(int w, int h)
        {
            keys_.clear();
            const float gap = std::max(2.0f, std::min(w, h) / 120.0f);
            const float unit = (w - 2.0f * gap) / 10.0f;
            const float height = std::min(unit - gap, h * 0.125f); // >=43px at 800x360
            const auto key = [&](Keys code, float col, int row, float units = 1.0f) {
                const float x = gap + col * unit, y = gap + row * (height + gap);
                keys_.push_back({code, {x,y,x + units * unit - gap,y + height}});
            };
            // The close key (Keys::None) two units wide with a word (Cancel), never a lone "X" that
            // reads as the letter X of the bottom row.
            key(Keys::F5,0,0); key(Keys::F4,1,0); key(Keys::F6,2,0);
            key(Keys::F7,3,0); key(Keys::F8,4,0); key(Keys::F12,7,0); key(Keys::None,8,0,2.0f);
            const char* rows[] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
            for (int row = 0; row < 3; ++row)
                for (int col = 0; rows[row][col]; ++col)
                    key(static_cast<Keys>(int(Keys::A) + rows[row][col] - 'a'), col + row * 0.5f, row + 1);
        }
        [[nodiscard]] const std::vector<Key>& KeysOnScreen() const { return keys_; }
        [[nodiscard]] bool Visible() const { return visible_; }
        [[nodiscard]] int Focused() const { return focus_.Focused(); }
        [[nodiscard]] int KeyAt(float x, float y) const
        {
            for (std::size_t i = 0; i < keys_.size(); ++i)
            {
                const auto& r = keys_[i].rect;
                if (x >= r.x0 && x < r.x1 && y >= r.y0 && y < r.y1) return int(i);
            }
            return -1;
        }
        void Close() { visible_ = false; focus_.Reset(); }
        void Reset() { *this = CheatKeyboard{}; }

        Output Update(bool allowed, int w, int h, const KeyboardState& keyboard,
                      const std::vector<Pointer>& pointers)
        {
            Layout(w,h);
            Output out;
            out.capture = visible_;
            unsigned now = 0;
            const Keys nav[] = {Keys::Up,Keys::Down,Keys::Left,Keys::Right,Keys::Enter,Keys::LeftControl,
                                Keys::Space,Keys::Escape,kHoldKey};
            for (unsigned i = 0; i < std::size(nav); ++i) if (keyboard.IsKeyDown(nav[i])) now |= 1u << i;
            const unsigned pressed = now & ~previousKeys_;
            previousKeys_ = now;
            if (!allowed) { Close(); pendingKeys_.clear(); }
            const bool wasVisible = visible_;
            const bool hold = keyboard.IsKeyDown(kHoldKey);
            if (!hold) { triggerTicks_ = 0; triggerConsumed_ = false; }
            else if (!allowed) { triggerTicks_ = 0; triggerConsumed_ = true; }
            else if (!triggerConsumed_ && ++triggerTicks_ >= kHoldTicks)
            {
                triggerConsumed_ = true;
                if (visible_) Close(); else visible_ = true;
            }
            std::set<int> seen;
            for (const auto& p : pointers)
            {
                seen.insert(p.id);
                auto [it, fresh] = pointers_.try_emplace(p.id, Hold{});
                auto& state = it->second;
                const float dx = p.x - w * 0.5f, dy = p.y - h * 0.5f;
                const float radius = std::min(w,h) * 0.11f;
                if (!allowed || p.claimed || dx*dx + dy*dy > radius*radius || wasVisible)
                { state.blocked = true; state.ticks = 0; }
                if (!visible_ && !state.blocked && ++state.ticks >= kHoldTicks)
                {
                    visible_ = true;
                    state.blocked = true;
                }
                if (wasVisible && fresh && allowed)
                {
                    const int at = KeyAt(p.x,p.y);
                    if (at >= 0) Fire(at,out);
                }
            }
            for (auto it = pointers_.begin(); it != pointers_.end();)
                if (!seen.contains(it->first)) it = pointers_.erase(it); else ++it;
            if (wasVisible && allowed)
            {
                if ((pressed & ((1u << 6) | (1u << 7))) != 0) Close(); // B/X, Escape/Pause
                else if (visible_)
                {
                    std::vector<QuadBatch::Rect> rects;
                    for (const auto& k : keys_) rects.push_back(k.rect);
                    FocusNavigator::Input in;
                    in.up = pressed & 1u; in.down = pressed & 2u;
                    in.left = pressed & 4u; in.right = pressed & 8u;
                    in.confirm = pressed & ((1u << 4) | (1u << 5)); // Enter/A
                    in.pointer = !pointers.empty();
                    const auto focus = focus_.Update(rects,in);
                    if (focus.pressed) Fire(focus.focused,out);
                }
            }
            if (allowed && !pendingKeys_.empty())
            {
                out.key = pendingKeys_.front();
                pendingKeys_.pop_front();
            }
            out.capture = out.capture || visible_;
            // Retain suppression through release after closing (A must not become a jump).
            if (out.capture) suppressed_ |= now & 0x7fu;
            suppressed_ &= now;
            return out;
        }

        // Merge simultaneous physical and screen letters without dropping either or changing order.
        // Physical event order is already recorded. Snapshot devices retain their rising-edge order.
        int OrderedLetter(int physical, Keys virtualKey, const KeyboardState& keyboard)
        {
            unsigned letters = 0;
            for (int i = 0; i < 26; ++i)
                if (keyboard.IsKeyDown(static_cast<Keys>(int(Keys::A) + i))) letters |= 1u << i;
            const unsigned rising = letters & ~physicalLetters_;
            physicalLetters_ = letters;
            const bool virtualLetter = int(virtualKey) >= int(Keys::A) && int(virtualKey) <= int(Keys::Z);
            if (pendingLetters_.empty() && !virtualLetter) return physical;
            if (physical > 0) pendingLetters_.push_back(physical);
            else if (physical < 0)
                for (int i = 0; i < 26; ++i) if (rising & (1u << i)) pendingLetters_.push_back('a' + i);
            if (virtualLetter) pendingLetters_.push_back('a' + int(virtualKey) - int(Keys::A));
            const int letter = pendingLetters_.empty() ? 0 : pendingLetters_.front();
            if (!pendingLetters_.empty()) pendingLetters_.pop_front();
            return letter;
        }

        [[nodiscard]] KeyboardState FilterGameplay(const KeyboardState& keyboard) const
        {
            std::unordered_set<Keys> result;
            const Keys nav[] = {Keys::Up,Keys::Down,Keys::Left,Keys::Right,Keys::Enter,Keys::LeftControl,Keys::Space};
            for (const auto k : keyboard.GetPressedKeys())
            {
                bool suppress = false;
                for (unsigned i = 0; i < std::size(nav); ++i)
                    if (k == nav[i] && (visible_ || (suppressed_ & (1u << i)))) suppress = true;
                if (!suppress) result.insert(k);
            }
            return KeyboardState(result);
        }
        [[nodiscard]] std::string StateSignature() const
        {
            std::ostringstream out;
            for (const auto key : pendingKeys_) out << int(key) << ',';
            out << '|';
            out << physicalLetters_ << ':';
            for (const auto letter : pendingLetters_) out << letter << ',';
            out << '|';
            out << visible_ << ' ' << triggerTicks_ << ' ' << triggerConsumed_ << ' ' << previousKeys_ << ' ' << suppressed_;
            for (const auto& [id, hold] : pointers_) out << ' ' << id << ':' << hold.ticks << ':' << hold.blocked;
            out << '|' << focus_.StateSignature();
            return out.str();
        }

    private:
        struct Hold { int ticks = 0; bool blocked = false; };
        void Fire(int at, Output& out)
        {
            const auto code = keys_[at].code;
            if (code == Keys::None || code == Keys::F12) Close();
            if (code != Keys::None) pendingKeys_.push_back(code);
        }
        bool visible_ = false, triggerConsumed_ = false;
        int triggerTicks_ = 0;
        unsigned physicalLetters_ = 0;
        std::deque<int> pendingLetters_;
        std::deque<Keys> pendingKeys_;
        unsigned previousKeys_ = 0, suppressed_ = 0;
        std::map<int,Hold> pointers_;
        std::vector<Key> keys_;
        FocusNavigator focus_;
    };
}
