#pragma once

#include "GalaxyEggbert/Worlds/Block.hpp"
#include "GalaxyEggbert/Worlds/Chunk.hpp"
#include "GalaxyEggbert/Worlds/VoxelConfig.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

/**
 * @file
 * @brief Fixed-size voxel world container and world-file serialization API.
 */

namespace GalaxyEggbert::Worlds {

/**
 * @brief Small fixed-size voxel world built from 10 x 10 x 10 chunks.
 *
 * Default configuration:
 * - 10 chunks per axis
 * - 100 blocks per axis
 * - 1000 chunks in total
 */
class World final {
public:
    /**
     * @brief Creates a cubic world with a fixed chunk grid.
     *
     * @param chunksPerAxis Number of chunks along each axis.
     * @throws std::out_of_range If @p chunksPerAxis is zero.
     */
    explicit World(std::uint8_t chunksPerAxis = VoxelConfig::WorldChunksPerAxis);

    /**
     * @brief Returns the configured number of chunks along one world axis.
     */
    [[nodiscard]] std::uint8_t chunksPerAxis() const noexcept;

    /**
     * @brief Returns the number of blocks along one world axis.
     */
    [[nodiscard]] std::uint16_t blocksPerAxis() const noexcept;

    /**
     * @brief Returns total number of chunks (<tt>chunksPerAxis()^3</tt>).
     */
    [[nodiscard]] std::size_t chunkCount() const noexcept;

    /**
     * @brief Returns the world-level sky/background region id (2026-07-09,
     * format v2), a direct pass-through of mobile-eggbert's level-header
     * `region=` field (0-31) -- selects `Content/backgrounds/decorNNN.png`.
     * Defaults to 0 for worlds that never call @ref setSkyRegion.
     */
    [[nodiscard]] std::uint32_t skyRegion() const noexcept { return skyRegion_; }

    /**
     * @brief Sets the world-level sky/background region id (see @ref skyRegion).
     */
    void setSkyRegion(std::uint32_t skyRegion) noexcept { skyRegion_ = skyRegion; }

    /**
     * @brief Returns the world-level mission/level number (2026-07-13,
     * format v2's first reserved field put to use), a direct pass-through
     * of mobile-eggbert's real `m_mission` -- gates level-specific logic
     * such as the real training-hint overlay (missions 11-14 only, see
     * `Hud`/`TrainingHints`). Defaults to 0 (no mission) for worlds
     * that never call @ref setMissionNumber, matching real mobile-eggbert
     * levels outside the tutorial world range.
     */
    [[nodiscard]] std::uint32_t missionNumber() const noexcept { return missionNumber_; }

    /**
     * @brief Sets the world-level mission/level number (see @ref missionNumber).
     */
    void setMissionNumber(std::uint32_t missionNumber) noexcept { missionNumber_ = missionNumber; }

    /**
     * @brief Whether this world carries an explicit Blupi start cell.
     *
     * Old v2 worlds used zeroes in all three corresponding header slots and
     * therefore return false, preserving each caller's historical default.
     */
    [[nodiscard]] bool hasSpawnPoint() const noexcept { return hasSpawnPoint_; }
    [[nodiscard]] std::uint16_t spawnX() const noexcept { return spawnX_; }
    [[nodiscard]] std::uint16_t spawnY() const noexcept { return spawnY_; }
    [[nodiscard]] std::uint16_t spawnZ() const noexcept { return spawnZ_; }

    /**
     * @brief Sets the explicit raw-grid-space cell where Blupi starts.
     * @throws std::out_of_range If a coordinate is outside world bounds.
     */
    void setSpawnPoint(std::uint16_t x, std::uint16_t y, std::uint16_t z);

    /**
     * @brief Restores the legacy caller-defined default spawn behavior.
     */
    void clearSpawnPoint() noexcept { hasSpawnPoint_ = false; }

    /**
     * @brief Reads a block at world-space coordinates.
     *
     * @param x World X coordinate in range <tt>[0, blocksPerAxis())</tt>.
     * @param y World Y coordinate in range <tt>[0, blocksPerAxis())</tt>.
     * @param z World Z coordinate in range <tt>[0, blocksPerAxis())</tt>.
     * @return Block value stored at the requested position.
     * @throws std::out_of_range If coordinates are outside world bounds.
     */
    [[nodiscard]] Block getBlock(std::uint16_t x,
                                 std::uint16_t y,
                                 std::uint16_t z) const;

    /**
     * @brief Writes a block at world-space coordinates.
     *
     * @param x World X coordinate in range <tt>[0, blocksPerAxis())</tt>.
     * @param y World Y coordinate in range <tt>[0, blocksPerAxis())</tt>.
     * @param z World Z coordinate in range <tt>[0, blocksPerAxis())</tt>.
     * @param block New block value.
     * @throws std::out_of_range If coordinates are outside world bounds.
     */
    void setBlock(std::uint16_t x,
                  std::uint16_t y,
                  std::uint16_t z,
                  Block block);

    /**
     * @brief Provides read-only access to one chunk by chunk-grid coordinates.
     *
     * @param chunkX Chunk X coordinate in range <tt>[0, chunksPerAxis())</tt>.
     * @param chunkY Chunk Y coordinate in range <tt>[0, chunksPerAxis())</tt>.
     * @param chunkZ Chunk Z coordinate in range <tt>[0, chunksPerAxis())</tt>.
     * @return Const reference to the requested chunk.
     * @throws std::out_of_range If coordinates are outside chunk-grid bounds.
     */
    [[nodiscard]] const Chunk& chunk(std::uint8_t chunkX,
                                     std::uint8_t chunkY,
                                     std::uint8_t chunkZ) const;

