#pragma once

#include "GalaxyEggbert/Worlds/Block.hpp"
#include "GalaxyEggbert/Worlds/VoxelConfig.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <vector>

namespace GalaxyEggbert::Worlds {

/**
 * @brief Palette-compressed 10 x 10 x 10 voxel chunk.
 *
 * The chunk stores palette indices in an adaptive bit-packed stream.
 * It does not store a render mesh. Mesh data should be runtime cache only.
 */
class Chunk final {
public:
    static constexpr std::uint8_t Size = VoxelConfig::ChunkSize;
    static constexpr std::size_t Volume = VoxelConfig::ChunkVolume;

    Chunk();

    static Chunk createUniform(Block block);
    static Chunk fromBlocks(const std::array<Block, Volume>& blocks);

    [[nodiscard]] Block getBlock(std::uint8_t localX,
                                 std::uint8_t localY,
                                 std::uint8_t localZ) const;

    void setBlock(std::uint8_t localX,
                  std::uint8_t localY,
                  std::uint8_t localZ,
                  Block block);

    [[nodiscard]] std::array<Block, Volume> unpackBlocks() const;

    [[nodiscard]] bool isEmpty() const noexcept;
    [[nodiscard]] bool isUniform() const noexcept;
    [[nodiscard]] bool isDirty() const noexcept;
    void clearDirty() noexcept;

    [[nodiscard]] const std::vector<Block>& palette() const noexcept;
    [[nodiscard]] const std::vector<std::uint64_t>& packedIndices() const noexcept;
    [[nodiscard]] std::uint8_t bitsPerBlock() const noexcept;

    void write(std::ostream& out) const;
    static Chunk read(std::istream& in);

private:
    std::vector<Block> palette_;
    std::vector<std::uint64_t> packedIndices_;
    std::uint8_t bitsPerBlock_ = 1;
    bool dirty_ = false;

    static std::size_t linearIndex(std::uint8_t localX,
                                   std::uint8_t localY,
                                   std::uint8_t localZ);

    static void validateLocalPosition(std::uint8_t localX,
                                      std::uint8_t localY,
                                      std::uint8_t localZ);

    std::uint16_t findOrAddToPalette(Block block);
    void repackFromBlocks(const std::array<Block, Volume>& blocks);
};

} // namespace GalaxyEggbert::Worlds
