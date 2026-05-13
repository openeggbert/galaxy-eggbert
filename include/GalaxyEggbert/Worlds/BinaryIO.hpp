#pragma once

#include <cstdint>
#include <iosfwd>

/**
 * @file
 * @brief Helpers for reading and writing fixed-size binary values in little-endian order.
 */

namespace GalaxyEggbert::Worlds::Binary {

/**
 * @brief Writes one unsigned 8-bit value to a binary stream.
 *
 * @param out Target output stream.
 * @param value Byte value to write.
 * @throws std::runtime_error If the write operation fails.
 */
void writeU8(std::ostream& out, std::uint8_t value);

/**
 * @brief Writes one unsigned 16-bit value in little-endian byte order.
 *
 * @param out Target output stream.
 * @param value Value to write.
 * @throws std::runtime_error If the write operation fails.
 */
void writeU16LE(std::ostream& out, std::uint16_t value);

/**
 * @brief Writes one unsigned 32-bit value in little-endian byte order.
 *
 * @param out Target output stream.
 * @param value Value to write.
 * @throws std::runtime_error If the write operation fails.
 */
void writeU32LE(std::ostream& out, std::uint32_t value);

/**
 * @brief Writes one unsigned 64-bit value in little-endian byte order.
 *
 * @param out Target output stream.
 * @param value Value to write.
 * @throws std::runtime_error If the write operation fails.
 */
void writeU64LE(std::ostream& out, std::uint64_t value);

/**
 * @brief Reads one unsigned 8-bit value from a binary stream.
 *
 * @param in Source input stream.
 * @return The decoded byte value.
 * @throws std::runtime_error If end-of-stream is reached before reading one byte.
 */
std::uint8_t readU8(std::istream& in);

/**
 * @brief Reads one unsigned 16-bit value encoded in little-endian byte order.
 *
 * @param in Source input stream.
 * @return The decoded 16-bit value.
 * @throws std::runtime_error If end-of-stream is reached before all bytes are read.
 */
std::uint16_t readU16LE(std::istream& in);

/**
 * @brief Reads one unsigned 32-bit value encoded in little-endian byte order.
 *
 * @param in Source input stream.
 * @return The decoded 32-bit value.
 * @throws std::runtime_error If end-of-stream is reached before all bytes are read.
 */
std::uint32_t readU32LE(std::istream& in);

/**
 * @brief Reads one unsigned 64-bit value encoded in little-endian byte order.
 *
 * @param in Source input stream.
 * @return The decoded 64-bit value.
 * @throws std::runtime_error If end-of-stream is reached before all bytes are read.
 */
std::uint64_t readU64LE(std::istream& in);

/**
 * @brief Writes a fixed four-byte magic signature.
 *
 * @param out Target output stream.
 * @param magic Pointer to a 4-byte character array containing the signature.
 * @throws std::runtime_error If the write operation fails.
 */
void writeMagic(std::ostream& out, const char magic[4]);

/**
 * @brief Reads a fixed four-byte magic signature.
 *
 * @param in Source input stream.
 * @param magic Output buffer of 4 bytes that receives the signature.
 * @throws std::runtime_error If fewer than 4 bytes can be read.
 */
void readMagic(std::istream& in, char magic[4]);

} // namespace GalaxyEggbert::Worlds::Binary
