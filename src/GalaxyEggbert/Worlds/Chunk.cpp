#include "GalaxyEggbert/Worlds/Chunk.hpp"

#include "GalaxyEggbert/Worlds/BinaryIO.hpp"
#include "GalaxyEggbert/Worlds/BitPacking.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <unordered_map>

namespace GalaxyEggbert::Worlds {

namespace {

constexpr char ChunkMagic[4] = {'V', 'C', 'H', '1'};
constexpr std::uint8_t ChunkFlagsNone = 0;

// Chunk header layout written by Chunk::write:
// magic[4], version u8, chunkSize u8, bitsPerBlock u8, flags u8,
// paletteCount u16, reserved u16, blockCount u32, dataSizeBytes u32.
constexpr std::uint32_t ExpectedBlockCount = static_cast<std::uint32_t>(Chunk::Volume);

} // namespace

Chunk::Chunk()
    : palette_{Block::air()},
      packedIndices_(packedWordCount(Volume, 1), 0),
      bitsPerBlock_(1),
      dirty_(false) {}

Chunk Chunk::createUniform(Block block) {
    Chunk chunk;
    chunk.palette_ = {block};
    chunk.bitsPerBlock_ = 1;
    chunk.packedIndices_ = std::vector<std::uint64_t>(packedWordCount(Volume, chunk.bitsPerBlock_), 0);
    chunk.dirty_ = false;
    return chunk;
}

Chunk Chunk::fromBlocks(const std::array<Block, Volume>& blocks) {
    Chunk chunk;
    chunk.repackFromBlocks(blocks);
    chunk.dirty_ = false;
    return chunk;
}

Block Chunk::getBlock(std::uint8_t localX,
                      std::uint8_t localY,
                      std::uint8_t localZ) const {
    validateLocalPosition(localX, localY, localZ);
    const std::size_t index = linearIndex(localX, localY, localZ);
    const std::uint16_t paletteIndex = getPackedIndex(packedIndices_, index, bitsPerBlock_);
    if (paletteIndex >= palette_.size()) {
        throw std::runtime_error("Chunk contains palette index outside of palette");
    }
    return palette_[paletteIndex];
}

void Chunk::setBlock(std::uint8_t localX,
                     std::uint8_t localY,
                     std::uint8_t localZ,
                     Block block) {
    validateLocalPosition(localX, localY, localZ);

    const std::uint16_t paletteIndex = findOrAddToPalette(block);
    setPackedIndex(packedIndices_, linearIndex(localX, localY, localZ), bitsPerBlock_, paletteIndex);
    dirty_ = true;
}

std::array<Block, Chunk::Volume> Chunk::unpackBlocks() const {
    std::array<Block, Volume> blocks{};
    for (std::size_t i = 0; i < Volume; ++i) {
        const std::uint16_t paletteIndex = getPackedIndex(packedIndices_, i, bitsPerBlock_);
        if (paletteIndex >= palette_.size()) {
            throw std::runtime_error("Chunk contains palette index outside of palette");
        }
        blocks[i] = palette_[paletteIndex];
    }
    return blocks;
}

bool Chunk::isEmpty() const noexcept {
    return palette_.size() == 1 && palette_.front().isAir();
}

bool Chunk::isUniform() const noexcept {
    return palette_.size() == 1;
}

bool Chunk::isDirty() const noexcept {
    return dirty_;
}

void Chunk::clearDirty() noexcept {
    dirty_ = false;
}

const std::vector<Block>& Chunk::palette() const noexcept {
    return palette_;
}

const std::vector<std::uint64_t>& Chunk::packedIndices() const noexcept {
    return packedIndices_;
}

std::uint8_t Chunk::bitsPerBlock() const noexcept {
    return bitsPerBlock_;
}

void Chunk::write(std::ostream& out) const {
    if (palette_.empty() || palette_.size() > VoxelConfig::MaxPaletteEntries) {
        throw std::runtime_error("Invalid chunk palette size");
    }

    const std::uint32_t dataSizeBytes = static_cast<std::uint32_t>(packedIndices_.size() * sizeof(std::uint64_t));

    Binary::writeMagic(out, ChunkMagic);
    Binary::writeU8(out, VoxelConfig::FormatVersion);
    Binary::writeU8(out, Size);
    Binary::writeU8(out, bitsPerBlock_);
    Binary::writeU8(out, ChunkFlagsNone);
    Binary::writeU16LE(out, static_cast<std::uint16_t>(palette_.size()));
    Binary::writeU16LE(out, 0); // reserved
    Binary::writeU32LE(out, ExpectedBlockCount);
    Binary::writeU32LE(out, dataSizeBytes);

    for (Block block : palette_) {
        Binary::writeU16LE(out, block.rawValue());
    }

    for (std::uint64_t word : packedIndices_) {
        Binary::writeU64LE(out, word);
    }
}

Chunk Chunk::read(std::istream& in) {
    char magic[4]{};
    Binary::readMagic(in, magic);
    if (std::memcmp(magic, ChunkMagic, 4) != 0) {
        throw std::runtime_error("Invalid chunk magic, expected VCH1");
    }

    const std::uint8_t version = Binary::readU8(in);
    const std::uint8_t chunkSize = Binary::readU8(in);
    const std::uint8_t bitsPerBlock = Binary::readU8(in);
    const std::uint8_t flags = Binary::readU8(in);
    const std::uint16_t paletteCount = Binary::readU16LE(in);
    static_cast<void>(Binary::readU16LE(in)); // reserved
    const std::uint32_t blockCount = Binary::readU32LE(in);
    const std::uint32_t dataSizeBytes = Binary::readU32LE(in);

    if (version != VoxelConfig::FormatVersion) {
        throw std::runtime_error("Unsupported chunk format version");
    }
    if (chunkSize != Size) {
        throw std::runtime_error("Unsupported chunk size in chunk file");
    }
    if (flags != ChunkFlagsNone) {
        throw std::runtime_error("Unsupported chunk flags: this reader supports only raw bit-packed chunks");
    }
    if (paletteCount == 0 || paletteCount > VoxelConfig::MaxPaletteEntries) {
        throw std::runtime_error("Invalid chunk palette count");
    }
    if (bitsPerBlock != bitsNeededForPalette(paletteCount)) {
        throw std::runtime_error("Chunk bitsPerBlock does not match palette size");
    }
    if (blockCount != ExpectedBlockCount) {
        throw std::runtime_error("Invalid block count in chunk file");
    }
    if (dataSizeBytes % sizeof(std::uint64_t) != 0) {
        throw std::runtime_error("Chunk packed data size must be multiple of 8 bytes");
    }

    Chunk chunk;
    chunk.palette_.clear();
    chunk.palette_.reserve(paletteCount);
    for (std::uint16_t i = 0; i < paletteCount; ++i) {
        chunk.palette_.push_back(Block(Binary::readU16LE(in)));
    }

    const std::size_t wordCount = dataSizeBytes / sizeof(std::uint64_t);
    const std::size_t expectedWordCount = packedWordCount(Volume, bitsPerBlock);
    if (wordCount != expectedWordCount) {
        throw std::runtime_error("Chunk packed word count does not match block count and bitsPerBlock");
    }

    chunk.packedIndices_.clear();
    chunk.packedIndices_.reserve(wordCount);
    for (std::size_t i = 0; i < wordCount; ++i) {
        chunk.packedIndices_.push_back(Binary::readU64LE(in));
    }

    chunk.bitsPerBlock_ = bitsPerBlock;
    chunk.dirty_ = false;
    return chunk;
}

std::size_t Chunk::linearIndex(std::uint8_t localX,
                               std::uint8_t localY,
                               std::uint8_t localZ) {
    return static_cast<std::size_t>(localX)
        + static_cast<std::size_t>(localY) * Size
        + static_cast<std::size_t>(localZ) * Size * Size;
}

void Chunk::validateLocalPosition(std::uint8_t localX,
                                  std::uint8_t localY,
                                  std::uint8_t localZ) {
    if (localX >= Size || localY >= Size || localZ >= Size) {
        throw std::out_of_range("Local chunk position is outside of 10 x 10 x 10 chunk");
    }
}

std::uint16_t Chunk::findOrAddToPalette(Block block) {
    const auto found = std::find(palette_.begin(), palette_.end(), block);
    if (found != palette_.end()) {
        return static_cast<std::uint16_t>(std::distance(palette_.begin(), found));
    }

    if (palette_.size() >= VoxelConfig::MaxPaletteEntries) {
        throw std::runtime_error("Chunk palette is full; v1 supports at most 256 entries per chunk");
    }

    const auto blocks = unpackBlocks();
    palette_.push_back(block);

    const std::uint8_t newBitsPerBlock = bitsNeededForPalette(palette_.size());
    if (newBitsPerBlock != bitsPerBlock_) {
        std::vector<std::uint16_t> indices;
        indices.reserve(Volume);
        for (Block existingBlock : blocks) {
            const auto it = std::find(palette_.begin(), palette_.end(), existingBlock);
            indices.push_back(static_cast<std::uint16_t>(std::distance(palette_.begin(), it)));
        }
        bitsPerBlock_ = newBitsPerBlock;
        packedIndices_ = packPaletteIndices(indices, bitsPerBlock_);
    }

    return static_cast<std::uint16_t>(palette_.size() - 1);
}

void Chunk::repackFromBlocks(const std::array<Block, Volume>& blocks) {
    palette_.clear();
    std::vector<std::uint16_t> indices;
    indices.reserve(Volume);

    for (Block block : blocks) {
        const auto found = std::find(palette_.begin(), palette_.end(), block);
        if (found != palette_.end()) {
            indices.push_back(static_cast<std::uint16_t>(std::distance(palette_.begin(), found)));
            continue;
        }

        if (palette_.size() >= VoxelConfig::MaxPaletteEntries) {
            throw std::runtime_error("Chunk palette is full; v1 supports at most 256 entries per chunk");
        }

        palette_.push_back(block);
        indices.push_back(static_cast<std::uint16_t>(palette_.size() - 1));
    }

    bitsPerBlock_ = bitsNeededForPalette(palette_.size());
    packedIndices_ = packPaletteIndices(indices, bitsPerBlock_);
    dirty_ = true;
}

} // namespace GalaxyEggbert::Worlds
