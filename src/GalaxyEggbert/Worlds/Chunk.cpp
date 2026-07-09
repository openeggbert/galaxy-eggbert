#include "GalaxyEggbert/Worlds/Chunk.hpp"

#include "GalaxyEggbert/Worlds/BinaryIO.hpp"
#include "GalaxyEggbert/Worlds/BitPacking.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace GalaxyEggbert::Worlds {

namespace {

constexpr char ChunkMagic[4] = {'V', 'C', 'H', '1'};
constexpr char BlockMetadataMagic[4] = {'B', 'M', 'D', '1'};
constexpr std::uint8_t ChunkFormatVersionV1 = 1;
constexpr std::uint8_t ChunkFormatVersionV2 = 2;
constexpr std::uint16_t BlockMetadataVersionV1 = 1;
constexpr std::uint8_t ChunkFlagsNone = 0;
constexpr std::uint8_t ChunkFlagHasExtraMetadata = 0x01;
constexpr std::uint8_t ChunkFlagsKnownMask = ChunkFlagHasExtraMetadata;

// Chunk header layout written by Chunk::write:
// magic[4], version u8, chunkSize u8, bitsPerBlock u8, flags u8,
// paletteCount u16, reserved u16, blockCount u32, dataSizeBytes u32,
// extraMetaSizeBytes u32 (for version 2 and newer).
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
    return palette_.size() == 1 && palette_.front().isAir() && extraMetadata_.empty();
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

const std::vector<ChunkBlockMetadataRecord>& Chunk::extraMetadata() const noexcept {
    return extraMetadata_;
}

void Chunk::setExtraMetadata(std::uint32_t localBlockIndex,
                             std::uint16_t metadataType,
                             std::vector<std::uint8_t> payload) {
    validateLocalBlockIndex(localBlockIndex);
    if (payload.size() > std::numeric_limits<std::uint16_t>::max()) {
        throw std::runtime_error("Chunk extra metadata payload exceeds uint16_t limit");
    }

    const auto existing = std::find_if(extraMetadata_.begin(), extraMetadata_.end(),
                                       [localBlockIndex, metadataType](const ChunkBlockMetadataRecord& record) {
                                           return record.localBlockIndex == localBlockIndex
                                               && record.metadataType == metadataType;
                                       });
    if (existing == extraMetadata_.end()) {
        extraMetadata_.push_back(ChunkBlockMetadataRecord{localBlockIndex, metadataType, std::move(payload)});
        dirty_ = true;
        return;
    }

    if (existing->payload != payload) {
        existing->payload = std::move(payload);
        dirty_ = true;
    }
}

bool Chunk::removeExtraMetadata(std::uint32_t localBlockIndex,
                                std::uint16_t metadataType) {
    const auto originalSize = extraMetadata_.size();
    extraMetadata_.erase(std::remove_if(extraMetadata_.begin(), extraMetadata_.end(),
                                        [localBlockIndex, metadataType](const ChunkBlockMetadataRecord& record) {
                                            return record.localBlockIndex == localBlockIndex
                                                && record.metadataType == metadataType;
                                        }),
                         extraMetadata_.end());
    const bool removed = extraMetadata_.size() != originalSize;
    if (removed) {
        dirty_ = true;
    }
    return removed;
}

void Chunk::clearExtraMetadata() {
    if (!extraMetadata_.empty()) {
        extraMetadata_.clear();
        dirty_ = true;
    }
}

void Chunk::write(std::ostream& out) const {
    if (palette_.empty() || palette_.size() > VoxelConfig::MaxPaletteEntries) {
        throw std::runtime_error("Invalid chunk palette size");
    }
    if (bitsPerBlock_ != bitsNeededForPalette(palette_.size())) {
        throw std::runtime_error("Chunk bitsPerBlock is inconsistent with chunk palette");
    }

    const std::uint64_t packedBytes = packedIndices_.size() * sizeof(std::uint64_t);
    if (packedBytes > std::numeric_limits<std::uint32_t>::max()) {
        throw std::runtime_error("Chunk packed index stream is too large");
    }
    const std::uint32_t dataSizeBytes = static_cast<std::uint32_t>(packedBytes);
    std::uint64_t extraMetaSizeBytes = 0;
    if (!extraMetadata_.empty()) {
        extraMetaSizeBytes = 12; // section header: magic + version + reserved + recordCount
        for (const ChunkBlockMetadataRecord& record : extraMetadata_) {
            validateLocalBlockIndex(record.localBlockIndex);
            if (record.payload.size() > std::numeric_limits<std::uint16_t>::max()) {
                throw std::runtime_error("Chunk extra metadata payload exceeds uint16_t limit");
            }
            extraMetaSizeBytes += 8 + record.payload.size();
        }
        if (extraMetaSizeBytes > std::numeric_limits<std::uint32_t>::max()) {
            throw std::runtime_error("Chunk extra metadata section is too large");
        }
    }

    Binary::writeMagic(out, ChunkMagic);
    Binary::writeU8(out, ChunkFormatVersionV2);
    Binary::writeU8(out, Size);
    Binary::writeU8(out, bitsPerBlock_);
    const std::uint8_t flags = extraMetadata_.empty() ? ChunkFlagsNone : ChunkFlagHasExtraMetadata;
    Binary::writeU8(out, flags);
    Binary::writeU16LE(out, static_cast<std::uint16_t>(palette_.size()));
    Binary::writeU16LE(out, 0); // reserved
    Binary::writeU32LE(out, ExpectedBlockCount);
    Binary::writeU32LE(out, dataSizeBytes);
    Binary::writeU32LE(out, static_cast<std::uint32_t>(extraMetaSizeBytes));

    for (Block block : palette_) {
        Binary::writeU16LE(out, block.rawValue());
    }

    for (std::uint64_t word : packedIndices_) {
        Binary::writeU64LE(out, word);
    }

    if (!extraMetadata_.empty()) {
        Binary::writeMagic(out, BlockMetadataMagic);
        Binary::writeU16LE(out, BlockMetadataVersionV1);
        Binary::writeU16LE(out, 0); // reserved
        Binary::writeU32LE(out, static_cast<std::uint32_t>(extraMetadata_.size()));

        for (const ChunkBlockMetadataRecord& record : extraMetadata_) {
            Binary::writeU32LE(out, record.localBlockIndex);
            Binary::writeU16LE(out, record.metadataType);
            Binary::writeU16LE(out, static_cast<std::uint16_t>(record.payload.size()));
            if (!record.payload.empty()) {
                out.write(reinterpret_cast<const char*>(record.payload.data()),
                          static_cast<std::streamsize>(record.payload.size()));
                if (!out) {
                    throw std::runtime_error("Failed to write chunk extra metadata payload");
                }
            }
        }
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
    const std::uint16_t reserved = Binary::readU16LE(in);
    const std::uint32_t blockCount = Binary::readU32LE(in);
    const std::uint32_t dataSizeBytes = Binary::readU32LE(in);
    std::uint32_t extraMetaSizeBytes = 0;
    if (version == ChunkFormatVersionV2) {
        extraMetaSizeBytes = Binary::readU32LE(in);
    } else if (version != ChunkFormatVersionV1) {
        throw std::runtime_error("Unsupported chunk format version");
    }
    if (chunkSize != Size) {
        throw std::runtime_error("Unsupported chunk size in chunk file");
    }
    if (reserved != 0) {
        throw std::runtime_error("Chunk reserved header field must be zero");
    }
    if ((flags & ~ChunkFlagsKnownMask) != 0) {
        throw std::runtime_error("Unsupported chunk flags");
    }
    if (version == ChunkFormatVersionV1 && flags != ChunkFlagsNone) {
        throw std::runtime_error("Chunk v1 does not support flags");
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
    const bool hasExtraMetadata = (flags & ChunkFlagHasExtraMetadata) != 0;
    if (version == ChunkFormatVersionV2 && hasExtraMetadata != (extraMetaSizeBytes > 0)) {
        throw std::runtime_error("Chunk extra metadata flag does not match extra metadata size");
    }
    if (version == ChunkFormatVersionV1 && extraMetaSizeBytes != 0) {
        throw std::runtime_error("Chunk v1 cannot contain extra metadata size field");
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

    chunk.extraMetadata_.clear();
    if (extraMetaSizeBytes > 0) {
        std::string sectionBytes(extraMetaSizeBytes, '\0');
        in.read(sectionBytes.data(), static_cast<std::streamsize>(extraMetaSizeBytes));
        if (!in) {
            throw std::runtime_error("Failed to read chunk extra metadata section");
        }

        std::istringstream sectionStream(sectionBytes, std::ios::binary);
        char sectionMagic[4]{};
        Binary::readMagic(sectionStream, sectionMagic);
        if (std::memcmp(sectionMagic, BlockMetadataMagic, 4) != 0) {
            throw std::runtime_error("Invalid chunk extra metadata magic, expected BMD1");
        }

        const std::uint16_t metadataVersion = Binary::readU16LE(sectionStream);
        const std::uint16_t metadataReserved = Binary::readU16LE(sectionStream);
        const std::uint32_t recordCount = Binary::readU32LE(sectionStream);
        if (metadataVersion != BlockMetadataVersionV1) {
            throw std::runtime_error("Unsupported chunk extra metadata version");
        }
        if (metadataReserved != 0) {
            throw std::runtime_error("Chunk extra metadata reserved header field must be zero");
        }

        chunk.extraMetadata_.reserve(recordCount);
        for (std::uint32_t i = 0; i < recordCount; ++i) {
            const std::uint32_t localBlockIndex = Binary::readU32LE(sectionStream);
            const std::uint16_t metadataType = Binary::readU16LE(sectionStream);
            const std::uint16_t payloadSize = Binary::readU16LE(sectionStream);
            if (localBlockIndex >= Volume) {
                throw std::runtime_error("Chunk extra metadata localBlockIndex is outside chunk volume");
            }

            std::vector<std::uint8_t> payload(payloadSize, 0);
            if (payloadSize > 0) {
                sectionStream.read(reinterpret_cast<char*>(payload.data()), payloadSize);
                if (!sectionStream) {
                    throw std::runtime_error("Failed to read chunk extra metadata payload");
                }
            }

            chunk.extraMetadata_.push_back(
                ChunkBlockMetadataRecord{localBlockIndex, metadataType, std::move(payload)});
        }

        if (sectionStream.peek() != std::char_traits<char>::eof()) {
            throw std::runtime_error("Chunk extra metadata section contains trailing bytes");
        }
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

void Chunk::validateLocalBlockIndex(std::uint32_t localBlockIndex) {
    if (localBlockIndex >= Volume) {
        throw std::out_of_range("Local block index is outside of chunk volume");
    }
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
