#pragma once

#include <optional>
#include <string>
#include <vector>

#include "SharpRuntime/SharpRuntimeHelper.hpp"
#include "System/Text/StringBuilder.hpp"
#include "GalaxyEggbert/TinyPoint.hpp"

namespace GalaxyEggbert
{
    using SharpRuntime::bytecs;
    using SharpRuntime::intcs;
    using std::string;

    /**
     * @brief Provides static helper methods for loading, reading, and writing world
     * and save-game data.
     *
     * This class is a C++ port of the original C# static class Worlds from
     * GalaxyEggbert.
     *
     * @note Status: Partial
     */
    class Worlds
    {
    public:
        Worlds() = delete;
        ~Worlds() = delete;

    private:
        static System::Text::StringBuilder output;
        /**
         * @brief Gets the filename used for game data storage.
         *
         * Corresponds to the original C# static read-only property GameDataFilename.
         *
         * @return Reference to the game data filename.
         *
         * @note Status: Ported
         */
        [[nodiscard]] static const std::string& getGameDataFilenameProperty();

        /**
         * @brief Gets the filename used for current game storage.
         *
         * Corresponds to the original C# static read-only property CurrentGameFilename.
         *
         * @return Reference to the current game filename.
         *
         * @note Status: Ported
         */
        [[nodiscard]] static const std::string& getCurrentGameFilenameProperty();

    public:
        /**
         * @brief Reads a world text file and splits it into lines.
         *
         * In the original C# code, this method returns null on failure.
         * In this C++ port, std::nullopt represents that state.
         *
         * @param gamer Gamer identifier. Present for compatibility with original signature.
         * @param rank World rank.
         * @return Optional vector of lines; std::nullopt on failure.
         *
         * @note Status: Partial
         */
        static std::optional<std::vector<std::string>> ReadWorld(intcs gamer, intcs rank);

    private:
        /**
         * @brief Gets the filename for a world by rank.
         *
         * Corresponds to the original C# method GetWorldFilename.
         *
         * @param gamer Gamer identifier. Present for compatibility with original signature.
         * @param rank World rank.
         * @return World filename.
         *
         * @note Status: Ported
         */
        static std::string GetWorldFilename(intcs gamer, intcs rank);

    public:
        /**
         * @brief Reads game data into the provided byte buffer.
         *
         * @param data Destination buffer.
         * @param dataSize Size of destination buffer.
         * @return True on success; otherwise false.
         *
         * @note Status: Partial
         */
        static bool ReadGameData(bytecs data[], size_t dataSize);

        /**
         * @brief Writes game data from the provided byte buffer.
         *
         * @param data Source buffer.
         * @param dataSize Size of source buffer.
         *
         * @note Status: Partial
         */
        static void WriteGameData(const bytecs data[], size_t dataSize);

        /**
         * @brief Deletes the current game save file if it exists.
         *
         * @note Status: Partial
         */
        static void DeleteCurrentGame();

        /**
         * @brief Reads the current game save text.
         *
         * In the original C# code, this method returns null on failure.
         * In this C++ port, std::nullopt represents that state.
         *
         * @return Optional save text; std::nullopt on failure.
         *
         * @note Status: Partial
         */
        static std::optional<string> ReadCurrentGame();

        /**
         * @brief Writes the current game save text.
         *
         * @param data Save text.
         *
         * @note Status: Partial
         */
        static void WriteCurrentGame(const string& data);

        /**
         * @brief Reads an integer array field from a section line.
         *
         * Mirrors the behavior of the original C# method as closely as practical.
         * Failed element parses are written as 0.
         *
         * @param lines Input lines.
         * @param lineCount Number of lines.
         * @param section Section name.
         * @param rank Zero-based occurrence rank within the section.
         * @param name Field name.
         * @param array Output array.
         * @param arraySize Size of output array.
         *
         * @note Status: Partial
         */
        static void GetIntArrayField(
            const string lines[],
            intcs lineCount,
            const string& section,
            intcs rank,
            const string& name,
            intcs array[],
            intcs arraySize);

