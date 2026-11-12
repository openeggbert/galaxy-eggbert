#pragma once

#include <GalaxyEggbert/Game/Localization.hpp>
#include <GalaxyEggbert/Game/QuadBatch.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace GalaxyEggbert::Game
{
    // A keyboard on the screen, for a name typed by touch, mouse or gamepad (2026-11-10, the
    // owner: a phone or a tablet had no way to name a galaxy). Five rows over the area it is laid
    // out in: the digits, three rows of letters (QWERTY, '-' at the end of the third), Shift and
    // Del around the last letters, then Cancel, Space and OK. Shift makes the next letter a
    // capital (on by itself at the start of an empty name); a letter typed turns it off. The
    // screen draws the keys (Keys(), Label()), hit-tests them (KeyAt()) and gives the arrows'
    // navigator their rects (Rects()); Press() edits the text.
    class ScreenKeyboard
    {
    public:
        enum class Kind : std::uint8_t { Char, Shift, Backspace, Space, Cancel, Done };
        struct Key
        {
            Kind kind = Kind::Char;
            char c = 0; // a Char's own (small) letter or digit
            QuadBatch::Rect rect{0.0f, 0.0f, 0.0f, 0.0f};
        };
        enum class Result : std::uint8_t { None, Edited, Done, Cancel };

        // @p address: an address's keys ('.' and ':' in place of the space bar), for a host's.
        void Layout(const QuadBatch::Rect& area, bool address = false)
        {
            keys_.clear();
            constexpr float kGap = 4.0f;
            const float rowH = (area.y1 - area.y0 - kGap * 4.0f) / 5.0f;
            const float unit = (area.x1 - area.x0 + kGap) / 10.0f; // ten units a row, gaps included
            const auto row = [&](int index, const std::vector<std::pair<Key, float>>& line) {
                const float y0 = area.y0 + static_cast<float>(index) * (rowH + kGap);
                float x = area.x0;
                for (auto [key, units] : line)
                {
                    key.rect = {x, y0, x + units * unit - kGap, y0 + rowH};
                    keys_.push_back(key);
                    x += units * unit;
                }
            };
            const auto letters = [](const char* text) {
                std::vector<std::pair<Key, float>> line;
                for (const char* p = text; *p != 0; ++p) line.push_back({Key{Kind::Char, *p, {}}, 1.0f});
                return line;
            };
            row(0, letters("1234567890"));
            row(1, letters("qwertyuiop"));
            row(2, letters("asdfghjkl-"));
            auto fourth = letters("zxcvbnm");
            fourth.insert(fourth.begin(), {Key{Kind::Shift, 0, {}}, 1.5f});
            fourth.push_back({Key{Kind::Backspace, 0, {}}, 1.5f});
            row(3, fourth);
            if (address)
                row(4, {{Key{Kind::Cancel, 0, {}}, 2.5f}, {Key{Kind::Char, '.', {}}, 2.5f}, {Key{Kind::Char, ':', {}}, 2.5f},
                        {Key{Kind::Done, 0, {}}, 2.5f}});
            else
                row(4, {{Key{Kind::Cancel, 0, {}}, 2.5f}, {Key{Kind::Space, ' ', {}}, 5.0f}, {Key{Kind::Done, 0, {}}, 2.5f}});
        }

        // Starts typing @p text: Shift on when it is empty (a name begins with a capital).
        void Begin(const std::string& text, bool capital = true) noexcept { shift_ = capital && text.empty(); }

        [[nodiscard]] const std::vector<Key>& Keys() const noexcept { return keys_; }
        [[nodiscard]] std::vector<QuadBatch::Rect> Rects() const
        {
            std::vector<QuadBatch::Rect> rects;
            for (const auto& key : keys_) rects.push_back(key.rect);
            return rects;
        }
        // The key under (x, y), -1 none.
        [[nodiscard]] int KeyAt(float x, float y) const noexcept
        {
            for (std::size_t i = 0; i < keys_.size(); ++i)
            {
                const auto& r = keys_[i].rect;
                if (x >= r.x0 && x < r.x1 && y >= r.y0 && y < r.y1) return static_cast<int>(i);
            }
            return -1;
        }
        // Whether the area holds (x, y): a click elsewhere is the screen's.
        [[nodiscard]] bool Holds(float x, float y) const noexcept
        {
            if (keys_.empty()) return false;
            const auto& first = keys_.front().rect;
            const auto& last = keys_.back().rect;
            return x >= first.x0 && x < last.x1 && y >= first.y0 && y < last.y1;
        }
        [[nodiscard]] bool Shifted() const noexcept { return shift_; }

        [[nodiscard]] std::string Label(const Key& key) const
        {
            switch (key.kind)
            {
            case Kind::Char:
                return std::string(1, shift_ && key.c >= 'a' && key.c <= 'z' ? static_cast<char>(key.c - 'a' + 'A') : key.c);
            case Kind::Shift: return std::string(Tr("Shift"));
            case Kind::Backspace: return std::string(Tr("Del"));
            case Kind::Space: return std::string(Tr("Space"));
            case Kind::Cancel: return std::string(Tr("Cancel"));
            case Kind::Done: return "OK";
            }
            return {};
        }

        // Key @p index pressed: @p text edited (at most @p maxLength characters), or the name done
        // or cancelled.
        Result Press(int index, std::string& text, std::size_t maxLength)
        {
            if (index < 0 || static_cast<std::size_t>(index) >= keys_.size()) return Result::None;
            const Key& key = keys_[static_cast<std::size_t>(index)];
            switch (key.kind)
            {
            case Kind::Shift:
                shift_ = !shift_;
                return Result::Edited;
            case Kind::Backspace:
                if (!text.empty()) text.pop_back();
                if (text.empty()) shift_ = true;
                return Result::Edited;
            case Kind::Cancel: return Result::Cancel;
            case Kind::Done: return Result::Done;
            case Kind::Space:
            case Kind::Char:
            {
                char c = key.c;
                if (shift_ && c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
                if (text.size() < maxLength) text += c;
                if (key.kind == Kind::Char && c != '-' && (c < '0' || c > '9')) shift_ = false;
                return Result::Edited;
            }
            }
            return Result::None;
        }

        // A key of the real keyboard typed into @p text: letters (@p shift: a capital), digits, the
        // number pad's, space and '-'; Backspace deletes, Enter is Done.
        // @p address: '.' and ':' too (an address's), no space.
        static Result TypeKey(Microsoft::Xna::Framework::Input::Keys key, bool shift, std::string& text,
                              std::size_t maxLength, bool address = false)
        {
            using Microsoft::Xna::Framework::Input::Keys;
            const int k = static_cast<int>(key);
            if (key == Keys::Enter) return Result::Done;
            if (key == Keys::Back)
            {
                if (!text.empty()) text.pop_back();
                return Result::Edited;
            }
            char c = 0;
            if (k >= static_cast<int>(Keys::A) && k <= static_cast<int>(Keys::Z))
                c = static_cast<char>((shift ? 'A' : 'a') + (k - static_cast<int>(Keys::A)));
            else if (k >= static_cast<int>(Keys::D0) && k <= static_cast<int>(Keys::D9))
                c = static_cast<char>('0' + (k - static_cast<int>(Keys::D0)));
            else if (k >= static_cast<int>(Keys::NumPad0) && k <= static_cast<int>(Keys::NumPad9))
                c = static_cast<char>('0' + (k - static_cast<int>(Keys::NumPad0)));
            else if (key == Keys::Space && !address)
                c = ' ';
            else if (address && (key == Keys::OemPeriod || key == Keys::Decimal))
                c = '.';
            else if (address && key == Keys::OemSemicolon && shift)
                c = ':';
            else if (key == Keys::OemMinus || key == Keys::Subtract)
                c = '-';
            if (c == 0) return Result::None;
            if (text.size() < maxLength) text += c;
            return Result::Edited;
        }

    private:
        std::vector<Key> keys_;
        bool shift_ = false;
    };
}
