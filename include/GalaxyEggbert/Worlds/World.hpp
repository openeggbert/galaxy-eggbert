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
     * @brief Saves the world in the `.vwr` binary file format.
     *
     * The file stores a world header (`VWR1` magic, format settings, chunk count,
     * table offsets), a chunk table, and serialized non-empty chunk payloads.
     *
     * @param path Destination file path.
     * @throws std::runtime_error If file creation or serialization fails.
     */
    void saveToFile(const std::filesystem::path& path) const;

    /**
     * @brief Loads a world from the `.vwr` binary file format.
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
