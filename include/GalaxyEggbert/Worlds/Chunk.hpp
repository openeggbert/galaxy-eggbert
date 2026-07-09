#pragma once

#include "GalaxyEggbert/Worlds/Block.hpp"
#include "GalaxyEggbert/Worlds/VoxelConfig.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <vector>

/**
 * @file
 * @brief Palette-compressed chunk container and binary serialization API.
 */

namespace GalaxyEggbert::Worlds {

/**
 * @brief Sparse metadata payload assigned to one concrete block in a chunk.
 */
struct ChunkBlockMetadataRecord final {
    /**
     * @brief Local linear block index in chunk coordinates.
     */
    std::uint32_t localBlockIndex = 0;

    /**
     * @brief Type discriminator of the payload.
     */
    std::uint16_t metadataType = 0;

    /**
     * @brief Type-specific payload bytes.
     */
    std::vector<std::uint8_t> payload;

    [[nodiscard]] bool operator==(const ChunkBlockMetadataRecord& other) const {
        return localBlockIndex == other.localBlockIndex
            && metadataType == other.metadataType
            && payload == other.payload;
    }
};

/**
 * @brief Palette-compressed 10 x 10 x 10 voxel chunk.
 *
 * The chunk stores palette indices in an adaptive bit-packed stream.
 * It does not store a render mesh. Mesh data should be runtime cache only.
 */
class Chunk final {
public:
    /**
     * @brief Number of blocks along one local chunk axis.
     */
    static constexpr std::uint8_t Size = VoxelConfig::ChunkSize;

    /**
     * @brief Total number of blocks in one chunk (<tt>Size^3</tt>).
     */
    static constexpr std::size_t Volume = VoxelConfig::ChunkVolume;

    /**
     * @brief Creates a new all-air chunk.
     *
     * The chunk starts as a uniform palette with one entry (@ref Block::air),
     * one bit per packed index, and a clean (<tt>isDirty() == false</tt>) state.
     */
    Chunk();

    /**
     * @brief Creates a chunk where all blocks have the same value.
     *
     * @param block Value to assign to every local block position.
     * @return A uniform chunk with a single-entry palette.
     */
    static Chunk createUniform(Block block);

    /**
     * @brief Creates a chunk from explicit block data.
     *
     * @param blocks Block values in local linear order produced by @ref linearIndex.
     * @return A palette-compressed chunk equivalent to @p blocks.
     * @throws std::out_of_range If the palette would exceed
     *         @ref VoxelConfig::MaxPaletteEntries.
     */
    static Chunk fromBlocks(const std::array<Block, Volume>& blocks);

    /**
     * @brief Returns the block value at a local chunk coordinate.
     *
     * @param localX Local X coordinate in range <tt>[0, Size)</tt>.
     * @param localY Local Y coordinate in range <tt>[0, Size)</tt>.
     * @param localZ Local Z coordinate in range <tt>[0, Size)</tt>.
     * @return Block stored at the requested position.
     * @throws std::out_of_range If any coordinate is outside <tt>[0, Size)</tt>.
     * @throws std::runtime_error If packed indices reference an invalid palette entry.
     */
    [[nodiscard]] Block getBlock(std::uint8_t localX,
                                 std::uint8_t localY,
                                 std::uint8_t localZ) const;

    /**
     * @brief Sets the block value at a local chunk coordinate.
     *
     * The palette is grown automatically when @p block is not already present.
     *
     * @param localX Local X coordinate in range <tt>[0, Size)</tt>.
     * @param localY Local Y coordinate in range <tt>[0, Size)</tt>.
     * @param localZ Local Z coordinate in range <tt>[0, Size)</tt>.
     * @param block New block value.
     * @throws std::out_of_range If coordinates are out of range or if the palette
     *         cannot represent additional distinct values.
     */
    void setBlock(std::uint8_t localX,
                  std::uint8_t localY,
                  std::uint8_t localZ,
                  Block block);

    /**
     * @brief Decompresses all blocks into a dense fixed-size array.
     *
     * @return Array with @ref Volume entries in @ref linearIndex order.
     * @throws std::runtime_error If packed indices reference an invalid palette entry.
     */
    [[nodiscard]] std::array<Block, Volume> unpackBlocks() const;

    /**
     * @brief Checks whether the chunk is entirely air AND carries no sparse
     * extra metadata (see @ref extraMetadata). World::saveToFile() uses this
     * to skip serializing a chunk entirely -- a chunk with metadata but only
     * air blocks (e.g. a MoveObject anchored on an otherwise-empty chunk)
     * must NOT be considered empty, or its metadata is silently lost on
     * save/load (found and fixed 2026-07-09).
     */
    [[nodiscard]] bool isEmpty() const noexcept;

    /**
     * @brief Checks whether all blocks in the chunk are equal.
     */
    [[nodiscard]] bool isUniform() const noexcept;

