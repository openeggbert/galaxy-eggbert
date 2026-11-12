#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
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
    // there).
    // Glyphs 128-170 are Galaxy's (tools/make_text_glyphs.py, owner Q-023 2026-11-12): the letters
    // Czech, German and French need that the sheet had not, kExtraGlyphs below; their widths are
    // their base letters' (ď's a little more: its caron beside the stem).
    inline constexpr std::uint8_t kWidth[256] = {
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
        // 128-170 (á č ď ě í ň ó ř š ť ú ů ý ž, Á Č Ď É Ě Í Ň Ó Ř Š Ť Ú Ů Ý Ž, Ä Ö Ü,
        // À Â Ç È Ê Ë Î Ï Ô Ù Û)
        15, 13, 19, 15, 6, 16, 14, 12, 14, 11, 13, 13, 15, 14,
        18, 17, 18, 16, 16, 15, 22, 22, 16, 18, 15, 19, 19, 18, 19,
        18, 22, 19,
        18, 18, 17, 16, 16, 16, 15, 15, 22, 19, 19,
    };

    // The glyph the original draws for a character it has no picture of (Text::GetOffset's
    // `return 1`).
    inline constexpr int kUnknownGlyph = 1;

    // Text is UTF-8 since the French translation (2026-11-11, owner Q-023). Bytes below 0x80 are
    // their own glyph as before, the pad pictograms 0-13 included.
    //
    // text.png's precomposed accented letters, glyphs 15-29, in the order of the original's
    // Text::table_accents (ü à â é è ë ê ï î ô ù û ä ö ç), which its table_char also gives for
    // those Latin-1 codes (252 -> 15, 224 -> 16, ... 231 -> 29). The original's GetOffset returns
    // the glyph instead of the code for them and so looks up record 15+i, which draws glyph 1: an
    // evident defect, not reproduced; the glyphs are drawn as both tables mean.
    inline constexpr char32_t kAccented[15] = {
        U'ü', U'à', U'â', U'é', U'è', U'ë', U'ê', U'ï',
        U'î', U'ô', U'ù', U'û', U'ä', U'ö', U'ç',
    };
    inline constexpr int kFirstAccentedGlyph = 15;

    // Galaxy's letters, glyphs 128 and on, in tools/make_text_glyphs.py's order (GLYPHS there).
    inline constexpr char32_t kExtraGlyphs[] = {
        U'á', U'č', U'ď', U'ě', U'í', U'ň', U'ó', U'ř', U'š', U'ť', U'ú', U'ů', U'ý', U'ž',
        U'Á', U'Č', U'Ď', U'É', U'Ě', U'Í', U'Ň', U'Ó', U'Ř', U'Š', U'Ť', U'Ú', U'Ů', U'Ý', U'Ž',
        U'Ä', U'Ö', U'Ü',
        U'À', U'Â', U'Ç', U'È', U'Ê', U'Ë', U'Î', U'Ï', U'Ô', U'Ù', U'Û',
    };
    inline constexpr int kFirstExtraGlyph = 128;

    // The glyph text.png draws for code point c: ASCII and the 15 accented letters as themselves; a
    // capital or a letter the sheet has no accented picture of as its plain letter (French drops
    // the accent on a capital, which the sheet does not have); typographic spaces, quotes and
    // dashes as their ASCII forms; anything else kUnknownGlyph.
    [[nodiscard]] inline constexpr int GlyphOf(char32_t c) noexcept
    {
        if (c < 0x80)
        {
            return static_cast<int>(c);
        }
        for (int i = 0; i < 15; ++i)
        {
            if (kAccented[i] == c)
            {
                return kFirstAccentedGlyph + i;
            }
        }
        for (int i = 0; i < static_cast<int>(std::size(kExtraGlyphs)); ++i)
        {
            if (kExtraGlyphs[i] == c)
            {
                return kFirstExtraGlyph + i;
            }
        }
        switch (c)
        {
            case U'À': case U'Á': case U'Â': case U'Ã': case U'Ä': case U'Å': return 'A';
            case U'Ç': return 'C';
            case U'È': case U'É': case U'Ê': case U'Ë': return 'E';
            case U'Ì': case U'Í': case U'Î': case U'Ï': return 'I';
            case U'Ñ': return 'N';
            case U'Ò': case U'Ó': case U'Ô': case U'Õ': case U'Ö': return 'O';
            case U'Ù': case U'Ú': case U'Û': case U'Ü': return 'U';
            case U'Ÿ': case U'Ý': return 'Y';
            case U'á': case U'ã': case U'å': return 'a';
            case U'ì': case U'í': return 'i';
            case U'ñ': return 'n';
            case U'ò': case U'ó': case U'õ': return 'o';
            case U'ú': return 'u';
            case U'ý': case U'ÿ': return 'y';
            case U' ': case U' ': case U' ': return ' ';
            case U'‘': case U'’': return '\'';
            case U'“': case U'”': case U'«': case U'»': return '"';
            case U'–': case U'—': return '-';
            default: return kUnknownGlyph;
        }
    }

    // Decodes the UTF-8 character at text[at], moves `at` past it and returns its code point. A byte
    // that does not start a valid sequence is taken alone and decodes to U+FFFD, so text from
    // anywhere (a peer's name, a file) never stops the layout or reads past its end.
    [[nodiscard]] inline constexpr char32_t NextCodePoint(std::string_view text, std::size_t& at) noexcept
    {
        const auto lead = static_cast<unsigned char>(text[at]);
        if (lead < 0x80)
        {
            ++at;
            return lead;
        }
        const int extra = lead >= 0xF0 && lead < 0xF5 ? 3 : lead >= 0xE0 && lead < 0xF0 ? 2 : lead >= 0xC2 && lead < 0xE0 ? 1 : -1;
        if (extra < 0 || at + static_cast<std::size_t>(extra) >= text.size())
        {
            ++at;
            return U'�';
        }
        char32_t code = lead & (0x3F >> extra);
        for (int k = 1; k <= extra; ++k)
        {
            const auto next = static_cast<unsigned char>(text[at + static_cast<std::size_t>(k)]);
            if ((next & 0xC0) != 0x80)
            {
                ++at;
                return U'�';
            }
            code = (code << 6) | (next & 0x3F);
        }
        const char32_t smallest = extra == 1 ? 0x80 : extra == 2 ? 0x800 : 0x10000;
        if (code < smallest || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF))
        {
            ++at;
            return U'�';
        }
        at += static_cast<std::size_t>(extra) + 1;
        return code;
    }

    // The glyph of the character at text[at]; moves `at` past it. Every layout walks its text so:
    //   for (std::size_t at = 0; at < text.size();) { const int glyph = NextGlyph(text, at); ... }
    [[nodiscard]] inline constexpr int NextGlyph(std::string_view text, std::size_t& at) noexcept
    {
        return GlyphOf(NextCodePoint(text, at));
    }

    // Pen advance after a glyph, in reference pixels at scale 1.
    [[nodiscard]] inline constexpr float GlyphAdvance(int glyph) noexcept
    {
        return static_cast<float>(kWidth[glyph] + 1);
    }

    // Width of a whole string at scale 1 (Text::GetTextWidth).
    [[nodiscard]] inline constexpr float Width(std::string_view text) noexcept
    {
        float width = 0.0f;
        for (std::size_t at = 0; at < text.size();)
        {
            width += GlyphAdvance(NextGlyph(text, at));
        }
        return width;
    }

    // Removes the last character, the whole of a UTF-8 sequence (a layout shortening a text to fit
    // must not cut one in two).
    inline void PopBack(std::string& text) noexcept
    {
        if (text.empty())
        {
            return;
        }
        std::size_t start = text.size() - 1;
        while (start > 0 && text.size() - start < 4 && (static_cast<unsigned char>(text[start]) & 0xC0) == 0x80)
        {
            --start;
        }
        std::size_t probe = start;
        (void)NextCodePoint(text, probe);
        text.erase(probe == text.size() ? start : text.size() - 1);
    }
}
