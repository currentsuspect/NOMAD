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
     * @brief Fill buffer with cryptographically secure random bytes
     * @param buffer Pointer to buffer to fill
     * @param size Number of bytes to generate
     * @throws std::runtime_error if secure random generation fails
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
     * @brief Generate a single secure random 32-bit value
     */
    static u32 generateU32() {
        u32 value;
        generateBytes(reinterpret_cast<u8*>(&value), sizeof(value));
        return value;
    }
    
    /**
     * @brief Generate a single secure random 64-bit value
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
     * @brief Construct a nil UUID (all zeros)
     */
    UUID() noexcept : m_bytes{} {}
    
    /**
     * @brief Construct UUID from byte array
     */
    explicit UUID(const ByteArray& bytes) noexcept : m_bytes(bytes) {}
    
    /**
     * @brief Generate a new random UUID v4
     * 
     * @security Uses CSPRNG for all random bytes. Safe for security-sensitive
     *           use cases like session tokens and resource identifiers.
     * 
     * @return New randomly generated UUID
     * @throws std::runtime_error if secure random generation fails
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
     * @brief Create a nil UUID (all zeros)
     */
    [[nodiscard]] static UUID nil() noexcept {
        return UUID();
    }
    
    /**
     * @brief Parse UUID from string
     * @param str String in format "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
     * @return Parsed UUID
     * @throws std::invalid_argument if string format is invalid
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
     * @brief Convert UUID to string
     * @return String in format "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
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
     * @brief Check if this is a nil UUID
     */
    [[nodiscard]] bool isNil() const noexcept {
        for (const auto& byte : m_bytes) {
            if (byte != 0) return false;
        }
        return true;
    }
    
    /**
     * @brief Get the UUID version (should be 4 for random UUIDs)
     */
    [[nodiscard]] u8 version() const noexcept {
        return (m_bytes[6] >> 4) & 0x0F;
    }
    
    /**
     * @brief Get raw bytes
     */
    [[nodiscard]] const ByteArray& bytes() const noexcept {
        return m_bytes;
    }
    
    // Comparison operators
    [[nodiscard]] bool operator==(const UUID& other) const noexcept {
        return m_bytes == other.m_bytes;
    }
    
    [[nodiscard]] bool operator!=(const UUID& other) const noexcept {
        return m_bytes != other.m_bytes;
    }
    
    [[nodiscard]] bool operator<(const UUID& other) const noexcept {
        return m_bytes < other.m_bytes;
    }
    
    /**
     * @brief Hash support for use in unordered containers
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
    std::size_t operator()(const nomad::core::UUID& uuid) const noexcept {
        return uuid.hash();
    }
};
} // namespace std