    /**
     * @brief Reports whether any mutating operation has modified the chunk.
     */
    [[nodiscard]] bool isDirty() const noexcept;

    /**
     * @brief Clears the dirty marker.
     */
    void clearDirty() noexcept;

    /**
     * @brief Provides read-only access to the current palette.
     */
    [[nodiscard]] const std::vector<Block>& palette() const noexcept;

    /**
     * @brief Provides read-only access to packed palette indices.
     */
    [[nodiscard]] const std::vector<std::uint64_t>& packedIndices() const noexcept;

    /**
     * @brief Returns the current packed bit width used per block index.
     */
    [[nodiscard]] std::uint8_t bitsPerBlock() const noexcept;

    /**
     * @brief Returns sparse per-block metadata records stored in this chunk.
     */
    [[nodiscard]] const std::vector<ChunkBlockMetadataRecord>& extraMetadata() const noexcept;

    /**
     * @brief Adds or replaces sparse metadata for one local block index and type.
     *
     * @param localBlockIndex Local linear index in range <tt>[0, Volume)</tt>.
     * @param metadataType Type discriminator of the payload.
     * @param payload Type-specific raw bytes; payload length must fit into <tt>uint16_t</tt>.
     * @throws std::out_of_range If @p localBlockIndex is out of range.
     * @throws std::runtime_error If @p payload is too large for on-disk encoding.
     */
    void setExtraMetadata(std::uint32_t localBlockIndex,
                          std::uint16_t metadataType,
                          std::vector<std::uint8_t> payload);

    /**
     * @brief Removes sparse metadata for one local block index and type.
     *
     * @return <tt>true</tt> when a record was removed.
     */
    bool removeExtraMetadata(std::uint32_t localBlockIndex,
                             std::uint16_t metadataType);

    /**
     * @brief Converts local 3D chunk coordinates to the linear block index
     * used by @ref setExtraMetadata / @ref ChunkBlockMetadataRecord::localBlockIndex.
     *
     * Public so callers outside this class (e.g. @ref World) can address a
     * specific block's extra metadata without duplicating this layout.
     *
     * @param localX Local X coordinate in range <tt>[0, Size)</tt>.
     * @param localY Local Y coordinate in range <tt>[0, Size)</tt>.
     * @param localZ Local Z coordinate in range <tt>[0, Size)</tt>.
     */
    [[nodiscard]] static std::size_t linearIndex(std::uint8_t localX,
                                                 std::uint8_t localY,
                                                 std::uint8_t localZ);

    /**
     * @brief Removes all sparse metadata records from this chunk.
     */
    void clearExtraMetadata();

    /**
     * @brief Serializes the chunk to a binary stream.
     *
     * The on-disk format includes a chunk header (`VCH1` magic, format version,
     * chunk size, bit width, palette size, block count, payload size), followed by
     * palette entries and packed index words.
     *
     * @param out Target binary stream.
     * @throws std::runtime_error If the chunk state is inconsistent or stream writes fail.
     */
    void write(std::ostream& out) const;

    /**
     * @brief Deserializes one chunk from a binary stream.
     *
     * @param in Source binary stream positioned at the beginning of a chunk record.
     * @return Parsed chunk instance.
     * @throws std::runtime_error If stream reads fail or serialized data is malformed.
     */
    static Chunk read(std::istream& in);

private:
    std::vector<Block> palette_;
    std::vector<std::uint64_t> packedIndices_;
    std::vector<ChunkBlockMetadataRecord> extraMetadata_;
    std::uint8_t bitsPerBlock_ = 1;
    bool dirty_ = false;

    /**
     * @brief Validates local linear block index inside this chunk.
     *
     * @throws std::out_of_range If index is outside <tt>[0, Volume)</tt>.
     */
    static void validateLocalBlockIndex(std::uint32_t localBlockIndex);

    /**
     * @brief Validates that local coordinates are inside chunk bounds.
     *
     * @throws std::out_of_range If any coordinate is outside <tt>[0, Size)</tt>.
     */
    static void validateLocalPosition(std::uint8_t localX,
                                      std::uint8_t localY,
                                      std::uint8_t localZ);

    /**
     * @brief Finds a block in the palette or appends it when missing.
     *
     * @param block Block value to find or insert.
     * @return Palette index of @p block after insertion/lookup.
     * @throws std::out_of_range If adding the block would exceed palette limits.
     */
    std::uint16_t findOrAddToPalette(Block block);

    /**
     * @brief Rebuilds palette and packed indices from explicit block values.
     *
     * @param blocks Block values in @ref linearIndex order.
     * @throws std::out_of_range If the required palette exceeds configured limits.
     */
    void repackFromBlocks(const std::array<Block, Volume>& blocks);
};

} // namespace GalaxyEggbert::Worlds
