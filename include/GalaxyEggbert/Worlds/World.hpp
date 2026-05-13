#pragma once

#include "GalaxyEggbert/Worlds/Block.hpp"
#include "GalaxyEggbert/Worlds/Chunk.hpp"
#include "GalaxyEggbert/Worlds/VoxelConfig.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

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
    explicit World(std::uint8_t chunksPerAxis = VoxelConfig::WorldChunksPerAxis);

    [[nodiscard]] std::uint8_t chunksPerAxis() const noexcept;
    [[nodiscard]] std::uint16_t blocksPerAxis() const noexcept;
    [[nodiscard]] std::size_t chunkCount() const noexcept;

    [[nodiscard]] Block getBlock(std::uint16_t x,
                                 std::uint16_t y,
                                 std::uint16_t z) const;

    void setBlock(std::uint16_t x,
                  std::uint16_t y,
                  std::uint16_t z,
                  Block block);

    [[nodiscard]] const Chunk& chunk(std::uint8_t chunkX,
                                     std::uint8_t chunkY,
                                     std::uint8_t chunkZ) const;

    [[nodiscard]] Chunk& chunk(std::uint8_t chunkX,
                               std::uint8_t chunkY,
                               std::uint8_t chunkZ);

    void saveToFile(const std::filesystem::path& path) const;
    static World loadFromFile(const std::filesystem::path& path);

private:
    std::uint8_t chunksPerAxis_;
    std::vector<Chunk> chunks_;

    [[nodiscard]] std::size_t chunkLinearIndex(std::uint8_t chunkX,
                                               std::uint8_t chunkY,
                                               std::uint8_t chunkZ) const;

    void validateBlockPosition(std::uint16_t x,
                               std::uint16_t y,
                               std::uint16_t z) const;

    void validateChunkPosition(std::uint8_t chunkX,
                               std::uint8_t chunkY,
                               std::uint8_t chunkZ) const;
};

} // namespace GalaxyEggbert::Worlds
