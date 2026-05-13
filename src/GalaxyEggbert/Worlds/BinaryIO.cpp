#include "GalaxyEggbert/Worlds/BinaryIO.hpp"

#include <istream>
#include <ostream>
#include <stdexcept>

namespace GalaxyEggbert::Worlds::Binary {

namespace {

void ensureRead(std::istream& in) {
    if (!in) {
        throw std::runtime_error("Unexpected end of binary stream");
    }
}

void ensureWrite(std::ostream& out) {
    if (!out) {
        throw std::runtime_error("Failed to write to binary stream");
    }
}

} // namespace

void writeU8(std::ostream& out, std::uint8_t value) {
    out.put(static_cast<char>(value));
    ensureWrite(out);
}

void writeU16LE(std::ostream& out, std::uint16_t value) {
    out.put(static_cast<char>(value & 0xFF));
    out.put(static_cast<char>((value >> 8) & 0xFF));
    ensureWrite(out);
}

void writeU32LE(std::ostream& out, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
        out.put(static_cast<char>((value >> shift) & 0xFF));
    }
    ensureWrite(out);
}

void writeU64LE(std::ostream& out, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8) {
        out.put(static_cast<char>((value >> shift) & 0xFF));
    }
    ensureWrite(out);
}

std::uint8_t readU8(std::istream& in) {
    const int value = in.get();
    if (value == std::char_traits<char>::eof()) {
        throw std::runtime_error("Unexpected end of binary stream while reading uint8");
    }
    return static_cast<std::uint8_t>(value);
}

std::uint16_t readU16LE(std::istream& in) {
    const std::uint16_t b0 = readU8(in);
    const std::uint16_t b1 = readU8(in);
    return static_cast<std::uint16_t>(b0 | (b1 << 8));
}

std::uint32_t readU32LE(std::istream& in) {
    std::uint32_t result = 0;
    for (int shift = 0; shift < 32; shift += 8) {
        result |= static_cast<std::uint32_t>(readU8(in)) << shift;
    }
    return result;
}

std::uint64_t readU64LE(std::istream& in) {
    std::uint64_t result = 0;
    for (int shift = 0; shift < 64; shift += 8) {
        result |= static_cast<std::uint64_t>(readU8(in)) << shift;
    }
    return result;
}

void writeMagic(std::ostream& out, const char magic[4]) {
    out.write(magic, 4);
    ensureWrite(out);
}

void readMagic(std::istream& in, char magic[4]) {
    in.read(magic, 4);
    ensureRead(in);
}

} // namespace GalaxyEggbert::Worlds::binary
