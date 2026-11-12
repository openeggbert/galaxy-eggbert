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
    inline constexpr std::uint8_t kProtocolVersion = 5; // 2: sounds, lifts, the aura, the cloud's touch, killers;
                                                        // 3: the match's lives and time limit, TimeUp,
                                                        //    a world of the host's editor sent (WorldData),
                                                        //    a dropped player back in the match (Rejoin, Snapshot);
                                                        // 4: names and chat in UTF-8 ("Hráč A", Czech's default name)
                                                        // 5: their limits in characters, not bytes (Ukrainian's
                                                        //    default "Гравець A" is 16 bytes)
    inline constexpr int kMaxPlayers = 4;     // the original's MAXNETPLAYER
    inline constexpr int kTeams = 4;          // yellow, orange, blue, green (the editor's starts)
    inline constexpr std::size_t kMaxName = 15;     // a CNA local profile's name (characters, as all the limits)
    inline constexpr std::size_t kMaxChat = 99;     // the original's chat line, its NUL taken off
    inline constexpr std::size_t kMaxTextBytes = 255; // a text's bytes on the wire (its length a byte)
    inline constexpr std::size_t kMaxEvents = 64;   // per Events packet
    inline constexpr int kMultiplayerLives = 10;    // PlayPrepare: m_nbVies = 10 in a network game
    inline constexpr std::size_t kMaxSounds = 32;   // per Sounds packet
    inline constexpr std::size_t kMaxLifts = 64;    // per Lifts packet
    inline constexpr int kSoundChannels = 93;       // Content/sounds/sound000-092
    inline constexpr std::uint8_t kNoKiller = 0xFF; // Die: no other player's doing
    inline constexpr std::uint8_t kMaxMinutes = 60; // a match's time limit (Galaxy's, Q-022); 0 none
    inline constexpr std::size_t kMaxWorldName = 31;          // a world of the editor's name
    inline constexpr std::size_t kWorldChunk = 1024;          // WorldData's bytes per packet
    inline constexpr std::uint32_t kMaxWorldBytes = 8u << 20; // a world sent, at most (a .vwr: some 40 KB)

    enum class MessageKind : std::uint8_t
    {
        Hello = 1,   // a player who joined, to everyone: version, content, name
        Lobby = 2,   // the host: the world chosen and the four places (also during a match: who is back)
        Team = 3,    // a player's colour (0-3)
        Ready = 4,   // a player ready or not
        Chat = 5,    // a line of chat, to everyone
        Start = 6,   // the host: the match begins in this world
        Pose = 7,    // a player's Blupi, unreliable, about 20 a second
        Events = 8,  // the changes a player's Blupi made to the world, reliable
        Die = 9,     // a player lost a life (the lives left)
        Lost = 10,   // a player died with no life in reserve: out of the match (and who killed him)
        Abandon = 11, // a player left the match
        Pause = 12,  // a player paused or went on: everyone's game stops (the original's PK_PAUSE)
        Sounds = 13, // the sounds a player's Blupi made, where (PK_PLAYSOUND / PK_STOPSOUND)
        Lifts = 14,  // the host: lifts at the start of a stop (PK_ASCENSEURSTART / PK_ASCENSEUREND)
        StopCloud = 15, // to one player: his cloud touched another and is gone (PK_STOPCLOUD)
        TimeUp = 16, // the host: the match's time is over, each place's final rank (Galaxy's, Q-022)
        WorldData = 17, // the host: a piece of the world of his editor chosen for the match (Galaxy's, Q-022)
        Rejoin = 18,    // the host: a player who had dropped is back in the match (Galaxy's, Q-022): to
                        // him the match as it is, to the others that he is back
        Snapshot = 19,  // the host, to the one back: the world's changes since the start (as Events)
    };

    struct LobbySlot
    {
        bool present = false;
        bool ready = false;
        bool playing = false;     // still in the match the host started (back in the lobby: false)
        std::uint8_t team = 0;
        std::uint8_t gamerId = 0; // CNA's NetworkGamer id of whoever holds the place
        std::string name;
    };

    struct LobbyState
    {
        std::uint16_t world = 201; // the multiplayer world (worlds3d/world201.vwr ..., SB2's numbers)
        // The match's settings, the host's (Galaxy's, Q-022): each player's reserve lives (the
        // original's ten) and a time limit in minutes (0: none, the original's last survivor only).
        std::uint8_t lives = kMultiplayerLives;
        std::uint8_t minutes = 0;
        // A world of the host's editor in place of `world` (0: none): its bytes' hash and its name;
        // WorldData brings it.
        std::uint64_t customHash = 0;
        std::string customName;
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
        std::uint8_t aura = 0;    // his secret power (Def::SecretPower 0-4): the original's m_blupiSec
    };
    inline constexpr std::uint8_t kAuraLimit = 4;   // SecretPower::Hide
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

    // A sound a Blupi made (or stopped), at his place: the others play it as a sound of the world.
    struct SoundEvent
    {
        std::uint8_t channel = 0; // 0-92
        bool stop = false;
        float x = 0.0f, y = 0.0f, z = 0.0f;
    };

    // A lift (by its index in the world) put at the start of its stop at one end (the patrol's step
    // 1, at its start, or 3, at its end), as the host's is.
    struct LiftSync
    {
        std::int32_t index = 0;
        std::uint8_t step = 1;
    };

    // A lobby place's statistics as the host has them (Rejoin).
    struct ScoreWire
    {
        bool played = false;
        bool left = false;  // out by leaving (or silence), not knocked out
        std::uint8_t deaths = 0, kills = 0, out = 0, team = 0;
        std::string name;
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
        std::uint8_t killer = kNoKiller; // Die, Lost: the place (0-3) of the player who killed him
        std::uint8_t target = 0;         // StopCloud: the gamer whose cloud is gone
        std::uint8_t minutes = 0;        // Start: the time limit (lives: the reserve lives)
        std::array<std::uint8_t, kMaxPlayers> places{}; // TimeUp: each lobby place's rank (1-4; 0 not in it)
        // Start: the editor's world played (0 none); WorldData: that world's hash, its size, where
        // this piece goes and the piece.
        std::uint64_t customHash = 0;
        // Rejoin: the place he plays at (target: his gamer), the match's time played, every place's
        // statistics, where his spawned objects' numbers begin (not those he made before).
        std::uint8_t rejoinSlot = 0;
        float seconds = 0.0f;
        std::array<ScoreWire, kMaxPlayers> scores{};
        std::uint32_t firstSpawn = 1;
        std::uint32_t worldSize = 0;
        std::uint32_t worldOffset = 0;
        std::vector<std::uint8_t> chunk;
        // Pose
        Pose pose;
        // Events
        std::vector<WorldEvent> events;
        // Sounds, Lifts
        std::vector<SoundEvent> sounds;
        std::vector<LiftSync> lifts;
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
    // What names and chat may hold: printable ASCII and whole UTF-8 characters from U+00A0 on (no
    // control, no broken sequence), at most @p limit characters and kMaxTextBytes bytes, a character
    // never cut (protocol 5; 4 counted bytes).
    [[nodiscard]] std::string CleanText(const std::string& text, std::size_t limit);
    // @p text in printable ASCII, a Latin letter without its mark ("Hráč" -> "Hrac", "ß" -> "ss"), any
    // other character left out: for CNA's local profile name.
    [[nodiscard]] std::string AsciiText(const std::string& text, std::size_t limit);
}
