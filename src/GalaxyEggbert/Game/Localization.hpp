#pragma once

#include <cstddef>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <string_view>

// The game's languages (the owner's Q-023, 2026-11-11; German and Czech 2026-11-12). Mobile Eggbert
// shows its texts in the system's language, French or English (MyResource::Init: "fr" gives French,
// anything else English); Galaxy does the same with its four and adds a Language item to the Setup
// (BD-0126).
//
// The English text is the key, as gettext's: code keeps its English literal and shows
// Tr("Continue"); a literal stored in a table and shown later is marked TrMark("...") where it is
// written and passed through Tr() where it is shown. Translations are UTF-8 and drawn by text.png
// (TextMetrics::GlyphOf: the original's French letters and Galaxy's, tools/make_text_glyphs.py). VerifyLocalization checks that every marked literal in src/ has a translation in every
// language, that no translation is left over, and that every character of a translation has a glyph.
namespace GalaxyEggbert::Game
{
    enum class Language
    {
        English,
        French,
        German,
        Czech,
    };

    // In the order the Setup's Language button goes through them.
    inline constexpr Language kLanguages[] = {Language::English, Language::French, Language::German, Language::Czech};

    // The language texts are shown in; English until the host chooses (GalaxyEggbertGame's constructor).
    [[nodiscard]] Language CurrentLanguage() noexcept;
    void SetLanguage(Language language) noexcept;

    // "en", "fr", "de", "cs": the save file's `language=` and what ParseLanguage reads back.
    [[nodiscard]] std::string_view LanguageCode(Language language) noexcept;
    // The language's own name, as the Setup shows it ("English", "Français", "Deutsch", "Čeština").
    [[nodiscard]] std::string_view LanguageName(Language language) noexcept;
    // The language of a code or locale name: "fr", "fr-CA", "fr_FR.UTF-8", "FR" give French, "en..."
    // English, "de..." German, "cs..." Czech; anything else (a language the game has no texts in)
    // nothing.
    [[nodiscard]] std::optional<Language> ParseLanguage(std::string_view codeOrLocale) noexcept;

    // @p english in the current language: its translation, or itself in English or when it has none
    // (VerifyLocalization keeps the second from happening to a marked literal).
    [[nodiscard]] std::string_view Tr(std::string_view english) noexcept;

    // Marks a literal as text to translate where it is shown (with Tr), not where it is written: a
    // table of names, a list of help lines. It is the literal itself.
    [[nodiscard]] constexpr const char* TrMark(const char* english) noexcept { return english; }
    [[nodiscard]] constexpr std::string_view TrMark(std::string_view english) noexcept { return english; }

    // @p pattern with {0}, {1}, ... replaced by @p args (MyResource's own placeholders, "Player {0}");
    // a placeholder without an argument stays as written.
    [[nodiscard]] std::string Format(std::string_view pattern, std::initializer_list<std::string_view> args);
    // Format(Tr(english), args): a sentence with values in it, translated whole so each language can
    // put them where its word order wants them.
    [[nodiscard]] std::string TrFormat(std::string_view english, std::initializer_list<std::string_view> args);
    // TrFormat of @p one when @p count is 1, else of @p other: a number's noun in its form. A language
    // with more than one plural writes them in @p other's translation joined by '|': Czech the form
    // for 2-4, then the one for 0 and 5 and more ("{0} brány|{0} bran").
    [[nodiscard]] std::string TrCount(std::string_view one, std::string_view other, long long count,
                                      std::initializer_list<std::string_view> args);

    // One translated text: the English key and the language's text.
    struct Translation
    {
        std::string_view english;
        std::string_view text;
    };

    // Every translation of @p language (none for English); VerifyLocalization's view of the tables.
    [[nodiscard]] std::span<const Translation> Translations(Language language) noexcept;
}
