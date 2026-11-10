#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace GalaxyEggbert::Net
{
    // Galaxy Eggbert's network game (the owner, 2026-11-10: Speedy Blupi 2's multiplayer as
    // ../blupi_recon reconstructs it, plans/plan_gemultiplayer.md, owner_questions.md Q-019). Each
    // machine simulates its own Blupi and sends his pose and the changes he made to the world; the
    // host keeps the lobby and starts the match. These are the messages, their little-endian byte
    // layout and their checks: a packet is decoded whole and checked before anything of it is used
    // (the original applied whatever arrived; blupi_recon's validator is the model). No CNA here.

    inline constexpr std::uint16_t kProtocolId = 0x4547; // "GE"
    inline constexpr std::uint8_t kProtocolVersion = 1;
    inline constexpr int kMaxPlayers = 4;     // the original's MAXNETPLAYER
    inline constexpr int kTeams = 4;          // yellow, orange, blue, green (the editor's starts)
    inline constexpr std::size_t kMaxName = 15;     // a CNA local profile's name
    inline constexpr std::size_t kMaxChat = 99;     // the original's chat line, its NUL taken off
    inline constexpr std::size_t kMaxEvents = 64;   // per Events packet
    inline constexpr int kMultiplayerLives = 10;    // PlayPrepare: m_nbVies = 10 in a network game

    enum class MessageKind : std::uint8_t
    {
        Hello = 1,   // a player who joined, to everyone: version, content, name
        Lobby = 2,   // the host: the world chosen and the four places
        Team = 3,    // a player's colour (0-3)
        Ready = 4,   // a player ready or not
        Chat = 5,    // a line of chat, to everyone
        Start = 6,   // the host: the match begins in this world
        Pose = 7,    // a player's Blupi, unreliable, about 20 a second
        Events = 8,  // the changes a player's Blupi made to the world, reliable
        Die = 9,     // a player lost a life (the lives left)
        Lost = 10,   // a player has no life left
        Abandon = 11, // a player left the match
        Pause = 12,  // a player paused or went on: everyone's game stops (the original's PK_PAUSE)
    };

    struct LobbySlot
    {
        bool present = false;
        bool ready = false;
        std::uint8_t team = 0;
        std::uint8_t gamerId = 0; // CNA's NetworkGamer id of whoever holds the place
        std::string name;
    };

    struct LobbyState
    {
        std::uint16_t world = 201; // the multiplayer world (worlds3d/world201.vwr ..., SB2's numbers)
        std::array<LobbySlot, kMaxPlayers> slots{};
    };

    // What a remote machine needs to draw a Blupi: his model's transform as his own machine draws
    // it (hanging, tilted, charred and all), the clip and its time.
    struct Pose
    {
        std::uint32_t tick = 0;   // the sender's simulation tick: an older pose is dropped
        float x = 0.0f, y = 0.0f, z = 0.0f; // where he is (world units)
        std::array<float, 12> world{}; // the model's matrix, rows 1-4 without the last column
        std::uint16_t clip = 0;   // index in the Blupi model's sorted clip names
        float clipSeconds = 0.0f;
        std::uint8_t alpha = 255; // the drink's Hide and the like
        std::uint8_t flags = 0;   // kPoseHidden ...
        std::uint8_t lives = 0;
    };
    inline constexpr std::uint8_t kPoseHidden = 1;  // not drawn (the original's icon -1, SEC_HIDE)
    inline constexpr std::uint8_t kPoseShield = 2;
    inline constexpr std::uint8_t kPoseCharred = 4; // burnt black
    inline constexpr std::uint8_t kPoseGlued = 8;   // the glue's green puddle
    inline constexpr std::uint8_t kPoseFlags = 15;

    enum class EventKind : std::uint8_t
    {
        ObjectRetired = 1,  // a world object (by its index in the world) taken or destroyed
        BlockChanged = 2,   // a cell of the world changed (a door opened, a wall blown)
        ObjectSpawned = 3,  // an object a Blupi made (a bomb laid, dynamite lit): its net id
        SpawnRetired = 4,   // such an object gone
    };

    struct WorldEvent
    {
        EventKind kind = EventKind::ObjectRetired;
        std::int32_t id = 0;      // object index, or the spawned object's net id
        std::int32_t a = 0, b = 0, c = 0; // BlockChanged: x, y, z; ObjectSpawned: type, then b-d its
        std::int32_t d = 0;       // end (a shot's) in 1/256 world units; BlockChanged d: the raw block
        float x = 0.0f, y = 0.0f, z = 0.0f; // ObjectSpawned: where
    };

    struct Message
    {
        MessageKind kind = MessageKind::Hello;
        // Hello
        std::uint32_t simulationRevision = 0;
        std::uint64_t contentHash = 0;
        std::string name;
        // Lobby, Start (world), Team, Ready (Pause: paused), Chat, Die
        LobbyState lobby;
        std::uint16_t world = 0;
        std::uint64_t worldHash = 0;
        std::uint8_t team = 0;
        bool ready = false;
        std::string text;
        std::uint8_t lives = 0;
        // Pose
        Pose pose;
        // Events
        std::vector<WorldEvent> events;
    };

    // The bytes of a message, envelope included (protocol id, version, kind).
    [[nodiscard]] std::vector<std::uint8_t> Encode(const Message& message);
    // A message from bytes; nothing when the packet is not exactly a valid one of ours (another
    // game or version, a wrong length, a value out of range, a name or text too long or not printable).
    [[nodiscard]] std::optional<Message> Decode(const std::uint8_t* data, std::size_t size);
    [[nodiscard]] inline std::optional<Message> Decode(const std::vector<std::uint8_t>& bytes)
    {
        return Decode(bytes.data(), bytes.size());
    }

    // Whether the message must arrive (lobby, chat, events, deaths) or may be lost (poses).
    [[nodiscard]] bool IsReliable(MessageKind kind) noexcept;
    // Printable ASCII of at most @p limit characters (what names and chat may hold).
    [[nodiscard]] std::string CleanText(const std::string& text, std::size_t limit);
}
