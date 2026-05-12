#include "GalaxyEggbert/Worlds.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "CNA/Logger.hpp"
#include "Microsoft/Xna/Framework/TitleContainer.hpp"
#include "System/Exception.hpp"
#include "System/String.hpp"
#include "System/IO/FileMode.hpp"
#include "System/IO/StreamReader.hpp"
#include "System/IO/IsolatedStorage/IsolatedStorageException.hpp"
#include "System/IO/IsolatedStorage/IsolatedStorageFile.hpp"
#include "System/IO/IsolatedStorage/IsolatedStorageFileStream.hpp"
#include "System/Text/Encoding.hpp"

namespace GalaxyEggbert
{
    using log = CNA::Logger;
    using String = System::String;

    System::Text::StringBuilder Worlds::output;

    namespace
    {
        [[nodiscard]] bool TryParseInt(const std::string& s, intcs& result)
        {
            try
            {
                std::size_t pos = 0;
                int value = std::stoi(s, &pos);
                if (pos != s.size())
                {
                    return false;
                }
                result = static_cast<intcs>(value);
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        [[nodiscard]] bool TryParseDouble(const std::string& s, double& result)
        {
            try
            {
                std::size_t pos = 0;
                double value = std::stod(s, &pos);
                if (pos != s.size())
                {
                    return false;
                }
                result = value;
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        [[nodiscard]] bool TryParseBool(const std::string& s, bool& result)
        {
            std::string lower = s;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            if (lower == "true")
            {
                result = true;
                return true;
            }
            if (lower == "false")
            {
                result = false;
                return true;
            }
            return false;
        }
    }

    const std::string& Worlds::getGameDataFilenameProperty()
    {
        static const std::string gameDataFilename = "SpeedyBlupi";
        return gameDataFilename;
    }

    const std::string& Worlds::getCurrentGameFilenameProperty()
    {
        static const std::string currentGameFilename = "CurrentGame";
        return currentGameFilename;
    }

    std::optional<std::vector<std::string>> Worlds::ReadWorld(intcs gamer, intcs rank)
    {
        (void)gamer;

        const std::string worldFilename = GetWorldFilename(gamer, rank);

        string text;
        bool loaded = false;

        try
        {
            auto stream = Microsoft::Xna::Framework::TitleContainer::OpenStream(worldFilename);
            System::IO::StreamReader streamReader(stream.get());
            text = streamReader.ReadToEnd();
            stream->Close();
            loaded = true;
        }
        catch (const System::Exception& e)
        {
            log::Error(e.getMessageProperty());
            log::Error("Fatal error. Loading world failed: " + worldFilename);

            //Environment.Exit(1);
        }
        catch (const std::exception& e)
        {
            log::Error(e.what());
            log::Error("Fatal error. Loading world failed: " + worldFilename);
        }

        // if (String::IsEmpty(text))
        // {
        //     return std::nullopt;
        // }

        if (!loaded)
        {
            return std::nullopt;
        }
        return String::Split(text, '\n');
    }

    std::string Worlds::GetWorldFilename(intcs gamer, intcs rank)
    {
        (void)gamer;

        // std::ostringstream oss;
        // oss << "worlds/world" << std::setw(3) << std::setfill('0') << rank << ".txt";
        // return oss.str();

        return System::String::Format(
            "worlds/world{0}.txt",
            System::String::ToString(rank, 3)
            );
    }

    bool Worlds::ReadGameData(bytecs data[], size_t dataSize)
    {
        log::Debug("ReadGameData");

        auto userStoreForApplication = System::IO::IsolatedStorage::IsolatedStorageFile::GetUserStoreForApplication();
        if (userStoreForApplication.FileExists(getGameDataFilenameProperty()))
        {
            try
            {
                auto isolatedStorageFileStream =
                    userStoreForApplication.OpenFile(
                        getGameDataFilenameProperty(),
                        System::IO::FileMode::Open);

                const intcs count = std::min(
                    static_cast<intcs>(dataSize),
                    isolatedStorageFileStream.getLengthProperty());

                isolatedStorageFileStream.Read(data, 0, count);
                isolatedStorageFileStream.Close();
                return true;
            }
            catch (const System::IO::IsolatedStorage::IsolatedStorageException&)
            {
                return false;
            }
        }
        return false;
    }

    System::IO::IsolatedStorage::IsolatedStorageFile getUserStoreForApplication()
    {
        return System::IO::IsolatedStorage::IsolatedStorageFile::GetUserStoreForApplication();
    }

    void Worlds::WriteGameData(const bytecs data[], size_t dataSize)
    {
        log::Debug("WriteGameData");

        auto userStoreForApplication = getUserStoreForApplication();

        try
        {
            auto isolatedStorageFileStream =
                userStoreForApplication.OpenFile(
                    getGameDataFilenameProperty(),
                    System::IO::FileMode::Create);

            isolatedStorageFileStream.Write(
                data,
                0,
                static_cast<SharpRuntime::intcs>(dataSize));

            isolatedStorageFileStream.Close();
        }
        catch (const System::IO::IsolatedStorage::IsolatedStorageException& e)
        {
            log::Error(e.getMessageProperty());
        }
    }

    void Worlds::DeleteCurrentGame()
    {
        log::Debug("DeleteCurrentGame");

        auto userStoreForApplication = getUserStoreForApplication();
        try
        {
            userStoreForApplication.DeleteFile(getCurrentGameFilenameProperty());
        }
        catch (...)
        {
        }
    }

    std::optional<string> Worlds::ReadCurrentGame()
    {
        log::Debug("ReadCurrentGame");

        auto userStoreForApplication = getUserStoreForApplication();

        if (!userStoreForApplication.FileExists(getCurrentGameFilenameProperty()))
        {
            return std::nullopt;
        }

        try
        {
            auto stream =
                userStoreForApplication.OpenFile(
                    getCurrentGameFilenameProperty(),
                    System::IO::FileMode::Open);

            const intcs length = stream.getLengthProperty();
            if (length <= 0)
            {
                return string{};
            }

            std::vector<bytecs> buffer(static_cast<size_t>(length));

            stream.Read(
                buffer.data(),
                0,
                length);

            return System::Text::Encoding::UTF8()->GetString(
                buffer.data(),
                0,
                length);
        }
        catch (const System::IO::IsolatedStorage::IsolatedStorageException&)
        {
            return std::nullopt;
        }
    }
    void Worlds::WriteCurrentGame(const string& data)
    {
        log::Debug("WriteCurrentGame");

        auto userStoreForApplication = getUserStoreForApplication();

        auto isolatedStorageFileStream =
            userStoreForApplication.OpenFile(
                getCurrentGameFilenameProperty(),
                System::IO::FileMode::Create);

        std::vector<bytecs> bytes = System::Text::Encoding::UTF8()->GetBytes(data);

        isolatedStorageFileStream.Write(
            bytes.data(),
            0,
            static_cast<intcs>(bytes.size()));

        isolatedStorageFileStream.Close();
    }

    void Worlds::GetIntArrayField(
        const string lines[],
        intcs lineCount,
        const string& section,
        intcs rank,
        const string& name,
        intcs array[],
        intcs arraySize)
    {
        for (intcs i = 0; i < lineCount; i++)
        {
            const string& text = lines[i];

            if (!String::StartsWith(text, section + ":") || rank-- != 0)
            {
                continue;
            }

            const std::size_t num = text.find(name + "=");
            if (num == string::npos)
            {
                break;
            }

            const std::size_t start = num + name.length() + 1;
            const std::size_t end = text.find(" ", start);
            if (end == string::npos)
            {
                break;
            }

            const std::vector<std::string> values = String::Split(text.substr(start, end - start), ',');

            for (intcs j = 0; j < static_cast<intcs>(values.size()) && j < arraySize; j++)
            {
                intcs parsedValue = 0;
                if (TryParseInt(values[j], parsedValue))
                {
                    array[j] = parsedValue;
                }
                else
                {
                    array[j] = 0;
                }
            }

            break;
        }
    }

    namespace
    {
        [[nodiscard]] std::optional<std::string> TryGetFieldValueText(
            const string lines[],
            intcs lineCount,
            const string& section,
            intcs rank,
            const string& name)
        {
            for (intcs i = 0; i < lineCount; i++)
            {
                const string& text = lines[i];

                if (String::StartsWith(text, section + ":") && rank-- == 0)
                {
                    std::size_t num = text.find(name + "=");
                    if (num == string::npos)
                    {
                        return std::nullopt;
                    }

                    num += name.length() + 1;

                    const std::size_t num2 = text.find(" ", num);
                    if (num2 == string::npos)
                    {
                        return std::nullopt;
                    }

                    return text.substr(num, num2 - num);
                }
            }

            return std::nullopt;
        }
    }
    bool Worlds::GetBoolField(
     const string lines[],
     intcs lineCount,
     const string& section,
     intcs rank,
     const string& name)
    {
        const auto valueText = TryGetFieldValueText(lines, lineCount, section, rank, name);
        if (!valueText.has_value())
        {
            return false;
        }

        bool result = false;
        if (TryParseBool(valueText.value(), result))
        {
            return result;
        }

        return false;
    }

    intcs Worlds::GetIntField(
      const string lines[],
      intcs lineCount,
      const string& section,
      intcs rank,
      const string& name)
    {
        const auto valueText = TryGetFieldValueText(lines, lineCount, section, rank, name);
        if (!valueText.has_value())
        {
            return 0;
        }

        intcs result = 0;
        if (TryParseInt(valueText.value(), result))
        {
            return result;
        }

        return 0;
    }

    double Worlds::GetDoubleField(
       const string lines[],
       intcs lineCount,
       const string& section,
       intcs rank,
       const string& name)
    {
        const auto valueText = TryGetFieldValueText(lines, lineCount, section, rank, name);
        if (!valueText.has_value())
        {
            return 0.0;
        }

        double result = 0.0;
        if (TryParseDouble(valueText.value(), result))
        {
            return result;
        }

        return 0.0;
    }

    TinyPoint Worlds::GetPointField(
        const string lines[],
        intcs lineCount,
        const string& section,
        intcs rank,
        const string& name)
    {
        for (intcs i = 0; i < lineCount; i++)
        {
            const string& text = lines[i];
            if (String::StartsWith(text, section + ":") && rank-- == 0)
            {
                std::size_t num = text.find(name + "=");
                if (num == string::npos)
                {
                    return TinyPoint{};
                }

                num += name.length() + 1;
                const std::size_t num2 = text.find(";", num);
                if (num2 == string::npos)
                {
                    return TinyPoint{};
                }

                const std::size_t num3 = text.find(" ", num);
                if (num3 == string::npos)
                {
                    return TinyPoint{};
                }

                const string s1 = text.substr(num, num2 - num);
                const string s2 = text.substr(num2 + 1, num3 - num2 - 1);

                intcs x = 0;
                if (!TryParseInt(s1, x))
                {
                    return TinyPoint{};
                }

                intcs y = 0;
                if (!TryParseInt(s2, y))
                {
                    return TinyPoint{};
                }

                TinyPoint result{};
                result.X = x;
                result.Y = y;
                return result;
            }
        }

        return TinyPoint{};
    }

    std::optional<intcs> Worlds::GetDecorField(
        const string lines[],
        intcs lineCount,
        const string& section,
        intcs x,
        intcs y)
    {
        for (intcs i = 0; i < lineCount; i++)
        {
            string text = lines[i];
            if (String::StartsWith(text, section + ":"))
            {
                const intcs rowIndex = i + 1 + x;
                if (rowIndex < 0 || rowIndex >= lineCount)
                {
                    return std::nullopt;
                }

                text = lines[rowIndex];
                const std::vector<std::string> parts = String::Split(text, ',');

                if (y < 0 || y >= static_cast<intcs>(parts.size()))
                {
                    return std::nullopt;
                }

                if (parts[y].empty())
                {
                    return static_cast<intcs>(-1);
                }

                intcs result = 0;
                if (TryParseInt(parts[y], result))
                {
                    return result;
                }

                return std::nullopt;
            }
        }

        return std::nullopt;
    }

    void Worlds::GetDoorsField(
        const string lines[],
        intcs lineCount,
        const string& section,
        intcs doors[],
        intcs doorsSize)
    {
        for (intcs i = 0; i < lineCount; i++)
        {
            const string& text = lines[i];
            if (!String::StartsWith(text, section + ":"))
            {
                continue;
            }

            const string payload = text.substr(section.length() + 2);
            const std::vector<std::string> parts = String::Split(payload, ',');

            for (intcs j = 0; j < static_cast<intcs>(parts.size()) && j < doorsSize; j++)
            {
                intcs result = 0;
                if (parts[j].empty())
                {
                    doors[j] = 1;
                }
                else if (TryParseInt(parts[j], result))
                {
                    doors[j] = result;
                }
            }
        }
    }

    void Worlds::WriteClear()
    {
        output.Clear();
    }

    void Worlds::WriteSection(const string& section)
    {
        output.Append(section);
        output.Append(": ");
    }

    void Worlds::WriteIntArrayField(const string& name, const intcs array[], intcs arraySize)
    {
        output.Append(name);
        output.Append("=");
        for (intcs i = 0; i < arraySize; i++)
        {
            if (array[i] != 0)
            {
                output.Append(array[i]);
            }
            if (i < arraySize - 1)
            {
                output.Append(",");
            }
        }
        output.Append(" ");
    }

    void Worlds::WriteBoolField(const string& name, bool n)
    {
        output.Append(name);
        output.Append("=");
        output.Append(n ? "True" : "False");
        output.Append(" ");
    }

    void Worlds::WriteIntField(const string& name, intcs n)
    {
        output.Append(name);
        output.Append("=");
        output.Append(n);
        output.Append(" ");
    }

    void Worlds::WriteDoubleField(const string& name, double n)
    {
        std::ostringstream ss;
        ss.imbue(std::locale::classic());
        ss << std::setprecision(15) << n;

        output.Append(name);
        output.Append("=");
        output.Append(ss.str());
        output.Append(" ");
    }

    void Worlds::WritePointField(const string& name, TinyPoint p)
    {
        output.Append(name);
        output.Append("=");
        output.Append(p.X);
        output.Append(";");
        output.Append(p.Y);
        output.Append(" ");
    }

    void Worlds::WriteDecorField(const intcs line[], intcs arraySize)
    {
        for (intcs i = 0; i < arraySize; i++)
        {
            if (line[i] != -1)
            {
                output.Append(line[i]);
            }
            if (i < arraySize - 1)
            {
                output.Append(",");
            }
        }
        output.Append("\n");
    }

    void Worlds::WriteDoorsField(const intcs doors[], intcs arraySize)
    {
        for (intcs i = 0; i < arraySize; i++)
        {
            if (doors[i] != 1)
            {
                output.Append(doors[i]);
            }
            if (i < arraySize - 1)
            {
                output.Append(",");
            }
        }
        output.Append("\n");
    }

    void Worlds::WriteEndSection()
    {
        output.Append("\n");
    }

    string Worlds::GetWriteString()
    {
        return output.ToString();
    }
}
