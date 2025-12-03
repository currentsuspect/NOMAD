// © 2025 Nomad Studios — All Rights Reserved. Licensed for personal & educational use only.
#include "SamplePool.h"
#include "NomadLog.h"

#include <algorithm>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

namespace Nomad {
namespace Audio {

/**
 * @brief Accesses the global SamplePool singleton.
 *
 * @return Reference to the single shared SamplePool instance.
 */
SamplePool& SamplePool::getInstance() {
    static SamplePool instance;
    return instance;
}

/**
 * @brief Create a SampleKey for identifying a sample file.
 *
 * Constructs a key using the file's absolute path and the file's last-write timestamp.
 *
 * @param path File path (may be relative or absolute); used to produce the key's filePath.
 * @return SampleKey Contains `filePath` set to the absolute path when available and `modTime`
 *         set to the file's last-write time (duration count since epoch). If the file cannot be
 *         stat'ed, `filePath` will be the original `path` and `modTime` will be `0`.
 */
SampleKey SamplePool::makeKey(const std::string& path) {
    SampleKey key;
    try {
        fs::path p = fs::absolute(path);
        key.filePath = p.string();
        auto mod = fs::last_write_time(p);
        key.modTime = static_cast<uint64_t>(mod.time_since_epoch().count());
    } catch (const std::exception& e) {
        key.filePath = path; // Fallback to raw path
        key.modTime = 0;
        Log::warning(std::string("SamplePool: failed to stat file '") + path + "': " + e.what());
    }
    return key;
}

/**
 * @brief Computes the memory footprint of an audio buffer's sample data in bytes.
 *
 * @param buffer AudioBuffer whose contiguous float sample data will be measured.
 * @return size_t Number of bytes occupied by buffer.data (number of floats multiplied by sizeof(float)).
 */
size_t SamplePool::calculateBufferBytes(const AudioBuffer& buffer) {
    return buffer.data.size() * sizeof(float);
}

/**
 * @brief Recomputes the current memory usage of cached audio buffers and stores the result.
 *
 * Recalculates total bytes held by all live (non-expired) samples tracked in the pool and updates
 * the atomic m_memoryCurrent with the new total.
 *
 * @note Caller must hold the pool mutex (function is the locked variant).
 */
void SamplePool::updateMemoryUsageLocked() {
    size_t total = 0;
    for (const auto& [_, weakBuf] : m_samples) {
        if (auto buf = weakBuf.lock()) {
            total += calculateBufferBytes(*buf);
        }
    }
    m_memoryCurrent.store(total);
}

/**
 * @brief Obtain a cached audio buffer for a file path, loading and caching it if missing.
 *
 * Looks up a sample by file-derived key and returns an existing buffer when available.
 * If the sample is not cached, attempts to load it using the provided loader, inserts the
 * loaded buffer into the cache, updates memory accounting, and may evict other entries
 * to satisfy the memory budget. The buffer's last-access timestamp is updated on use.
 *
 * @param path Filesystem path identifying the sample.
 * @param loader Callable that fills an AudioBuffer and returns `true` on success. If empty, no load is attempted.
 * @return std::shared_ptr<AudioBuffer> The cached or newly loaded buffer, or `nullptr` if loading failed or no loader was provided.
 */
std::shared_ptr<AudioBuffer> SamplePool::acquire(
    const std::string& path,
    const std::function<bool(AudioBuffer&)>& loader) {
    
    SampleKey key = makeKey(path);

    // Fast path: try to find an existing buffer
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (auto it = m_samples.find(key); it != m_samples.end()) {
            if (auto existing = it->second.lock()) {
                existing->lastAccessTick.store(++m_accessCounter);
                return existing; // Cache hit
            }
            // Expired entry: clean it up
            m_samples.erase(it);
        }
    }

    // Miss: load the sample outside the lock
    if (!loader) {
        Log::warning("SamplePool: no loader provided for missing sample: " + path);
        return nullptr;
    }

