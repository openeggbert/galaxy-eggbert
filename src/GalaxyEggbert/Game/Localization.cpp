#include "Localization.hpp"

#include <atomic>
#include <cctype>
#include <unordered_map>

namespace GalaxyEggbert::Game
{
    // TranslationsFr.cpp.
    extern const Translation kFrenchTranslations[];
    extern const std::size_t kFrenchTranslationCount;

    namespace
    {
        std::atomic<Language> currentLanguage{Language::English};

        using Table = std::unordered_map<std::string_view, std::string_view>;

        const Table& FrenchTable()
        {
            static const Table table = []
            {
                Table built;
                built.reserve(kFrenchTranslationCount);
                for (std::size_t i = 0; i < kFrenchTranslationCount; ++i)
                {
                    built.emplace(kFrenchTranslations[i].english, kFrenchTranslations[i].text);
                }
                return built;
            }();
            return table;
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
        return language == Language::French ? "fr" : "en";
    }

    std::string_view LanguageName(Language language) noexcept
    {
        return language == Language::French ? "Français" : "English";
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
        return std::nullopt;
    }

    std::string_view Tr(std::string_view english) noexcept
    {
        if (CurrentLanguage() != Language::French)
        {
            return english;
        }
        const Table& table = FrenchTable();
        const auto found = table.find(english);
        return found == table.end() ? english : found->second;
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

    std::span<const Translation> Translations(Language language) noexcept
    {
        if (language == Language::French)
        {
            return {kFrenchTranslations, kFrenchTranslationCount};
        }
        return {};
    }
}
