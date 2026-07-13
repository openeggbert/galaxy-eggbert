#include "GalaxyEggbert/Worlds/World.hpp"

#include "GalaxyEggbert/Worlds/BinaryIO.hpp"

#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace GalaxyEggbert::Worlds {

namespace {

constexpr char WorldMagic[4] = {'V', 'W', 'R', '1'};
constexpr std::uint8_t WorldFlagsNone = 0;
constexpr std::uint16_t ChunkTableFlagEmpty = 0x0001;

// v2 (2026-07-09, breaking change): 20-byte v1 core (magic/version/
// chunkSize/chunksPerAxis/flags/chunkCount/tableOffset/dataOffset) + 4-byte
// skyRegion + 4 reserved uint32 fields (16 bytes) for future world-level
// metadata (e.g. spawn point) -- see World Format.md. 2026-07-13: the first
// reserved field is now missionNumber (still 4 total header uint32 fields
// after skyRegion, WorldHeaderSize unchanged -- just repurposing one
// already-reserved slot, not a format/size change), leaving 3 truly
// reserved.
constexpr std::uint32_t WorldHeaderSize = 40;
constexpr std::uint32_t WorldHeaderReservedFieldCount = 3;
constexpr std::uint32_t ChunkTableEntrySize = 20;

struct SerializedChunkEntry final {
    std::uint64_t offset = 0;
    std::uint32_t size = 0;
    std::uint16_t x = 0;
    std::uint16_t y = 0;
    std::uint16_t z = 0;
    std::uint16_t flags = 0;
};

std::string serializeChunk(const Chunk& chunk) {
    std::ostringstream out(std::ios::out | std::ios::binary);
    chunk.write(out);
    return out.str();
}

Chunk deserializeChunk(const std::string& data) {
    std::istringstream in(data, std::ios::in | std::ios::binary);
    return Chunk::read(in);
}

void writeChunkTableEntry(std::ostream& out, const SerializedChunkEntry& entry) {
    Binary::writeU64LE(out, entry.offset);
    Binary::writeU32LE(out, entry.size);
    Binary::writeU16LE(out, entry.x);
    Binary::writeU16LE(out, entry.y);
    Binary::writeU16LE(out, entry.z);
    Binary::writeU16LE(out, entry.flags);
}

SerializedChunkEntry readChunkTableEntry(std::istream& in) {
    SerializedChunkEntry entry;
    entry.offset = Binary::readU64LE(in);
    entry.size = Binary::readU32LE(in);
    entry.x = Binary::readU16LE(in);
    entry.y = Binary::readU16LE(in);
    entry.z = Binary::readU16LE(in);
    entry.flags = Binary::readU16LE(in);
    return entry;
}

} // namespace

World::World(std::uint8_t chunksPerAxis)
    : chunksPerAxis_(chunksPerAxis),
      chunks_(static_cast<std::size_t>(chunksPerAxis) * chunksPerAxis * chunksPerAxis) {
    if (chunksPerAxis_ == 0) {
        throw std::out_of_range("World must have at least one chunk per axis");
    }
}

std::uint8_t World::chunksPerAxis() const noexcept {
    return chunksPerAxis_;
}

std::uint16_t World::blocksPerAxis() const noexcept {
    return static_cast<std::uint16_t>(chunksPerAxis_) * VoxelConfig::ChunkSize;
}

std::size_t World::chunkCount() const noexcept {
    return chunks_.size();
}

Block World::getBlock(std::uint16_t x,
                      std::uint16_t y,
                      std::uint16_t z) const {
    validateBlockPosition(x, y, z);

    const auto chunkX = static_cast<std::uint8_t>(x / VoxelConfig::ChunkSize);
    const auto chunkY = static_cast<std::uint8_t>(y / VoxelConfig::ChunkSize);
    const auto chunkZ = static_cast<std::uint8_t>(z / VoxelConfig::ChunkSize);

    const auto localX = static_cast<std::uint8_t>(x % VoxelConfig::ChunkSize);
    const auto localY = static_cast<std::uint8_t>(y % VoxelConfig::ChunkSize);
    const auto localZ = static_cast<std::uint8_t>(z % VoxelConfig::ChunkSize);

    return chunk(chunkX, chunkY, chunkZ).getBlock(localX, localY, localZ);
}