    /**
     * @brief Provides mutable access to one chunk by chunk-grid coordinates.
     *
     * @param chunkX Chunk X coordinate in range <tt>[0, chunksPerAxis())</tt>.
     * @param chunkY Chunk Y coordinate in range <tt>[0, chunksPerAxis())</tt>.
     * @param chunkZ Chunk Z coordinate in range <tt>[0, chunksPerAxis())</tt>.
     * @return Mutable reference to the requested chunk.
     * @throws std::out_of_range If coordinates are outside chunk-grid bounds.
     */
    [[nodiscard]] Chunk& chunk(std::uint8_t chunkX,
                               std::uint8_t chunkY,
                               std::uint8_t chunkZ);

    /**
     * @brief One sparse extra-metadata record resolved to world-space coordinates
     * (see @ref collectExtraMetadata).
     */
    struct BlockExtraMetadataRecord final {
        std::uint16_t x = 0;
        std::uint16_t y = 0;
        std::uint16_t z = 0;
        std::uint16_t metadataType = 0;
        std::vector<std::uint8_t> payload;
    };

    /**
     * @brief Adds or replaces sparse extra metadata for the block at world-space
     * coordinates (see @ref Chunk::setExtraMetadata).
     *
     * @param x World X coordinate in range <tt>[0, blocksPerAxis())</tt>.
     * @param y World Y coordinate in range <tt>[0, blocksPerAxis())</tt>.
     * @param z World Z coordinate in range <tt>[0, blocksPerAxis())</tt>.
     * @param metadataType Type discriminator of the payload.
     * @param payload Type-specific raw bytes; payload length must fit into <tt>uint16_t</tt>.
     * @throws std::out_of_range If coordinates are outside world bounds.
     * @throws std::runtime_error If @p payload is too large for on-disk encoding.
     */
    void setBlockExtraMetadata(std::uint16_t x,
                               std::uint16_t y,
                               std::uint16_t z,
                               std::uint16_t metadataType,
                               std::vector<std::uint8_t> payload);

    /**
     * @brief Removes sparse extra metadata for the block at world-space
     * coordinates, if any (plan.md EDITOR-109 -- e.g. MoveObjectRecord::
     * RemoveMoveObject()'s own use, deleting a placed enemy/pickup/lift in
     * the in-game world editor).
     *
     * @param x World X coordinate in range <tt>[0, blocksPerAxis())</tt>.
     * @param y World Y coordinate in range <tt>[0, blocksPerAxis())</tt>.
     * @param z World Z coordinate in range <tt>[0, blocksPerAxis())</tt>.
     * @param metadataType Type discriminator of the payload to remove.
     * @return <tt>true</tt> when a record was actually removed.
     * @throws std::out_of_range If coordinates are outside world bounds.
     */
    bool removeBlockExtraMetadata(std::uint16_t x,
                                  std::uint16_t y,
                                  std::uint16_t z,
                                  std::uint16_t metadataType);

    /**
     * @brief Collects every sparse extra-metadata record of a given type across
     * the whole world, with local block indices resolved back to world-space
     * coordinates.
     *
     * @param metadataType Type discriminator to filter by.
     */
    [[nodiscard]] std::vector<BlockExtraMetadataRecord> collectExtraMetadata(
        std::uint16_t metadataType) const;

    /**
     * @brief Saves the world in the `.vwr` binary file format (header v2,
     * 2026-07-09).
     *
     * The file stores a world header (`VWR1` magic, format settings, chunk
     * count, table offsets, @ref skyRegion, @ref missionNumber, and the
     * optional @ref hasSpawnPoint coordinates), a chunk
     * table, and serialized non-empty chunk payloads.
     *
     * @param path Destination file path.
     * @throws std::runtime_error If file creation or serialization fails.
     */
    void saveToFile(const std::filesystem::path& path) const;

    /**
     * @brief Loads a world from the `.vwr` binary file format.
     *
     * Breaking change (2026-07-09): only header v2 is accepted -- v1 `.vwr`
     * files (written before @ref skyRegion existed) fail to load with
     * `std::runtime_error` and must be regenerated.
     *
     * @param path Source world file path.
     * @return Deserialized world.
     * @throws std::runtime_error If the file cannot be read, fails validation,
     *         or contains unsupported format parameters.
     */
    static World loadFromFile(const std::filesystem::path& path);

private:
    std::uint8_t chunksPerAxis_;
    std::vector<Chunk> chunks_;
    std::uint32_t skyRegion_ = 0;
    std::uint32_t missionNumber_ = 0;
    bool hasSpawnPoint_ = false;
    std::uint16_t spawnX_ = 0;
    std::uint16_t spawnY_ = 0;
    std::uint16_t spawnZ_ = 0;

    /**
     * @brief Converts chunk-grid coordinates to a linear vector index.
     */
    [[nodiscard]] std::size_t chunkLinearIndex(std::uint8_t chunkX,
                                               std::uint8_t chunkY,
                                               std::uint8_t chunkZ) const;

    /**
     * @brief Validates world-space block coordinates.
     *
     * @throws std::out_of_range If coordinates are outside world bounds.
     */
    void validateBlockPosition(std::uint16_t x,
                               std::uint16_t y,
                               std::uint16_t z) const;

    /**
     * @brief Validates chunk-grid coordinates.
     *
     * @throws std::out_of_range If coordinates are outside chunk-grid bounds.
     */
    void validateChunkPosition(std::uint8_t chunkX,
                               std::uint8_t chunkY,
                               std::uint8_t chunkZ) const;
};

} // namespace GalaxyEggbert::Worlds
