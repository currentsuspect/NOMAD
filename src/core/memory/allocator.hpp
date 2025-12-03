/**
 * @file allocator.hpp
 * @brief Memory allocator interfaces and implementations for Nomad DAW
 * @author Nomad Team
 * @date 2025
 * 
 * This file provides a hierarchical memory allocation system with:
 * - Abstract allocator interface
 * - System allocator (malloc/free wrapper)
 * - Aligned allocator for SIMD operations
 * - Tracking allocator for debugging
 */

#pragma once

#include "../base/types.hpp"
#include "../base/config.hpp"
#include "../base/assert.hpp"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <atomic>
#include <new>
#include <mutex>
#include <unordered_map>
#include <algorithm>
#include <iostream>

#ifndef LOG_DEBUG
#define LOG_DEBUG std::clog
#endif
#ifndef LOG_ERROR
#define LOG_ERROR std::cerr
#endif

namespace nomad::memory {

//=============================================================================
// Allocator Interface
//=============================================================================

/**
 * @brief Abstract base class for all allocators
 * 
 * Provides a common interface for memory allocation strategies.
 * All allocators must implement allocate() and deallocate().
 */
class IAllocator {
public:
    virtual ~IAllocator() = default;

    /**
     * @brief Allocate memory
     * @param size Number of bytes to allocate
     * @param alignment Required alignment (must be power of 2)
     * @return Pointer to allocated memory, or nullptr on failure
     */
    [[nodiscard]] virtual void* allocate(usize size, usize alignment = alignof(std::max_align_t)) = 0;

    /**
     * @brief Deallocate memory
     * @param ptr Pointer to memory to free (must have been allocated by this allocator)
     */
    virtual void deallocate(void* ptr) = 0;

    /**
     * @brief Reallocate memory to a new size
     * @param ptr Pointer to existing memory (or nullptr for new allocation)
     * @param oldSize Previous allocation size (required to copy existing data)
     * @param newSize New size in bytes
     * @param alignment Required alignment
     * @return Pointer to reallocated memory, or nullptr on failure
     * 
     * @note The default implementation copies min(oldSize, newSize) bytes from
     *       the old allocation to the new one. Derived classes should override
     *       for better performance if they track allocation sizes internally.
     */
    [[nodiscard]] virtual void* reallocate(void* ptr, usize oldSize, usize newSize, 
                                           usize alignment = alignof(std::max_align_t)) {
        if (ptr == nullptr) {
            return allocate(newSize, alignment);
        }
        if (newSize == 0) {
            deallocate(ptr);
            return nullptr;
        }
        
        // Allocate new block
        void* newPtr = allocate(newSize, alignment);
        if (newPtr) {
            // Copy the smaller of old/new sizes to preserve existing data
            std::memcpy(newPtr, ptr, oldSize < newSize ? oldSize : newSize);
            deallocate(ptr);
        }
        return newPtr;
    }

    /// Get the name of this allocator (for debugging)
    [[nodiscard]] virtual const char* getName() const = 0;

    /// Get current allocation statistics
    struct Stats {
        usize totalAllocations = 0;
        usize totalDeallocations = 0;
        usize currentAllocations = 0;
        usize currentBytes = 0;
        usize peakBytes = 0;
    };

    [[nodiscard]] virtual Stats getStats() const { return {}; }
};

//=============================================================================
// System Allocator
//=============================================================================

/**
 * @brief System allocator using malloc/free
 * 
 * A simple wrapper around the system's malloc/free functions.
 * Thread-safe and suitable for general-purpose allocations.
 */
class SystemAllocator : public IAllocator {
public:
    [[nodiscard]] void* allocate(usize size, usize alignment = alignof(std::max_align_t)) override {
        if (size == 0) return nullptr;
        
        void* ptr = nullptr;
        
#if defined(NOMAD_PLATFORM_WINDOWS)
        ptr = _aligned_malloc(size, alignment);
#else
        if (posix_memalign(&ptr, alignment, size) != 0) {
            ptr = nullptr;
        }
#endif
        
        if (ptr) {
            allocCount_.fetch_add(1, std::memory_order_relaxed);
            allocBytes_.fetch_add(size, std::memory_order_relaxed);
        }
        
        return ptr;
    }

    void deallocate(void* ptr) override {
        if (ptr == nullptr) return;
        
#if defined(NOMAD_PLATFORM_WINDOWS)
        _aligned_free(ptr);
#else
        free(ptr);
#endif
        
        deallocCount_.fetch_add(1, std::memory_order_relaxed);
    }

    [[nodiscard]] const char* getName() const override {
        return "SystemAllocator";
    }

    [[nodiscard]] Stats getStats() const override {
        Stats stats;
        stats.totalAllocations = allocCount_.load(std::memory_order_relaxed);
        stats.totalDeallocations = deallocCount_.load(std::memory_order_relaxed);
        stats.currentAllocations = stats.totalAllocations - stats.totalDeallocations;
        stats.currentBytes = allocBytes_.load(std::memory_order_relaxed);
        return stats;
    }

