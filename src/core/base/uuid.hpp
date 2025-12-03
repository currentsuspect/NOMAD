/**
 * @file uuid.hpp
 * @brief UUID generation and handling for Nomad DAW
 * @author Nomad Team
 * @date 2025
 * 
 * This file provides UUID v4 generation using cryptographically secure
 * random number generation. UUIDs are used for identifying resources,
 * plugins, sessions, and other entities throughout the DAW.
 * 
 * @security UUIDs use OS-provided CSPRNG for all random bytes to prevent
 *           predictable identifier generation. Do not replace with weaker
 *           PRNGs like std::mt19937 without security review.
 */

#pragma once

#include "types.hpp"
#include "config.hpp"

#include <array>
#include <string>
#include <cstring>
#include <random>
#include <mutex>
#include <stdexcept>

#if defined(NOMAD_PLATFORM_WINDOWS)
    #include <windows.h>
    #include <bcrypt.h>
    #pragma comment(lib, "bcrypt.lib")
#elif defined(NOMAD_PLATFORM_MACOS) || defined(NOMAD_PLATFORM_LINUX)
    #include <fcntl.h>
    #include <unistd.h>
#endif

namespace nomad::core {

/**
 * @brief Cryptographically secure random byte generator
 * 
 * Uses platform-specific APIs to generate cryptographically secure random bytes:
 * - Windows: BCryptGenRandom (CNG API)
 * - macOS/Linux: /dev/urandom
 * 
 * @security This class is critical for UUID security. Always use OS entropy sources.
 */
class SecureRandom {
public:
    /**
     * @brief Fill a buffer with cryptographically secure random bytes.
     *
     * Fills the provided buffer with exactly `size` bytes of entropy sourced from
     * the platform's secure random provider. If a platform-specific secure source
     * is unavailable the implementation may fall back to a weaker generator.
     *
     * @param buffer Pointer to a writable buffer of at least `size` bytes.
     * @param size Number of bytes to write into `buffer`.
     * @throws std::runtime_error If a cryptographically secure random source cannot
     *         be used or read (e.g., OS API or /dev/urandom failures).
     */
    static void generateBytes(u8* buffer, usize size) {
#if defined(NOMAD_PLATFORM_WINDOWS)
        // Use BCrypt (Cryptography Next Generation) API
        NTSTATUS status = BCryptGenRandom(
            nullptr,                    // Use default algorithm
            buffer,
            static_cast<ULONG>(size),
            BCRYPT_USE_SYSTEM_PREFERRED_RNG
        );
        if (!BCRYPT_SUCCESS(status)) {
            throw std::runtime_error("BCryptGenRandom failed - secure random unavailable");
        }
#elif defined(NOMAD_PLATFORM_MACOS) || defined(NOMAD_PLATFORM_LINUX)
        // Use /dev/urandom for cryptographically secure random bytes
        // Note: /dev/urandom is safe for cryptographic use on modern systems
        // and won't block, unlike /dev/random
        int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
        if (fd < 0) {
            throw std::runtime_error("Failed to open /dev/urandom");
        }
        
        usize bytesRead = 0;
        while (bytesRead < size) {
            ssize_t result = read(fd, buffer + bytesRead, size - bytesRead);
            if (result < 0) {
                close(fd);
                throw std::runtime_error("Failed to read from /dev/urandom");
            }
            bytesRead += static_cast<usize>(result);
        }
        close(fd);
#else
        // Fallback: Use std::random_device directly for each byte
        // This is less ideal but works on platforms with hardware RNG support
        // WARNING: std::random_device may not be cryptographically secure on all platforms
        static std::mutex mtx;
        std::lock_guard<std::mutex> lock(mtx);
        std::random_device rd;
        
        for (usize i = 0; i < size; ++i) {
            buffer[i] = static_cast<u8>(rd() & 0xFF);
        }
#endif
    }
    
    /**
     * @brief Generates a cryptographically secure 32-bit random value.
     *
     * @return u32 A uniformly distributed 32-bit unsigned random value sourced from the platform secure RNG.
     */
    static u32 generateU32() {
        u32 value;
        generateBytes(reinterpret_cast<u8*>(&value), sizeof(value));
        return value;
    }
    
