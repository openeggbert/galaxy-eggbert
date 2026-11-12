#include "Localization.hpp"

#include <atomic>
#include <cctype>
#include <unordered_map>

namespace GalaxyEggbert::Game
{
    // TranslationsFr.cpp, TranslationsDe.cpp, TranslationsCs.cpp.
    extern const Translation kFrenchTranslations[];
    extern const std::size_t kFrenchTranslationCount;
    extern const Translation kGermanTranslations[];
    extern const std::size_t kGermanTranslationCount;
    extern const Translation kCzechTranslations[];
    extern const std::size_t kCzechTranslationCount;

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

        // The language's table, built on its first use.
        const Table* TableOf(Language language)
        {
            static const Table french = Build(Translations(Language::French));
            static const Table german = Build(Translations(Language::German));
            static const Table czech = Build(Translations(Language::Czech));
            switch (language)
            {
                case Language::French: return &french;
                case Language::German: return &german;
                case Language::Czech: return &czech;
                case Language::English: break;
            }
            return nullptr;
        }
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
        switch (language)
        {
            case Language::French: return "fr";
            case Language::German: return "de";
            case Language::Czech: return "cs";
            case Language::English: break;
        }
        return "en";
    }

    std::string_view LanguageName(Language language) noexcept
    {
        switch (language)
        {
            case Language::French: return "Français";
            case Language::German: return "Deutsch";
            case Language::Czech: return "Čeština";
            case Language::English: break;
        }
        return "English";
    }

    std::optional<Language> ParseLanguage(std::string_view codeOrLocale) noexcept
    {
        if (codeOrLocale.size() < 2 || (codeOrLocale.size() > 2 && std::isalpha(static_cast<unsigned char>(codeOrLocale[2]))))
        {
            return std::nullopt;
        }
        const char first = static_cast<char>(std::tolower(static_cast<unsigned char>(codeOrLocale[0])));
        const char second = static_cast<char>(std::tolower(static_cast<unsigned char>(codeOrLocale[1])));
        if (first == 'f' && second == 'r')
        {
            return Language::French;
        }
        if (first == 'e' && second == 'n')
        {
            return Language::English;
        }
        if (first == 'd' && second == 'e')
        {
            return Language::German;
        }
        if (first == 'c' && second == 's')
        {
            return Language::Czech;
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
            // Czech: 2-4 the first form, 0 and 5 and more the second.
            const bool few = CurrentLanguage() == Language::Czech && count >= 2 && count <= 4;
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
            case Language::English: break;
        }
        return {};
    }
}
