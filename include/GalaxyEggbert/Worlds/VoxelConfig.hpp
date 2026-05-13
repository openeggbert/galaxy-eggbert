#pragma once

#include <cstddef>
#include <cstdint>

namespace GalaxyEggbert::Worlds {

/**
 * @brief Constants for the first version of the small voxel world format.
 *
 * The default world is 100 x 100 x 100 blocks:
 * - chunk size: 10 x 10 x 10 blocks
 * - chunks per axis: 10
 * - total chunks: 1000
 */
struct VoxelConfig final {
    static constexpr std::uint8_t FormatVersion = 1;

    static constexpr std::uint8_t ChunkSize = 10;
    static constexpr std::size_t ChunkVolume =
        static_cast<std::size_t>(ChunkSize) * ChunkSize * ChunkSize;

    static constexpr std::uint8_t WorldChunksPerAxis = 10;
    static constexpr std::uint16_t WorldBlocksPerAxis =
        static_cast<std::uint16_t>(ChunkSize) * WorldChunksPerAxis;

    static constexpr std::uint16_t MaxBlockType = 4095;      // 12 bits
    static constexpr std::uint8_t MaxBlockMetadata = 15;     // 4 bits
    static constexpr std::uint16_t MaxPaletteEntries = 256;  // indices are stored in up to 8 bits
};

} // namespace GalaxyEggbert::Worlds