    /**
     * @brief Produce a cryptographically secure 64-bit random value.
     *
     * @return u64 A 64-bit unsigned integer populated with cryptographically secure random bits.
     */
    static u64 generateU64() {
        u64 value;
        generateBytes(reinterpret_cast<u8*>(&value), sizeof(value));
        return value;
    }
};

/**
 * @brief UUID (Universally Unique Identifier) v4
 * 
 * Represents a 128-bit UUID following RFC 4122 version 4 (random).
 * All random bits are generated using cryptographically secure RNG.
 * 
 * Memory layout: 16 bytes in big-endian format
 * String format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
 *   where x is any hex digit and y is one of 8, 9, a, or b
 */
class UUID {
public:
    static constexpr usize BYTE_SIZE = 16;
    static constexpr usize STRING_SIZE = 36; // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
    
    using ByteArray = std::array<u8, BYTE_SIZE>;

    /**
 * @brief Create a nil UUID with all bytes set to zero.
 */
    UUID() noexcept : m_bytes{} {}
    
    /**
 * @brief Initializes the UUID from the provided 16-byte array.
 *
 * No validation or normalization is performed; the bytes are copied directly into the UUID.
 *
 * @param bytes 16 bytes representing the UUID in RFC 4122 byte order.
 */
    explicit UUID(const ByteArray& bytes) noexcept : m_bytes(bytes) {}
    
    /**
     * @brief Create a version 4 (random) RFC 4122 UUID.
     *
     * Generates 16 cryptographically secure random bytes and returns a UUID
     * with the version and variant bits set according to RFC 4122.
     *
     * @return UUID A version 4 RFC 4122 UUID.
     * @throws std::runtime_error if secure random byte generation fails.
     */
    [[nodiscard]] static UUID generate() {
        ByteArray bytes;
        
        // Generate all 16 bytes using CSPRNG
        SecureRandom::generateBytes(bytes.data(), BYTE_SIZE);
        
        // Set version to 4 (random UUID)
        // Version is stored in the high nibble of byte 6
        bytes[6] = (bytes[6] & 0x0F) | 0x40;
        
        // Set variant to RFC 4122 (10xx xxxx)
        // Variant is stored in the high bits of byte 8
        bytes[8] = (bytes[8] & 0x3F) | 0x80;
        
        return UUID(bytes);
    }
    
    /**
     * @brief Create a nil UUID with all bytes set to zero.
     *
     * @return UUID whose 16 bytes are all zero.
     */
    [[nodiscard]] static UUID nil() noexcept {
        return UUID();
    }
    
    /**
     * @brief Parse a UUID from its canonical hex string representation with dashes.
     *
     * Parses a UUID string in the form "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" (hex digits may be upper- or lower-case)
     * and constructs the corresponding UUID.
     *
     * @param str UUID string to parse.
     * @return UUID The parsed UUID.
     * @throws std::invalid_argument if the string length, dash placement, or hex characters are invalid.
     */
    [[nodiscard]] static UUID fromString(std::string_view str) {
        if (str.size() != STRING_SIZE) {
            throw std::invalid_argument("Invalid UUID string length");
        }
        
        // Validate format: 8-4-4-4-12 with dashes
        if (str[8] != '-' || str[13] != '-' || str[18] != '-' || str[23] != '-') {
            throw std::invalid_argument("Invalid UUID string format");
        }
        
        ByteArray bytes;
        usize byteIndex = 0;
        
        for (usize i = 0; i < STRING_SIZE && byteIndex < BYTE_SIZE; ++i) {
            if (str[i] == '-') continue;
            
            auto hexToNibble = [](char c) -> u8 {
                if (c >= '0' && c <= '9') return static_cast<u8>(c - '0');
                if (c >= 'a' && c <= 'f') return static_cast<u8>(c - 'a' + 10);
                if (c >= 'A' && c <= 'F') return static_cast<u8>(c - 'A' + 10);
                throw std::invalid_argument("Invalid hex character in UUID");
            };
            
            u8 high = hexToNibble(str[i]);
            u8 low = hexToNibble(str[++i]);
            bytes[byteIndex++] = static_cast<u8>((high << 4) | low);
        }
        
        return UUID(bytes);
    }
    