void World::setBlock(std::uint16_t x,
                     std::uint16_t y,
                     std::uint16_t z,
                     Block blockValue) {
    validateBlockPosition(x, y, z);

    const auto chunkX = static_cast<std::uint8_t>(x / VoxelConfig::ChunkSize);
    const auto chunkY = static_cast<std::uint8_t>(y / VoxelConfig::ChunkSize);
    const auto chunkZ = static_cast<std::uint8_t>(z / VoxelConfig::ChunkSize);

    const auto localX = static_cast<std::uint8_t>(x % VoxelConfig::ChunkSize);
    const auto localY = static_cast<std::uint8_t>(y % VoxelConfig::ChunkSize);
    const auto localZ = static_cast<std::uint8_t>(z % VoxelConfig::ChunkSize);

    chunk(chunkX, chunkY, chunkZ).setBlock(localX, localY, localZ, blockValue);
}

void World::setBlockExtraMetadata(std::uint16_t x,
                                  std::uint16_t y,
                                  std::uint16_t z,
                                  std::uint16_t metadataType,
                                  std::vector<std::uint8_t> payload) {
    validateBlockPosition(x, y, z);

    const auto chunkX = static_cast<std::uint8_t>(x / VoxelConfig::ChunkSize);
    const auto chunkY = static_cast<std::uint8_t>(y / VoxelConfig::ChunkSize);
    const auto chunkZ = static_cast<std::uint8_t>(z / VoxelConfig::ChunkSize);

    const auto localX = static_cast<std::uint8_t>(x % VoxelConfig::ChunkSize);
    const auto localY = static_cast<std::uint8_t>(y % VoxelConfig::ChunkSize);
    const auto localZ = static_cast<std::uint8_t>(z % VoxelConfig::ChunkSize);

    const auto localBlockIndex = static_cast<std::uint32_t>(Chunk::linearIndex(localX, localY, localZ));
    chunk(chunkX, chunkY, chunkZ).setExtraMetadata(localBlockIndex, metadataType, std::move(payload));
}

std::vector<World::BlockExtraMetadataRecord> World::collectExtraMetadata(std::uint16_t metadataType) const {
    std::vector<BlockExtraMetadataRecord> records;

    for (std::uint16_t chunkZ = 0; chunkZ < chunksPerAxis_; ++chunkZ) {
        for (std::uint16_t chunkY = 0; chunkY < chunksPerAxis_; ++chunkY) {
            for (std::uint16_t chunkX = 0; chunkX < chunksPerAxis_; ++chunkX) {
                const Chunk& currentChunk = chunk(static_cast<std::uint8_t>(chunkX),
                                                  static_cast<std::uint8_t>(chunkY),
                                                  static_cast<std::uint8_t>(chunkZ));
                for (const ChunkBlockMetadataRecord& record : currentChunk.extraMetadata()) {
                    if (record.metadataType != metadataType) {
                        continue;
                    }

                    const auto localIndex = record.localBlockIndex;
                    const auto localX = static_cast<std::uint16_t>(localIndex % VoxelConfig::ChunkSize);
                    const auto localY = static_cast<std::uint16_t>((localIndex / VoxelConfig::ChunkSize) % VoxelConfig::ChunkSize);
                    const auto localZ = static_cast<std::uint16_t>(localIndex / (VoxelConfig::ChunkSize * VoxelConfig::ChunkSize));

                    BlockExtraMetadataRecord resolved;
                    resolved.x = static_cast<std::uint16_t>(chunkX * VoxelConfig::ChunkSize + localX);
                    resolved.y = static_cast<std::uint16_t>(chunkY * VoxelConfig::ChunkSize + localY);
                    resolved.z = static_cast<std::uint16_t>(chunkZ * VoxelConfig::ChunkSize + localZ);
                    resolved.metadataType = record.metadataType;
                    resolved.payload = record.payload;
                    records.push_back(std::move(resolved));
                }
            }
        }
    }

    return records;
}

