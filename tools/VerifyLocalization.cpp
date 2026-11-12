// VerifyLocalization: the game's texts in every language (Localization.hpp, the owner's Q-023).
//
//   VerifyLocalization <source dir>
//
// 1. TextMetrics reads UTF-8 as text.png can draw it: ASCII and the pad pictograms as themselves, the
//    original's 15 accented letters as glyphs 15-29, capitals without their accent, a broken sequence
//    as one unknown glyph without reading past the text's end; PopBack never cuts a character in two.
// 2. Format/TrFormat place their values; ParseLanguage reads codes and locale names.
// 3. Every literal of src/ and include/ given to Tr, TrMark or TrFormat has a translation in every
//    language, no translation is left without its literal, a translation keeps the English's {0},
//    {1}... placeholders, and each of its characters has a glyph of its own (the training hints' own
//    French table too).
#include <GalaxyEggbert/Game/Localization.hpp>
#include <GalaxyEggbert/Game/TextMetrics.hpp>
#include <GalaxyEggbert/Game/TrainingHints.hpp>

#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    using namespace GalaxyEggbert::Game;

    int failures = 0;

    void Check(bool ok, const std::string& what)
    {
        if (!ok)
        {
            ++failures;
            std::cout << "FAIL: " << what << "\n";
        }
    }

    std::vector<int> Glyphs(std::string_view text)
    {
        std::vector<int> glyphs;
        for (std::size_t at = 0; at < text.size();) glyphs.push_back(TextMetrics::NextGlyph(text, at));
        return glyphs;
    }

    void CheckTextMetrics()
    {
        Check(Glyphs("Ab 9") == std::vector<int>{'A', 'b', ' ', '9'}, "ASCII is its own glyph");
        Check(Glyphs(std::string_view("\000\010\013", 3)) == std::vector<int>{0, 8, 11}, "the pad pictograms are their own glyphs");
        Check(Glyphs("üàâéèëêïîôùûäöç") ==
                  std::vector<int>{15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29},
              "the 15 accented letters are glyphs 15-29 (Text::table_char)");
        Check(Glyphs("áčďěíňóřšťúůýž") == std::vector<int>{128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141},
              "Czech's letters are Galaxy's glyphs 128-141 (tools/make_text_glyphs.py)");
        Check(Glyphs("ÉÀÇÎÄŽ") == std::vector<int>{145, 160, 162, 166, 157, 156}, "a capital keeps its accent (Galaxy's glyphs)");
        Check(Glyphs("Ÿ") == std::vector<int>{'Y'}, "a capital the sheet has no picture of is its plain letter");
        Check(Glyphs("Аарой") == std::vector<int>{'A', 'a', 'p', 'o', TextMetrics::GlyphOf(U'й')} &&
                  TextMetrics::GlyphOf(U'й') >= TextMetrics::kFirstExtraGlyph,
              "a Cyrillic letter like a Latin one is its glyph, the others Galaxy's (Comic Relief)");
        Check(Glyphs("Αλό") == std::vector<int>{'A', TextMetrics::GlyphOf(U'λ'), TextMetrics::GlyphOf(U'ó')} &&
                  TextMetrics::GlyphOf(U'λ') >= TextMetrics::kFirstExtraGlyph &&
                  TextMetrics::GlyphOf(U'ş') >= TextMetrics::kFirstExtraGlyph &&
                  TextMetrics::GlyphOf(U'ø') >= TextMetrics::kFirstExtraGlyph &&
                  TextMetrics::GlyphOf(U'ı') != TextMetrics::GlyphOf(U'i'),
              "Greek like Latin (ό as ó), Greek's, Turkish's and Danish's own letters Galaxy's");
        bool widths = true;
        for (int glyph = TextMetrics::kFirstExtraGlyph;
             glyph < TextMetrics::kFirstExtraGlyph + static_cast<int>(std::size(TextMetrics::kExtraGlyphs)); ++glyph)
            widths = widths && TextMetrics::kWidth[glyph] > 0;
        Check(widths, "every glyph of Galaxy's has a width");
        Check(Glyphs("’«» –") == std::vector<int>{'\'', '"', '"', ' ', '-'},
              "typographic quotes, spaces and dashes are their ASCII forms");
        Check(Glyphs("中") == std::vector<int>{TextMetrics::kUnknownGlyph}, "a character the sheet lacks is glyph 1");
        Check(Glyphs("a\xC3") == std::vector<int>{'a', TextMetrics::kUnknownGlyph}, "a sequence cut at the end is one glyph 1");
        Check(Glyphs("\xC3(") == std::vector<int>{TextMetrics::kUnknownGlyph, '('}, "a bad continuation byte is not swallowed");
        Check(Glyphs("\xC0\xAF") == std::vector<int>{TextMetrics::kUnknownGlyph, TextMetrics::kUnknownGlyph},
              "an overlong sequence is not a character");
        Check(Glyphs("\xE2\x82") == std::vector<int>{TextMetrics::kUnknownGlyph, TextMetrics::kUnknownGlyph},
              "a three-byte sequence cut short reads no further");
        Check(TextMetrics::Width("é") == TextMetrics::GlyphAdvance(18), "an accented letter is one advance wide");
        std::string text = "Français";
        TextMetrics::PopBack(text);
        TextMetrics::PopBack(text);
        TextMetrics::PopBack(text);
        Check(text == "Franç", "PopBack removes whole characters (ASCII)");
        TextMetrics::PopBack(text);
        Check(text == "Fran", "PopBack removes a two-byte character whole");
        std::string broken = "ab\xA9";
        TextMetrics::PopBack(broken);
        Check(broken == "ab", "PopBack removes a stray continuation byte alone");
    }

    void CheckFormatAndParse()
    {
        Check(Format("{0} has {1} lives", {"Ann", "3"}) == "Ann has 3 lives", "Format places the values");
        Check(Format("{1}, {0}", {"a", "b"}) == "b, a", "Format follows the pattern's order");
        Check(Format("{0} {3}", {"x"}) == "x {3}", "a placeholder without a value stays");
        Check(ParseLanguage("fr") == Language::French && ParseLanguage("fr-CA") == Language::French &&
                  ParseLanguage("fr_FR.UTF-8") == Language::French && ParseLanguage("FR") == Language::French,
              "French codes and locale names");
        Check(ParseLanguage("de") == Language::German && ParseLanguage("de_AT.UTF-8") == Language::German &&
                  ParseLanguage("cs-CZ") == Language::Czech && ParseLanguage("cs") == Language::Czech,
              "German and Czech codes and locale names");
        Check(ParseLanguage("sk-SK") == Language::Slovak && ParseLanguage("es_MX") == Language::Spanish &&
                  ParseLanguage("it") == Language::Italian && ParseLanguage("pl-PL") == Language::Polish &&
                  ParseLanguage("pt-BR") == Language::Portuguese && ParseLanguage("pt_PT.UTF-8") == Language::Portuguese,
              "the other languages' codes and locale names");
        for (const Language language : kLanguages)
            Check(ParseLanguage(LanguageCode(language)) == language && kLanguages[LanguageIndex(language)] == language,
                  "a language's code reads back as it, its index is its place");
        Check(ParseLanguage("fi-FI") == Language::Finnish && ParseLanguage("da_DK.UTF-8") == Language::Danish &&
                  ParseLanguage("nb-NO") == Language::Norwegian && ParseLanguage("no") == Language::Norwegian &&
                  ParseLanguage("nn_NO") == Language::Norwegian && ParseLanguage("tr-TR") == Language::Turkish &&
                  ParseLanguage("el-GR") == Language::Greek,
              "Finnish, Danish, Norwegian (nb, no and nn), Turkish and Greek");
        Check(ParseLanguage("en-US") == Language::English && !ParseLanguage("ja-JP") && !ParseLanguage("fra") &&
                  !ParseLanguage("") && !ParseLanguage("C"),
              "English, and no language for the others");
        SetLanguage(Language::English);
        Check(Tr("Continue") == "Continue", "English is the key itself");
        SetLanguage(Language::French);
        Check(Tr("Continue") == "Continuer" && TrFormat("Player {0}", {"B"}) == "Joueur B",
              "French from the table (MyResource's TX_BUTTON_CONTINUE, TX_GAMER_TITLE)");
        Check(Tr("A text no one translated") == "A text no one translated", "a missing translation is the English");
        // A number's noun: English and French one or other, Czech's two plurals (2-4, 0 and 5+).
        SetLanguage(Language::English);
        Check(TrCount("{0} place", "{0} places", 1, {"1"}) == "1 place" &&
                  TrCount("{0} place", "{0} places", 0, {"0"}) == "0 places",
              "TrCount: English one and other");
        SetLanguage(Language::Czech);
        Check(TrCount("{0} place", "{0} places", 1, {"1"}) == "1 místo" &&
                  TrCount("{0} place", "{0} places", 3, {"3"}) == "3 místa" &&
                  TrCount("{0} place", "{0} places", 5, {"5"}) == "5 míst" &&
                  TrCount("{0} place", "{0} places", 0, {"0"}) == "0 míst",
              "TrCount: Czech 1, 2-4, 0 and 5+");
        SetLanguage(Language::German);
        Check(TrCount("{0} place", "{0} places", 4, {"4"}) == "4 Orte" && Tr("Continue") == "Weiter", "German");
        SetLanguage(Language::Slovak);
        Check(TrCount("{0} place", "{0} places", 3, {"3"}) == "3 miesta" &&
                  TrCount("{0} place", "{0} places", 7, {"7"}) == "7 miest",
              "TrCount: Slovak 2-4 and 5+");
        SetLanguage(Language::Polish);
        Check(TrCount("{0} place", "{0} places", 2, {"2"}) == "2 miejsca" &&
                  TrCount("{0} place", "{0} places", 5, {"5"}) == "5 miejsc" &&
                  TrCount("{0} place", "{0} places", 12, {"12"}) == "12 miejsc" &&
                  TrCount("{0} place", "{0} places", 22, {"22"}) == "22 miejsca",
              "TrCount: Polish 2-4 and 22-24 but not 12-14");
        SetLanguage(Language::Romanian);
        Check(TrCount("{0} place", "{0} places", 3, {"3"}) == "3 locuri" &&
                  TrCount("{0} place", "{0} places", 20, {"20"}) == "20 de locuri" &&
                  TrCount("{0} place", "{0} places", 119, {"119"}) == "119 locuri",
              "TrCount: Romanian 2-19 and 20 with \"de\"");
        SetLanguage(Language::Russian);
        Check(TrCount("{0} place", "{0} places", 3, {"3"}) == "3 места" &&
                  TrCount("{0} place", "{0} places", 5, {"5"}) == "5 мест" &&
                  TrCount("{0} place", "{0} places", 11, {"11"}) == "11 мест" &&
                  TrCount("{0} place", "{0} places", 21, {"21"}) == "21 место",
              "TrCount: Russian 2-4, 5+, 11, 21");
        SetLanguage(Language::Spanish);
        Check(TrCount("{0} place", "{0} places", 2, {"2"}) == "2 lugares" && Tr("Language") == "Idioma", "Spanish");
        SetLanguage(Language::Finnish);
        Check(TrCount("{0} place", "{0} places", 1, {"1"}) == "1 paikka" &&
                  TrCount("{0} place", "{0} places", 3, {"3"}) == "3 paikkaa",
              "TrCount: Finnish one and other");
        SetLanguage(Language::Greek);
        Check(Tr("Language") == "Γλώσσα" && TrFormat("Player {0}", {"A"}) == "Παίκτης A", "Greek");
        SetLanguage(Language::Turkish);
        Check(Tr("Language") == "Dil" && TrCount("{0} demo", "{0} demos", 2, {"2"}) == "2 demo", "Turkish");
        SetLanguage(Language::English);
    }

    // The C++ string literal at text[at] (at its opening quote), decoded; at moves past it and its
    // suffix (sv).
    std::string ReadLiteral(const std::string& text, std::size_t& at)
    {
        std::string out;
        ++at; // the quote
        while (at < text.size() && text[at] != '"')
        {
            char c = text[at++];
            if (c != '\\')
            {
                out += c;
                continue;
            }
            c = text[at++];
            switch (c)
            {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case 'v': out += '\v'; break;
                case 'f': out += '\f'; break;
                case 'b': out += '\b'; break;
                case 'a': out += '\a'; break;
                case 'x':
                {
                    int value = 0;
                    while (at < text.size() && std::isxdigit(static_cast<unsigned char>(text[at])))
                        value = value * 16 + std::stoi(std::string(1, text[at++]), nullptr, 16);
                    out += static_cast<char>(value);
                    break;
                }
                default:
                    if (c >= '0' && c <= '7')
                    {
                        int value = c - '0';
                        for (int k = 0; k < 2 && at < text.size() && text[at] >= '0' && text[at] <= '7'; ++k)
                            value = value * 8 + (text[at++] - '0');
                        out += static_cast<char>(value);
                    }
                    else out += c; // \" \\ \' \?
            }
        }
        ++at; // the closing quote
        if (text.compare(at, 2, "sv") == 0) at += 2;
        return out;
    }

    // @p source without its comments, its strings and characters kept.
    std::string StripComments(const std::string& source)
    {
        std::string out;
        out.reserve(source.size());
        for (std::size_t i = 0; i < source.size();)
        {
            if (source.compare(i, 2, "//") == 0)
            {
                while (i < source.size() && source[i] != '\n') ++i;
                continue;
            }
            if (source.compare(i, 2, "/*") == 0)
            {
                const auto end = source.find("*/", i + 2);
                i = end == std::string::npos ? source.size() : end + 2;
                out += ' ';
                continue;
            }
            if (source[i] == '"' || source[i] == '\'')
            {
                const char quote = source[i];
                out += source[i++];
                while (i < source.size() && source[i] != quote)
                {
                    if (source[i] == '\\') out += source[i++];
                    if (i < source.size()) out += source[i++];
                }
                if (i < source.size()) out += source[i++];
                continue;
            }
            out += source[i++];
        }
        return out;
    }

    bool IsIdentifier(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

    // The keys of every Tr/TrMark/TrFormat call in @p code: the literals of its first argument (those
    // written next to each other joined, as the compiler joins them; a ?: gives two).
    void CollectKeys(const std::string& code, const std::string& where, std::map<std::string, std::string>& keys)
    {
        for (std::size_t i = 0; i < code.size(); ++i)
        {
            std::size_t length = 0;
            for (const std::string_view name : {std::string_view("TrFormat"), std::string_view("TrMark"),
                                                std::string_view("TrCount"), std::string_view("Tr")})
                if (code.compare(i, name.size(), name) == 0)
                {
                    length = name.size();
                    break;
                }
            if (length == 0 || (i > 0 && IsIdentifier(code[i - 1])) || IsIdentifier(code[i + length])) continue;
            // TrCount's keys are its first two arguments (the one and the other form).
            int arguments = length == 7 && code.compare(i, 7, "TrCount") == 0 ? 2 : 1;
            std::size_t at = i + length;
            while (at < code.size() && std::isspace(static_cast<unsigned char>(code[at]))) ++at;
            if (at >= code.size() || code[at] != '(') continue;
            ++at;
            int depth = 0;
            std::string current;
            bool joining = false;
            while (at < code.size())
            {
                const char c = code[at];
                if (c == '"')
                {
                    // Only the argument's own literals: one inside a call within it is that call's.
                    const std::string literal = ReadLiteral(code, at);
                    if (depth == 0)
                    {
                        current += literal;
                        joining = true;
                    }
                    continue;
                }
                if (c == '\'')
                {
                    ++at;
                    while (at < code.size() && code[at] != '\'') at += code[at] == '\\' ? 2 : 1;
                    ++at;
                    continue;
                }
                if (std::isspace(static_cast<unsigned char>(c)))
                {
                    ++at;
                    continue;
                }
                if (joining)
                {
                    keys.emplace(current, where);
                    current.clear();
                    joining = false;
                }
                if (c == '(' || c == '[' || c == '{') ++depth;
                else if (c == ')' || c == ']' || c == '}')
                {
                    if (depth == 0) break;
                    --depth;
                }
                else if (c == ',' && depth == 0)
                {
                    if (--arguments == 0) break;
                }
                ++at;
            }
            if (joining) keys.emplace(current, where);
        }
    }

    std::set<std::string> Placeholders(std::string_view text)
    {
        std::set<std::string> found;
        for (std::size_t i = 0; i + 2 < text.size(); ++i)
            if (text[i] == '{' && text[i + 2] == '}' && text[i + 1] >= '0' && text[i + 1] <= '9')
                found.insert(std::string(text.substr(i, 3)));
        return found;
    }

    std::string Shown(std::string_view text)
    {
        std::string out;
        for (const char c : text)
        {
            if (static_cast<unsigned char>(c) < 0x20)
            {
                char escaped[8];
                std::snprintf(escaped, sizeof escaped, "\\%03o", static_cast<unsigned char>(c));
                out += escaped;
            }
            else out += c;
        }
        return out;
    }

    // Every character of @p text drawn by a glyph of its own (pictograms 0-13 allowed in a hint).
    void CheckDrawable(std::string_view text, const std::string& what)
    {
        for (std::size_t at = 0; at < text.size();)
        {
            const std::size_t start = at;
            const char32_t code = TextMetrics::NextCodePoint(text, at);
            if (TextMetrics::GlyphOf(code) == TextMetrics::kUnknownGlyph && code != 1)
            {
                Check(false, what + ": a character text.png cannot draw at byte " + std::to_string(start) + " in \"" +
                                 Shown(text) + "\"");
                return;
            }
        }
    }

    void CheckTranslations(const std::filesystem::path& sourceDir)
    {
        std::map<std::string, std::string> keys; // key -> the first file it is in
        std::size_t files = 0;
        for (const auto* root : {"src", "include"})
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceDir / root))
            {
                const auto extension = entry.path().extension();
                if (!entry.is_regular_file() || (extension != ".cpp" && extension != ".hpp")) continue;
                if (entry.path().filename().string().rfind("Translations", 0) == 0) continue; // the tables
                std::ifstream in(entry.path(), std::ios::binary);
                std::stringstream buffer;
                buffer << in.rdbuf();
                CollectKeys(StripComments(buffer.str()), entry.path().lexically_relative(sourceDir).generic_string(), keys);
                ++files;
            }
        }
        Check(files > 50 && keys.size() > 300, "the sources were found and read (" + std::to_string(files) + " files, " +
                                                    std::to_string(keys.size()) + " texts)");
        for (const Language language : kLanguages)
        {
            const auto table = Translations(language);
            if (language == Language::English)
            {
                Check(table.empty(), "English has no table: its texts are the keys");
                continue;
            }
            const std::string name(LanguageCode(language));
            std::map<std::string_view, std::string_view> byKey;
            for (const auto& translation : table)
            {
                Check(byKey.emplace(translation.english, translation.text).second,
                      name + ": translated twice: \"" + Shown(translation.english) + "\"");
            }
            for (const auto& [key, where] : keys)
            {
                if (key.empty()) continue;
                const auto found = byKey.find(key);
                Check(found != byKey.end(), name + ": no translation of \"" + Shown(key) + "\" (" + where + ")");
            }
            for (const auto& [key, text] : byKey)
            {
                Check(keys.count(std::string(key)) != 0, name + ": a translation no code asks for: \"" + Shown(key) + "\"");
                // A plural's forms each keep the key's placeholders (TrCount: "few|many").
                for (std::size_t from = 0;;)
                {
                    const std::size_t bar = text.find('|', from);
                    const std::string_view form = text.substr(from, bar == std::string_view::npos ? text.npos : bar - from);
                    Check(Placeholders(key) == Placeholders(form),
                          name + ": the placeholders differ: \"" + Shown(key) + "\" / \"" + Shown(form) + "\"");
                    if (bar == std::string_view::npos) break;
                    from = bar + 1;
                }
                Check(!text.empty(), name + ": an empty translation of \"" + Shown(key) + "\"");
                // A space the key starts or ends with (a piece joined to others: "{0} kill, ") every form
                // keeps too.
                for (std::size_t from = 0;;)
                {
                    const std::size_t bar = text.find('|', from);
                    const std::string_view form = text.substr(from, bar == std::string_view::npos ? text.npos : bar - from);
                    Check(form.empty() || key.empty() || (key.back() == ' ') == (form.back() == ' '),
                          name + ": the space at the end differs: \"" + Shown(key) + "\" / \"" + Shown(form) + "\"");
                    if (bar == std::string_view::npos) break;
                    from = bar + 1;
                }
                CheckDrawable(text, name);
            }
            for (const std::string_view hint : AllTrainingHintTexts(language)) CheckDrawable(hint, name + " training hint");
            std::cout << name << ": " << byKey.size() << " translations, " << keys.size() << " texts asked for\n";
        }
    }
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "usage: VerifyLocalization <source dir>\n";
        return 2;
    }
    CheckTextMetrics();
    CheckFormatAndParse();
    CheckTranslations(argv[1]);
    std::cout << (failures == 0 ? "VerifyLocalization: all passed" : "VerifyLocalization: FAILED") << " (" << failures
              << " failures)\n";
    return failures == 0 ? 0 : 1;
}
