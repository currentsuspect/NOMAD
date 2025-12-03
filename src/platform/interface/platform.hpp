/**
 * @file platform.hpp
 * @brief Platform abstraction interfaces for Nomad DAW
 * @author Nomad Team
 * @date 2025
 * 
 * This file defines the core platform abstraction interfaces.
 * Each platform (Windows, macOS, Linux) provides implementations.
 */

#pragma once

#include "../../core/base/types.hpp"
#include "../../core/base/config.hpp"

#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <functional>
#include <filesystem>

namespace nomad::platform {

//=============================================================================
// Platform Information
//=============================================================================

/**
 * @brief Operating system type
 */
enum class OSType : u8 {
    Unknown,
    Windows,
    MacOS,
    Linux
};

/**
 * @brief CPU architecture
 */
enum class Architecture : u8 {
    Unknown,
    x86,
    x64,
    ARM,
    ARM64
};

/**
 * @brief Platform capabilities and information
 */
struct PlatformInfo {
    OSType os = OSType::Unknown;
    Architecture arch = Architecture::Unknown;
    std::string osVersion;
    std::string cpuName;
    u32 numLogicalCores = 0;
    u32 numPhysicalCores = 0;
    u64 totalMemory = 0;         ///< Total RAM in bytes
    u64 availableMemory = 0;     ///< Available RAM in bytes
    bool hasSSE = false;
    bool hasSSE2 = false;
    bool hasSSE3 = false;
    bool hasSSE41 = false;
    bool hasSSE42 = false;
    bool hasAVX = false;
    bool hasAVX2 = false;
    bool hasAVX512 = false;
    bool hasNeon = false;        ///< ARM SIMD
};

/**
 * @brief Get current platform information
 */
[[nodiscard]] PlatformInfo getPlatformInfo();

/**
 * @brief Get current OS type at compile time
 */
[[nodiscard]] constexpr OSType getCurrentOS() noexcept {
#if defined(NOMAD_PLATFORM_WINDOWS)
    return OSType::Windows;
#elif defined(NOMAD_PLATFORM_MACOS)
    return OSType::MacOS;
#elif defined(NOMAD_PLATFORM_LINUX)
    return OSType::Linux;
#else
    return OSType::Unknown;
#endif
}

//=============================================================================
// File System Abstraction
//=============================================================================

/**
 * @brief Special folder locations
 */
enum class SpecialFolder : u8 {
    UserHome,           ///< User's home directory
    UserDocuments,      ///< User's documents folder
    UserMusic,          ///< User's music folder
    UserDesktop,        ///< User's desktop
    AppData,            ///< Application data (roaming on Windows)
    LocalAppData,       ///< Local application data
    Temp,               ///< Temporary files
    ProgramFiles,       ///< Program installation folder
    CommonPlugins       ///< System-wide VST plugins folder
};

/**
 * @brief File system interface
 */
class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    
    /**
     * @brief Get path to special folder
     */
    [[nodiscard]] virtual std::filesystem::path getSpecialFolder(SpecialFolder folder) const = 0;
    
    /**
     * @brief Get application data folder for Nomad
     */
    [[nodiscard]] virtual std::filesystem::path getAppDataFolder() const = 0;
    
    /**
     * @brief Get user presets folder
     */
    [[nodiscard]] virtual std::filesystem::path getUserPresetsFolder() const = 0;
    
    /**
     * @brief Create directory (and parents if needed)
     */
    virtual bool createDirectory(const std::filesystem::path& path) const = 0;
    
    /**
     * @brief Check if path exists
     */
    [[nodiscard]] virtual bool exists(const std::filesystem::path& path) const = 0;
    
    /**
     * @brief List files in directory
     */
    [[nodiscard]] virtual std::vector<std::filesystem::path> listDirectory(
        const std::filesystem::path& path,
        std::string_view extension = ""
    ) const = 0;
    
    /**
     * @brief Show native file open dialog
     * @param title Dialog title
     * @param filters File type filters (e.g., {"Audio Files", "*.wav;*.mp3"})
     * @param initialPath Starting directory
     * @return Selected file path, or empty if cancelled
     */
    [[nodiscard]] virtual std::filesystem::path showOpenDialog(
        std::string_view title,
        const std::vector<std::pair<std::string, std::string>>& filters,
        const std::filesystem::path& initialPath = {}
    ) const = 0;
    
    /**
     * @brief Show native file save dialog
     */
    [[nodiscard]] virtual std::filesystem::path showSaveDialog(
        std::string_view title,
        const std::vector<std::pair<std::string, std::string>>& filters,
        const std::filesystem::path& initialPath = {},
        std::string_view defaultName = ""
    ) const = 0;
    