    auto buffer = std::make_shared<AudioBuffer>();
    buffer->sourcePath = path;
    
    // Exception-safe loading
    try {
        if (!loader(*buffer)) {
            Log::warning("SamplePool: loader failed for " + path);
            return nullptr;
        }
    } catch (const std::exception& e) {
        Log::warning(std::string("SamplePool: loader exception for ") + path + ": " + e.what());
        return nullptr;
    }

    buffer->numFrames = (buffer->channels > 0) ? buffer->data.size() / buffer->channels : 0;
    buffer->ready.store(true);
    buffer->lastAccessTick.store(++m_accessCounter);

    // Insert into cache (re-check to prevent race)
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Race check: another thread may have loaded this while we were unlocked
    if (auto it = m_samples.find(key); it != m_samples.end()) {
        if (auto existing = it->second.lock()) {
            return existing; // Another thread won: use their buffer
        }
        m_samples.erase(it); // Expired: remove stale entry
    }
    
    m_samples.emplace(key, buffer);
    updateMemoryUsageLocked();
    garbageCollectLocked(); // Already locked: use private variant

    return buffer;
}

/**
 * @brief Remove expired samples and evict cached buffers to satisfy the memory budget.
 *
 * Performs a thread-safe garbage collection pass that removes expired cache entries
 * and, if a memory budget is set, evicts least-recently-used buffers until the
 * current memory usage is within the configured budget.
 *
 * This method is safe to call concurrently from multiple threads.
 */
void SamplePool::garbageCollect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    garbageCollectLocked();
}

/**
 * @brief Removes expired samples and evicts least-recently-used live samples until the memory usage fits the configured budget.
 *
 * Iterates the internal cache to remove entries whose weak references have expired, computes the total memory used by live samples, updates the current memory accounting, and—if a nonzero memory budget is set—evicts live samples in order of oldest last access until memory usage is at or below the budget.
 */
void SamplePool::garbageCollectLocked() {
    // Remove expired entries first
    for (auto it = m_samples.begin(); it != m_samples.end(); ) {
        if (it->second.expired()) {
            it = m_samples.erase(it);
        } else {
            ++it;
        }
    }

    if (m_memoryBudget == 0) {
        return; // Unlimited budget, skip calculation entirely
    }

    // Build live list AND calculate total size in one pass
    struct EntryInfo {
        SampleKey key;
        uint64_t lastTick;
        size_t sizeBytes;
    };
    std::vector<EntryInfo> live;
    live.reserve(m_samples.size());
    
    size_t totalBytes = 0;
    for (auto& [key, weakBuf] : m_samples) {
        if (auto buf = weakBuf.lock()) {
            size_t sz = calculateBufferBytes(*buf);
            live.push_back({key, buf->lastAccessTick.load(), sz});
            totalBytes += sz;
        }
    }

    m_memoryCurrent.store(totalBytes);

    if (m_memoryCurrent.load() <= m_memoryBudget) {
        return;
    }

    // Sort and evict (using pre-calculated sizes)
    std::sort(live.begin(), live.end(), [](const EntryInfo& a, const EntryInfo& b) {
        return a.lastTick < b.lastTick;
    });

    for (const auto& info : live) {
        if (m_memoryCurrent.load() <= m_memoryBudget) break;
        if (m_samples.erase(info.key) > 0) {
            m_memoryCurrent.fetch_sub(info.sizeBytes);
        }
    }
}

/**
 * @brief Update the pool's memory budget and immediately enforce it.
 *
 * Setting the budget will cause the pool to evict cached samples as needed so
 * that current memory usage does not exceed the new limit.
 *
 * @param bytes New memory budget in bytes; zero disables budgeting (no automatic evictions).
 */
void SamplePool::setMemoryBudget(size_t bytes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_memoryBudget = bytes;
    garbageCollectLocked();
}

} // namespace Audio
} // namespace Nomad