#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @file
 * @brief Compile-time limits and default dimensions for the voxel world format.
 */

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
    /**
     * @brief Binary format version written to the `.vwr` world file header
     * (`World::saveToFile`/`loadFromFile` -- the `VWR1` header's own
     * `version` byte, distinct from `Chunk`'s independently-versioned
     * payload format). Bumped 1 -> 2 (2026-07-09, breaking change, not
     * backward compatible -- old v1 `.vwr` files fail to load and must be
     * regenerated) to add a `skyRegion` world header field plus 4 more
     * reserved fields for future world-level metadata; see `World Format.md`.
     */
    static constexpr std::uint8_t FormatVersion = 2;

    /**
     * @brief Number of blocks along one axis of a chunk.
     */
    static constexpr std::uint8_t ChunkSize = 10;

    /**
     * @brief Total number of blocks in one chunk (<tt>ChunkSize^3</tt>).
     */
    static constexpr std::size_t ChunkVolume =
        static_cast<std::size_t>(ChunkSize) * ChunkSize * ChunkSize;

    /**
     * @brief Default number of chunks along one world axis.
     */
    static constexpr std::uint8_t WorldChunksPerAxis = 10;

    /**
     * @brief Default number of blocks along one world axis.
     */
    static constexpr std::uint16_t WorldBlocksPerAxis =
        static_cast<std::uint16_t>(ChunkSize) * WorldChunksPerAxis;

    /**
     * @brief Largest supported block type identifier (12-bit field).
     */
    static constexpr std::uint16_t MaxBlockType = 4095;      // 12 bits

    /**
     * @brief Largest supported block metadata value (4-bit field).
     */
    static constexpr std::uint8_t MaxBlockMetadata = 15;     // 4 bits

    /**
     * @brief Maximum number of entries in one chunk palette.
     */
    static constexpr std::uint16_t MaxPaletteEntries = 256;  // indices are stored in up to 8 bits
};

} // namespace GalaxyEggbert::Worlds