const Chunk& World::chunk(std::uint8_t chunkX,
                          std::uint8_t chunkY,
                          std::uint8_t chunkZ) const {
    validateChunkPosition(chunkX, chunkY, chunkZ);
    return chunks_[chunkLinearIndex(chunkX, chunkY, chunkZ)];
}

Chunk& World::chunk(std::uint8_t chunkX,
                    std::uint8_t chunkY,
                    std::uint8_t chunkZ) {
    validateChunkPosition(chunkX, chunkY, chunkZ);
    return chunks_[chunkLinearIndex(chunkX, chunkY, chunkZ)];
}

void World::saveToFile(const std::filesystem::path& path) const {
    struct ChunkBlob final {
        SerializedChunkEntry entry;
        std::string data;
    };

    std::vector<ChunkBlob> blobs;
    blobs.reserve(chunks_.size());

    const std::uint64_t tableOffset = WorldHeaderSize;
    const std::uint64_t dataOffset = tableOffset + static_cast<std::uint64_t>(chunks_.size()) * ChunkTableEntrySize;
    std::uint64_t currentOffset = dataOffset;

    for (std::uint16_t z = 0; z < chunksPerAxis_; ++z) {
        for (std::uint16_t y = 0; y < chunksPerAxis_; ++y) {
            for (std::uint16_t x = 0; x < chunksPerAxis_; ++x) {
                const Chunk& currentChunk = chunk(static_cast<std::uint8_t>(x),
                                                  static_cast<std::uint8_t>(y),
                                                  static_cast<std::uint8_t>(z));
                ChunkBlob blob;
                blob.entry.x = x;
                blob.entry.y = y;
                blob.entry.z = z;

                if (currentChunk.isEmpty()) {
                    blob.entry.flags = ChunkTableFlagEmpty;
                    blob.entry.offset = 0;
                    blob.entry.size = 0;
                } else {
                    blob.data = serializeChunk(currentChunk);
                    if (blob.data.size() > std::numeric_limits<std::uint32_t>::max()) {
                        throw std::runtime_error("Serialized chunk is too large for v1 world file");
                    }
                    blob.entry.flags = 0;
                    blob.entry.offset = currentOffset;
                    blob.entry.size = static_cast<std::uint32_t>(blob.data.size());
                    currentOffset += blob.entry.size;
                }

                blobs.push_back(std::move(blob));
            }
        }
    }

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Cannot open world file for writing");
    }

    Binary::writeMagic(out, WorldMagic);
    Binary::writeU8(out, VoxelConfig::FormatVersion);
    Binary::writeU8(out, VoxelConfig::ChunkSize);
    Binary::writeU8(out, chunksPerAxis_);
    Binary::writeU8(out, WorldFlagsNone);
    Binary::writeU32LE(out, static_cast<std::uint32_t>(chunks_.size()));
    Binary::writeU32LE(out, static_cast<std::uint32_t>(tableOffset));
    Binary::writeU32LE(out, static_cast<std::uint32_t>(dataOffset));
    Binary::writeU32LE(out, skyRegion_);
    Binary::writeU32LE(out, missionNumber_);
    for (std::uint32_t i = 0; i < WorldHeaderReservedFieldCount; ++i) {
        Binary::writeU32LE(out, 0); // reserved
    }

    for (const ChunkBlob& blob : blobs) {
        writeChunkTableEntry(out, blob.entry);
    }

    for (const ChunkBlob& blob : blobs) {
        if (!blob.data.empty()) {
            out.write(blob.data.data(), static_cast<std::streamsize>(blob.data.size()));
            if (!out) {
                throw std::runtime_error("Failed to write chunk data");
            }
        }
    }
}