    /**
     * @brief Show native folder selection dialog
     */
    [[nodiscard]] virtual std::filesystem::path showFolderDialog(
        std::string_view title,
        const std::filesystem::path& initialPath = {}
    ) const = 0;
};

//=============================================================================
// Threading Abstraction
//=============================================================================

/**
 * @brief Thread priority levels
 */
enum class ThreadPriority : u8 {
    Lowest,
    BelowNormal,
    Normal,
    AboveNormal,
    High,
    Realtime       ///< Audio thread priority
};

/**
 * @brief Threading utilities interface
 */
class IThreading {
public:
    virtual ~IThreading() = default;
    
    /**
     * @brief Set current thread priority
     */
    virtual bool setThreadPriority(ThreadPriority priority) = 0;
    
    /**
     * @brief Set thread CPU affinity
     * @param cpuMask Bitmask of allowed CPUs
     */
    virtual bool setThreadAffinity(u64 cpuMask) = 0;
    
    /**
     * @brief Set thread name (for debugging)
     */
    virtual void setThreadName(std::string_view name) = 0;
    
    /**
     * @brief Get current thread ID
     */
    [[nodiscard]] virtual u64 getCurrentThreadId() const = 0;
    
    /**
     * @brief High-precision sleep
     * @param microseconds Sleep duration
     */
    virtual void sleepMicroseconds(u64 microseconds) const = 0;
    
    /**
     * @brief Yield CPU to other threads
     */
    virtual void yield() const = 0;
};

//=============================================================================
// Timer Abstraction
//=============================================================================

/**
 * @brief High-resolution timer interface
 */
class ITimer {
public:
    virtual ~ITimer() = default;
    
    /**
     * @brief Get current time in nanoseconds
     */
    [[nodiscard]] virtual u64 getNanoseconds() const = 0;
    
    /**
     * @brief Get current time in microseconds
     */
    [[nodiscard]] virtual u64 getMicroseconds() const = 0;
    
    /**
     * @brief Get current time in milliseconds
     */
    [[nodiscard]] virtual u64 getMilliseconds() const = 0;
    
    /**
     * @brief Get timer frequency (ticks per second)
     */
    [[nodiscard]] virtual u64 getFrequency() const = 0;
};

//=============================================================================
// Clipboard Interface
//=============================================================================

/**
 * @brief Clipboard interface
 */
class IClipboard {
public:
    virtual ~IClipboard() = default;
    
    /**
     * @brief Copy text to clipboard
     */
    virtual bool setText(std::string_view text) = 0;
    
    /**
     * @brief Get text from clipboard
     */
    [[nodiscard]] virtual std::string getText() const = 0;
    
    /**
     * @brief Check if clipboard has text
     */
    [[nodiscard]] virtual bool hasText() const = 0;
    
    /**
     * @brief Clear clipboard
     */
    virtual void clear() = 0;
};

//=============================================================================
// Platform Factory
//=============================================================================

/**
 * @brief Platform services factory
 * 
 * Creates platform-specific implementations of all interfaces.
 */
class IPlatform {
public:
    virtual ~IPlatform() = default;
    
    /**
     * @brief Initialize platform services
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief Shutdown platform services
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief Get file system interface
     */
    [[nodiscard]] virtual IFileSystem& getFileSystem() = 0;
    
    /**
     * @brief Get threading interface
     */
    [[nodiscard]] virtual IThreading& getThreading() = 0;
    
    /**
     * @brief Get timer interface
     */
    [[nodiscard]] virtual ITimer& getTimer() = 0;
    
    /**
     * @brief Get clipboard interface
     */
    [[nodiscard]] virtual IClipboard& getClipboard() = 0;
    
    /**
     * @brief Get platform information
     */
    [[nodiscard]] virtual const PlatformInfo& getInfo() const = 0;
    
    /**
     * @brief Show native message box
     */
    virtual void showMessageBox(
        std::string_view title,
        std::string_view message,
        bool isError = false
    ) = 0;
    
    /**
     * @brief Open URL in default browser
     */
    virtual void openURL(std::string_view url) = 0;
    
    /**
     * @brief Reveal file in file explorer
     */
    virtual void revealInExplorer(const std::filesystem::path& path) = 0;
};

/**
 * @brief Create platform-specific implementation
 */
[[nodiscard]] std::unique_ptr<IPlatform> createPlatform();

} // namespace nomad::platform
