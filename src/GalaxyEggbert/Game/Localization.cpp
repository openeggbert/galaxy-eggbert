#include "Localization.hpp"

#include <array>
#include <atomic>
#include <cctype>
#include <unordered_map>

namespace GalaxyEggbert::Game
{
    // Translations<Code>.cpp, one a language but English.
    extern const Translation kFrenchTranslations[];
    extern const std::size_t kFrenchTranslationCount;
    extern const Translation kGermanTranslations[];
    extern const std::size_t kGermanTranslationCount;
    extern const Translation kCzechTranslations[];
    extern const std::size_t kCzechTranslationCount;
    extern const Translation kSlovakTranslations[];
    extern const std::size_t kSlovakTranslationCount;
    extern const Translation kSpanishTranslations[];
    extern const std::size_t kSpanishTranslationCount;
    extern const Translation kItalianTranslations[];
    extern const std::size_t kItalianTranslationCount;
    extern const Translation kPolishTranslations[];
    extern const std::size_t kPolishTranslationCount;
    extern const Translation kPortugueseTranslations[];
    extern const std::size_t kPortugueseTranslationCount;

    namespace
    {
        std::atomic<Language> currentLanguage{Language::English};

        using Table = std::unordered_map<std::string_view, std::string_view>;

        Table Build(std::span<const Translation> translations)
        {
            Table built;
            built.reserve(translations.size());
            for (const auto& translation : translations)
            {
                built.emplace(translation.english, translation.text);
            }
            return built;
        }

        // The language's table (English none), all built on the first use of one.
        const Table* TableOf(Language language)
        {
            static const std::array<Table, kLanguageCount> tables = []
            {
                std::array<Table, kLanguageCount> built;
                for (const Language each : kLanguages) built[LanguageIndex(each)] = Build(Translations(each));
                return built;
            }();
            return language == Language::English ? nullptr : &tables[LanguageIndex(language)];
        }

        struct LanguageInfo
        {
            std::string_view code;
            std::string_view name;
        };
        constexpr std::array<LanguageInfo, kLanguageCount> kInfo{{
            {"en", "English"},  {"fr", "Français"}, {"de", "Deutsch"},  {"cs", "Čeština"},   {"sk", "Slovenčina"},
            {"es", "Español"},  {"it", "Italiano"}, {"pl", "Polski"},   {"pt", "Português"},
        }};
    }

    Language CurrentLanguage() noexcept
    {
        return currentLanguage.load(std::memory_order_relaxed);
    }

    void SetLanguage(Language language) noexcept
    {
        currentLanguage.store(language, std::memory_order_relaxed);
    }

    std::string_view LanguageCode(Language language) noexcept
    {
        return kInfo[LanguageIndex(language)].code;
    }

    std::string_view LanguageName(Language language) noexcept
    {
        return kInfo[LanguageIndex(language)].name;
    }

    std::optional<Language> ParseLanguage(std::string_view codeOrLocale) noexcept
    {
        if (codeOrLocale.size() < 2 || (codeOrLocale.size() > 2 && std::isalpha(static_cast<unsigned char>(codeOrLocale[2]))))
        {
            return std::nullopt;
        }
        const char code[2] = {static_cast<char>(std::tolower(static_cast<unsigned char>(codeOrLocale[0]))),
                              static_cast<char>(std::tolower(static_cast<unsigned char>(codeOrLocale[1])))};
        for (const Language language : kLanguages)
        {
            if (kInfo[LanguageIndex(language)].code == std::string_view(code, 2))
            {
                return language;
            }
        }
        return std::nullopt;
    }

    std::string_view Tr(std::string_view english) noexcept
    {
        const Table* table = TableOf(CurrentLanguage());
        if (table == nullptr)
        {
            return english;
        }
        const auto found = table->find(english);
        return found == table->end() ? english : found->second;
    }

    std::string Format(std::string_view pattern, std::initializer_list<std::string_view> args)
    {
        std::string out;
        out.reserve(pattern.size() + 16);
        for (std::size_t i = 0; i < pattern.size(); ++i)
        {
            if (pattern[i] == '{' && i + 2 < pattern.size() && pattern[i + 2] == '}' && pattern[i + 1] >= '0' &&
                pattern[i + 1] <= '9')
            {
                const auto index = static_cast<std::size_t>(pattern[i + 1] - '0');
                if (index < args.size())
                {
                    out += *(args.begin() + index);
                    i += 2;
                    continue;
                }
            }
            out += pattern[i];
        }
        return out;
    }

    std::string TrFormat(std::string_view english, std::initializer_list<std::string_view> args)
    {
        return Format(Tr(english), args);
    }

    std::string TrCount(std::string_view one, std::string_view other, long long count,
                        std::initializer_list<std::string_view> args)
    {
        if (count == 1)
        {
            return TrFormat(one, args);
        }
        std::string_view forms = Tr(other);
        const auto bar = forms.find('|');
        if (bar != std::string_view::npos)
        {
            // The "few" form, then the "many" (the language's own rule; 0 is "many").
            const Language language = CurrentLanguage();
            const long long tens = count % 100, units = count % 10;
            const bool few = ((language == Language::Czech || language == Language::Slovak) && count >= 2 && count <= 4) ||
                             (language == Language::Polish && units >= 2 && units <= 4 && !(tens >= 12 && tens <= 14));
            forms = few ? forms.substr(0, bar) : forms.substr(bar + 1);
        }
        return Format(forms, args);
    }

    std::span<const Translation> Translations(Language language) noexcept
    {
        switch (language)
        {
            case Language::French: return {kFrenchTranslations, kFrenchTranslationCount};
            case Language::German: return {kGermanTranslations, kGermanTranslationCount};
            case Language::Czech: return {kCzechTranslations, kCzechTranslationCount};
            case Language::Slovak: return {kSlovakTranslations, kSlovakTranslationCount};
            case Language::Spanish: return {kSpanishTranslations, kSpanishTranslationCount};
            case Language::Italian: return {kItalianTranslations, kItalianTranslationCount};
            case Language::Polish: return {kPolishTranslations, kPolishTranslationCount};
            case Language::Portuguese: return {kPortugueseTranslations, kPortugueseTranslationCount};
            case Language::English: break;
        }
        return {};
    }
}