World World::loadFromFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Cannot open world file for reading");
    }

    char magic[4]{};
    Binary::readMagic(in, magic);
    if (std::memcmp(magic, WorldMagic, 4) != 0) {
        throw std::runtime_error("Invalid world magic, expected VWR1");
    }

    const std::uint8_t version = Binary::readU8(in);
    const std::uint8_t chunkSize = Binary::readU8(in);
    const std::uint8_t chunksPerAxis = Binary::readU8(in);
    const std::uint8_t flags = Binary::readU8(in);
    const std::uint32_t chunkCount = Binary::readU32LE(in);
    const std::uint32_t tableOffset = Binary::readU32LE(in);
    static_cast<void>(Binary::readU32LE(in)); // dataOffset, informational
    const std::uint32_t skyRegion = Binary::readU32LE(in);
    const std::uint32_t missionNumber = Binary::readU32LE(in);
    for (std::uint32_t i = 0; i < WorldHeaderReservedFieldCount; ++i) {
        static_cast<void>(Binary::readU32LE(in)); // reserved
    }

    if (version != VoxelConfig::FormatVersion) {
        // Breaking change (2026-07-09, header v1 -> v2): v1 files (written
        // before skyRegion existed) are rejected outright, not silently
        // upgraded -- see World Format.md.
        throw std::runtime_error("Unsupported world format version");
    }
    if (chunkSize != VoxelConfig::ChunkSize) {
        throw std::runtime_error("Unsupported chunk size in world file");
    }
    if (flags != WorldFlagsNone) {
        throw std::runtime_error("Unsupported world flags");
    }

    const std::uint32_t expectedChunkCount = static_cast<std::uint32_t>(chunksPerAxis) * chunksPerAxis * chunksPerAxis;
    if (chunkCount != expectedChunkCount) {
        throw std::runtime_error("World chunk count does not match chunksPerAxis^3");
    }

    World world(chunksPerAxis);
    world.skyRegion_ = skyRegion;
    world.missionNumber_ = missionNumber;

    in.seekg(tableOffset, std::ios::beg);
    if (!in) {
        throw std::runtime_error("Cannot seek to chunk table");
    }

    std::vector<SerializedChunkEntry> table;
    table.reserve(chunkCount);
    for (std::uint32_t i = 0; i < chunkCount; ++i) {
        table.push_back(readChunkTableEntry(in));
    }

    for (const SerializedChunkEntry& entry : table) {
        if (entry.x >= chunksPerAxis || entry.y >= chunksPerAxis || entry.z >= chunksPerAxis) {
            throw std::runtime_error("Chunk table contains out-of-range chunk position");
        }

        if ((entry.flags & ChunkTableFlagEmpty) != 0) {
            world.chunk(static_cast<std::uint8_t>(entry.x),
                        static_cast<std::uint8_t>(entry.y),
                        static_cast<std::uint8_t>(entry.z)) = Chunk::createUniform(Block::air());
            continue;
        }

        if (entry.size == 0) {
            throw std::runtime_error("Non-empty chunk table entry has zero size");
        }

        in.seekg(static_cast<std::streamoff>(entry.offset), std::ios::beg);
        if (!in) {
            throw std::runtime_error("Cannot seek to chunk data");
        }

        std::string data(entry.size, '\0');
        in.read(data.data(), static_cast<std::streamsize>(entry.size));
        if (!in) {
            throw std::runtime_error("Cannot read chunk data");
        }

        world.chunk(static_cast<std::uint8_t>(entry.x),
                    static_cast<std::uint8_t>(entry.y),
                    static_cast<std::uint8_t>(entry.z)) = deserializeChunk(data);
    }

    return world;
}

std::size_t World::chunkLinearIndex(std::uint8_t chunkX,
                                    std::uint8_t chunkY,
                                    std::uint8_t chunkZ) const {
    return static_cast<std::size_t>(chunkX)
        + static_cast<std::size_t>(chunkY) * chunksPerAxis_
        + static_cast<std::size_t>(chunkZ) * chunksPerAxis_ * chunksPerAxis_;
}

void World::validateBlockPosition(std::uint16_t x,
                                  std::uint16_t y,
                                  std::uint16_t z) const {
    const std::uint16_t limit = blocksPerAxis();
    if (x >= limit || y >= limit || z >= limit) {
        throw std::out_of_range("World block position is outside of world bounds");
    }
}

void World::validateChunkPosition(std::uint8_t chunkX,
                                  std::uint8_t chunkY,
                                  std::uint8_t chunkZ) const {
    if (chunkX >= chunksPerAxis_ || chunkY >= chunksPerAxis_ || chunkZ >= chunksPerAxis_) {
        throw std::out_of_range("Chunk position is outside of world bounds");
    }
}

} // namespace GalaxyEggbert::Worlds