        /**
         * @brief Reads a boolean field from a section line.
         *
         * Returns false on failure, matching the original C# behavior.
         *
         * @note Status: Ported
         */
        static bool GetBoolField(
            const string lines[],
            intcs lineCount,
            const string& section,
            intcs rank,
            const string& name);

        /**
         * @brief Reads an integer field from a section line.
         *
         * Returns 0 on failure, matching the original C# behavior.
         *
         * @note Status: Ported
         */
        static intcs GetIntField(
            const string lines[],
            intcs lineCount,
            const string& section,
            intcs rank,
            const string& name);

        /**
         * @brief Reads a double field from a section line.
         *
         * Returns 0.0 on failure, matching the original C# behavior.
         *
         * @note Status: Partial
         */
        static double GetDoubleField(
            const string lines[],
            intcs lineCount,
            const string& section,
            intcs rank,
            const string& name);

        /**
         * @brief Reads a TinyPoint field from a section line.
         *
         * Returns default TinyPoint on failure, matching the original C# behavior.
         *
         * @note Status: Ported
         */
        static TinyPoint GetPointField(
            const string lines[],
            intcs lineCount,
            const string& section,
            intcs rank,
            const string& name);

        /**
         * @brief Reads a decor field from a decor section.
         *
         * Corresponds to the original C# method returning int?.
         * std::nullopt represents null.
         *
         * @note Status: Ported
         */
        static std::optional<intcs> GetDecorField(
            const string lines[],
            intcs lineCount,
            const string& section,
            intcs x,
            intcs y);

        /**
         * @brief Reads door values from a section line.
         *
         * @param lines Input lines.
         * @param lineCount Number of lines.
         * @param section Section name.
         * @param doors Output array.
         * @param doorsSize Size of output array.
         *
         * @note Status: Partial
         */
        static void GetDoorsField(
            const string lines[],
            intcs lineCount,
            const string& section,
            intcs doors[],
            intcs doorsSize);

        /**
         * @brief Clears the write buffer.
         *
         * @note Status: Ported
         */
        static void WriteClear();

        /**
         * @brief Starts a new section in the write buffer.
         *
         * @param section Section name.
         *
         * @note Status: Ported
         */
        static void WriteSection(const string& section);

        /**
         * @brief Writes an integer array field to the write buffer.
         *
         * @note Status: Ported
         */
        static void WriteIntArrayField(const string& name, const intcs array[], intcs arraySize);

        /**
         * @brief Writes a boolean field to the write buffer.
         *
         * @note Status: Partial
         */
        static void WriteBoolField(const string& name, bool n);

        /**
         * @brief Writes an integer field to the write buffer.
         *
         * @note Status: Ported
         */
        static void WriteIntField(const string& name, intcs n);

        /**
         * @brief Writes a double field to the write buffer.
         *
         * @note Status: Partial
         */
        static void WriteDoubleField(const string& name, double n);

        /**
         * @brief Writes a TinyPoint field to the write buffer.
         *
         * @note Status: Ported
         */
        static void WritePointField(const string& name, TinyPoint p);

        /**
         * @brief Writes one decor line to the write buffer.
         *
         * @note Status: Ported
         */
        static void WriteDecorField(const intcs line[], intcs arraySize);

        /**
         * @brief Writes one doors line to the write buffer.
         *
         * @note Status: Ported
         */
        static void WriteDoorsField(const intcs doors[], intcs arraySize);

        /**
         * @brief Ends the current section in the write buffer.
         *
         * @note Status: Ported
         */
        static void WriteEndSection();

        /**
         * @brief Gets the accumulated write buffer string.
         *
         * @return Serialized text.
         *
         * @note Status: Ported
         */
        static string GetWriteString();
    };
}
