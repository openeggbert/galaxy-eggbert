#pragma once

#include <cstdint>
#include <iosfwd>

namespace GalaxyEggbert::Worlds::Binary {

void writeU8(std::ostream& out, std::uint8_t value);
void writeU16LE(std::ostream& out, std::uint16_t value);
void writeU32LE(std::ostream& out, std::uint32_t value);
void writeU64LE(std::ostream& out, std::uint64_t value);

std::uint8_t readU8(std::istream& in);
std::uint16_t readU16LE(std::istream& in);
std::uint32_t readU32LE(std::istream& in);
std::uint64_t readU64LE(std::istream& in);

void writeMagic(std::ostream& out, const char magic[4]);
void readMagic(std::istream& in, char magic[4]);

} // namespace GalaxyEggbert::Worlds::binary