    /**
     * @brief Format the UUID as a canonical lowercase hexadecimal string with dashes.
     *
     * Produces the canonical UUID representation: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
     * using lowercase hexadecimal digits.
     *
     * @return std::string UUID formatted as "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx".
     */
    [[nodiscard]] std::string toString() const {
        static constexpr char hexChars[] = "0123456789abcdef";
        std::string result;
        result.reserve(STRING_SIZE);
        
        for (usize i = 0; i < BYTE_SIZE; ++i) {
            if (i == 4 || i == 6 || i == 8 || i == 10) {
                result += '-';
            }
            result += hexChars[(m_bytes[i] >> 4) & 0x0F];
            result += hexChars[m_bytes[i] & 0x0F];
        }
        
        return result;
    }
    
    /**
     * @brief Determines whether the UUID is nil (all bytes are zero).
     *
     * @return true if all 16 UUID bytes are zero, false otherwise.
     */
    [[nodiscard]] bool isNil() const noexcept {
        for (const auto& byte : m_bytes) {
            if (byte != 0) return false;
        }
        return true;
    }
    
    /**
     * @brief Retrieve the UUID version encoded in this UUID.
     *
     * @return u8 The UUID version number (1–5). For RFC 4122 version 4 UUIDs this will be `4`.
     */
    [[nodiscard]] u8 version() const noexcept {
        return (m_bytes[6] >> 4) & 0x0F;
    }
    
    /**
     * @brief Access the underlying 16-byte UUID storage.
     *
     * @return const ByteArray& Reference to the internal array of 16 bytes representing the UUID.
     */
    [[nodiscard]] const ByteArray& bytes() const noexcept {
        return m_bytes;
    }
    
    /**
     * @brief Compare two UUIDs for equality.
     *
     * @returns `true` if the UUIDs contain identical bytes, `false` otherwise.
     */
    [[nodiscard]] bool operator==(const UUID& other) const noexcept {
        return m_bytes == other.m_bytes;
    }
    
    /**
     * @brief Checks whether this UUID is different from another UUID.
     *
     * @param other UUID to compare against.
     * @return `true` if this UUID differs from `other`, `false` otherwise.
     */
    [[nodiscard]] bool operator!=(const UUID& other) const noexcept {
        return m_bytes != other.m_bytes;
    }
    
    /**
     * @brief Compares this UUID to another using lexicographical ordering of the raw bytes.
     *
     * @param other UUID to compare against.
     * @return true if this UUID is lexicographically less than `other`, false otherwise.
     */
    [[nodiscard]] bool operator<(const UUID& other) const noexcept {
        return m_bytes < other.m_bytes;
    }
    
    /**
     * @brief Computes a hash value for the UUID suitable for use in unordered containers.
     *
     * The hash is computed over the UUID's 16 bytes using the FNV-1a algorithm.
     *
     * @return std::size_t Hash value derived from the UUID bytes.
     */
    [[nodiscard]] std::size_t hash() const noexcept {
        // FNV-1a hash
        std::size_t hash = 14695981039346656037ULL;
        for (const auto& byte : m_bytes) {
            hash ^= static_cast<std::size_t>(byte);
            hash *= 1099511628211ULL;
        }
        return hash;
    }

private:
    ByteArray m_bytes;
};

} // namespace nomad::core

// Standard library hash support
namespace std {
template <>
struct hash<nomad::core::UUID> {
    /**
     * @brief Computes a hash value for a UUID.
     *
     * @param uuid The UUID to hash.
     * @return std::size_t A hash value derived from the UUID's bytes.
     */
    std::size_t operator()(const nomad::core::UUID& uuid) const noexcept {
        return uuid.hash();
    }
};
} // namespace std