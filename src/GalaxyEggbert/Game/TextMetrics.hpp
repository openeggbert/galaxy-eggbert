#pragma once

#include <cstdint>
#include <string_view>

namespace GalaxyEggbert::Game::TextMetrics
{
    // text.png's proportional font, as Mobile Eggbert lays it out
    // (Text::DrawChar/GetCharWidth): a glyph is drawn at the pen position and
    // the pen then advances by its width plus one pixel. Every text layout
    // used a fixed 17-pixel advance until 2026-10-25, which opened gaps after
    // narrow letters ("Pl ayer", "Mai n gates", user report).
    //
    // kWidth is mobile-eggbert's Text::table_width, transcribed with the
    // owner's approval (2026-10-25; mobile-eggbert is MIT, Daniel Roux and
    // Robert Vokac): pixels at scale 1 per glyph index. For printable ASCII
    // the glyph index is the character code (its table_char is the identity
    // there); every other byte draws glyph 1, as GetOffset() maps it.
    inline constexpr std::uint8_t kWidth[128] = {
        32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
        32, 32, 32, 32, 14, 13, 15, 15, 15, 15,
        15, 15, 11, 12, 14, 13, 13, 15, 15, 14,
        0, 0, 9, 5, 10, 25, 17, 22, 18, 5,
        9, 9, 14, 13, 6, 10, 6, 14, 18, 10,
        14, 14, 17, 16, 16, 17, 15, 16, 5, 6,
        9, 12, 10, 14, 25, 18, 16, 17, 18, 16,
        16, 19, 20, 15, 15, 15, 14, 24, 22, 22,
        15, 24, 16, 18, 15, 19, 18, 29, 20, 18,
        19, 8, 13, 8, 12, 20, 7, 15, 15, 13,
        15, 15, 13, 14, 14, 6, 8, 14, 6, 21,
        16, 14, 14, 13, 12, 14, 11, 13, 13, 19,
        16, 15, 14, 10, 5, 11, 16, 0,
    };

    // The glyph text.png draws for character c.
    [[nodiscard]] inline constexpr int GlyphIndex(char c) noexcept
    {
        const auto code = static_cast<unsigned char>(c);
        return code < 128 ? code : 1;
    }

    // Pen advance after character c, in reference pixels at scale 1.
    [[nodiscard]] inline constexpr float Advance(char c) noexcept
    {
        return static_cast<float>(kWidth[GlyphIndex(c)] + 1);
    }

    // Width of a whole string at scale 1 (Text::GetTextWidth).
    [[nodiscard]] inline constexpr float Width(std::string_view text) noexcept
    {
        float width = 0.0f;
        for (const char c : text)
        {
            width += Advance(c);
        }
        return width;
    }
}