    /// Get the global system allocator instance
    static SystemAllocator& instance() {
        static SystemAllocator allocator;
        return allocator;
    }

private:
    std::atomic<usize> allocCount_{0};
    std::atomic<usize> deallocCount_{0};
    std::atomic<usize> allocBytes_{0};
};

//=============================================================================
// Aligned Allocator
//=============================================================================

/**
 * @brief Allocator with configurable default alignment
 * 
 * Useful for SIMD operations requiring specific alignment (16, 32, 64 bytes).
 */
template<usize DefaultAlignment = 64>
class AlignedAllocator : public IAllocator {
    static_assert((DefaultAlignment & (DefaultAlignment - 1)) == 0, 
                  "Alignment must be a power of 2");
    static_assert(DefaultAlignment >= alignof(std::max_align_t),
                  "Alignment must be at least max_align_t");

public:
    [[nodiscard]] void* allocate(usize size, usize alignment = DefaultAlignment) override {
        return SystemAllocator::instance().allocate(size, alignment);
    }

    void deallocate(void* ptr) override {
        SystemAllocator::instance().deallocate(ptr);
    }

    [[nodiscard]] const char* getName() const override {
        return "AlignedAllocator";
    }
};

/// Common aligned allocators
using Allocator16 = AlignedAllocator<16>;   // SSE alignment
using Allocator32 = AlignedAllocator<32>;   // AVX alignment
using Allocator64 = AlignedAllocator<64>;   // Cache line alignment

//=============================================================================
// Tracking Allocator (Debug)
//=============================================================================

#ifdef NOMAD_BUILD_DEBUG

/**
 * @brief Allocator wrapper that tracks allocations for leak detection
 * 
 * Wraps another allocator and maintains detailed allocation records.
 * Only active in debug builds.
 */
class TrackingAllocator : public IAllocator {
public:
    explicit TrackingAllocator(IAllocator& backing = SystemAllocator::instance())
        : backing_(backing) {}

    [[nodiscard]] void* allocate(usize size, usize alignment = alignof(std::max_align_t)) override {
        void* ptr = backing_.allocate(size, alignment);
        if (ptr) {
            std::lock_guard<std::mutex> lock(mutex_);
            allocations_[ptr] = {size, alignment};
            totalAllocated_ += size;
            peakAllocated_ = std::max(peakAllocated_, totalAllocated_);
        }
        return ptr;
    }

    void deallocate(void* ptr) override {
        if (ptr == nullptr) return;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = allocations_.find(ptr);
            if (it != allocations_.end()) {
                totalAllocated_ -= it->second.size;
                allocations_.erase(it);
            } else {
                NOMAD_ASSERT_MSG(false, "Deallocating untracked pointer!");
            }
        }
        
        backing_.deallocate(ptr);
    }

    [[nodiscard]] const char* getName() const override {
        return "TrackingAllocator";
    }

    [[nodiscard]] Stats getStats() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        Stats stats;
        stats.currentAllocations = allocations_.size();
        stats.currentBytes = totalAllocated_;
        stats.peakBytes = peakAllocated_;
        return stats;
    }

    /// Check for memory leaks (call at shutdown)
    void reportLeaks() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (allocations_.empty()) {
            LOG_DEBUG << "TrackingAllocator: No memory leaks detected";
            return;
        }
        
        LOG_ERROR << "TrackingAllocator: " << allocations_.size() << " memory leaks detected!";
        for (const auto& [ptr, info] : allocations_) {
            LOG_ERROR << "  Leak: " << ptr << " (" << info.size << " bytes)";
        }
    }

private:
    struct AllocationInfo {
        usize size;
        usize alignment;
    };

    IAllocator& backing_;
    mutable std::mutex mutex_;
    std::unordered_map<void*, AllocationInfo> allocations_;
    usize totalAllocated_ = 0;
    usize peakAllocated_ = 0;
};

#endif // NOMAD_BUILD_DEBUG

//=============================================================================
// Global Allocator Access
//=============================================================================

/// Get the default allocator for general use
inline IAllocator& getDefaultAllocator() {
    return SystemAllocator::instance();
}

/// Get the allocator optimized for audio processing (cache-aligned)
inline IAllocator& getAudioAllocator() {
    static Allocator64 allocator;
    return allocator;
}

//=============================================================================
// Smart Pointer Helpers
//=============================================================================

/// Deleter that uses a specific allocator
template<typename T>
struct AllocatorDeleter {
    IAllocator* allocator;

    void operator()(T* ptr) const {
        if (ptr) {
            ptr->~T();
            allocator->deallocate(ptr);
        }
    }
};

/// Create a unique_ptr using a custom allocator
template<typename T, typename... Args>
[[nodiscard]] std::unique_ptr<T, AllocatorDeleter<T>> 
makeUnique(IAllocator& allocator, Args&&... args) {
    void* mem = allocator.allocate(sizeof(T), alignof(T));
    if (!mem) return nullptr;
    
    T* obj = new (mem) T(std::forward<Args>(args)...);
    return std::unique_ptr<T, AllocatorDeleter<T>>(obj, {&allocator});
}

/// Allocate and construct an array using a custom allocator
template<typename T>
[[nodiscard]] T* allocateArray(IAllocator& allocator, usize count) {
    void* mem = allocator.allocate(sizeof(T) * count, alignof(T));
    if (!mem) return nullptr;
    
    T* arr = static_cast<T*>(mem);
    for (usize i = 0; i < count; ++i) {
        new (&arr[i]) T();
    }
    return arr;
}

/// Deallocate an array
template<typename T>
void deallocateArray(IAllocator& allocator, T* arr, usize count) {
    if (!arr) return;
    
    for (usize i = 0; i < count; ++i) {
        arr[i].~T();
    }
    allocator.deallocate(arr);
}

} // namespace nomad::memory
