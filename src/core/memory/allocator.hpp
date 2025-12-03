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
    /**
 * @brief Ensures derived allocator destructors are invoked correctly.
 *
 * Provide a virtual destructor so implementations of IAllocator can clean up
 * resources when deleted through a base-class pointer.
 */
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
     * @brief Resize an allocation to a new size (or allocate/free based on inputs).
     *
     * The default behavior: if `ptr` is `nullptr` this allocates `newSize` bytes; if
     * `newSize` is zero this frees `ptr` and returns `nullptr`; otherwise it
     * allocates a new block with the requested `alignment`, copies `min(oldSize, newSize)`
     * bytes from the old block into the new block, frees the old block, and returns
     * the new pointer. Derived allocators may override to provide more efficient
     * resizing when they track allocation sizes.
     *
     * @param ptr Pointer to an existing allocation, or `nullptr` to allocate a new block.
     * @param oldSize Size in bytes of the existing allocation (used to determine how many bytes to copy).
     * @param newSize Desired size in bytes for the new allocation; if zero the function frees `ptr`.
     * @param alignment Alignment requirement for the new allocation.
     * @return void* Pointer to the resized (or newly allocated) memory, or `nullptr` on failure.
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

    /**
 * @brief Retrieve allocator statistics.
 *
 * Provides aggregated counters for allocations, deallocations, current active allocations,
 * current allocated bytes, and peak bytes.
 *
 * @return Stats Struct with allocation metrics; default implementation returns an empty
 * `Stats` (all fields zero). Derived allocators should override to return accurate values.
 */
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
    /**
     * @brief Allocates a block of memory with the requested size and alignment.
     *
     * @param size Number of bytes to allocate. If `size` is 0, no allocation is performed and `nullptr` is returned.
     * @param alignment Alignment in bytes for the returned block; defaults to `alignof(std::max_align_t)`.
     * @return void* Pointer to the allocated memory, or `nullptr` if allocation fails or `size` is 0.
     */
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

    /**
     * @brief Frees memory previously allocated by this allocator.
     *
     * Safe to call with `nullptr` (has no effect). Also increments the allocator's
     * internal deallocation counter used for statistics.
     *
     * @param ptr Pointer to the memory block to free.
     */
    void deallocate(void* ptr) override {
        if (ptr == nullptr) return;
        
#if defined(NOMAD_PLATFORM_WINDOWS)
        _aligned_free(ptr);
#else
        free(ptr);
#endif
        
        deallocCount_.fetch_add(1, std::memory_order_relaxed);
    }

    /**
     * @brief Return the allocator's human-readable name.
     *
     * @return const char* Null-terminated string identifying this allocator ("SystemAllocator").
     */
    [[nodiscard]] const char* getName() const override {
        return "SystemAllocator";
    }

    /**
     * @brief Retrieves allocation statistics for the system allocator.
     *
     * Provides totals for allocations and deallocations, computes current active allocations,
     * and reports the current number of allocated bytes. Peak bytes are not tracked by this allocator.
     *
     * @return Stats Structure containing `totalAllocations`, `totalDeallocations`, `currentAllocations`, and `currentBytes`.
     */
    [[nodiscard]] Stats getStats() const override {
        Stats stats;
        stats.totalAllocations = allocCount_.load(std::memory_order_relaxed);
        stats.totalDeallocations = deallocCount_.load(std::memory_order_relaxed);
        stats.currentAllocations = stats.totalAllocations - stats.totalDeallocations;
        stats.currentBytes = allocBytes_.load(std::memory_order_relaxed);
        return stats;
    }

    /**
     * @brief Accesses the global SystemAllocator singleton.
     *
     * The instance is constructed on first use and remains valid until program termination.
     *
     * @return SystemAllocator& Reference to the global SystemAllocator instance.
     */
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
    /**
     * @brief Allocates a block of memory with the requested size and alignment.
     *
     * @param size Number of bytes to allocate.
     * @param alignment Alignment in bytes; defaults to the allocator's DefaultAlignment.
     * @return void* Pointer to the allocated memory, or `nullptr` if allocation fails.
     */
    [[nodiscard]] void* allocate(usize size, usize alignment = DefaultAlignment) override {
        return SystemAllocator::instance().allocate(size, alignment);
    }

    /**
     * @brief Releases memory previously allocated by this allocator.
     *
     * Frees the block pointed to by `ptr`. If `ptr` is `nullptr`, no action is taken.
     *
     * @param ptr Pointer to a memory block returned by this allocator's `allocate`.
     */
    void deallocate(void* ptr) override {
        SystemAllocator::instance().deallocate(ptr);
    }

    /**
     * @brief Allocator identifier string for this allocator implementation.
     *
     * @return const char* The null-terminated string "AlignedAllocator".
     */
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
    /**
         * @brief Constructs a TrackingAllocator that wraps a backing allocator used for actual allocations.
         *
         * The TrackingAllocator will forward allocation and deallocation requests to the provided backing
         * allocator while tracking active allocations for debugging and leak detection.
         *
         * @param backing Reference to the underlying allocator to use (defaults to the global SystemAllocator instance).
         */
        explicit TrackingAllocator(IAllocator& backing = SystemAllocator::instance())
        : backing_(backing) {}

    /**
     * @brief Allocates memory via the backing allocator and records the allocation for leak tracking.
     *
     * Records the allocated pointer, size, and alignment in the tracking map and updates total and peak
     * allocated byte counters when allocation succeeds.
     *
     * @param size Number of bytes to allocate.
     * @param alignment Alignment requirement for the allocation.
     * @return void* Pointer to the allocated memory, or `nullptr` on failure.
     */
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

    /**
     * @brief Deallocates a previously allocated block and removes its tracking entry.
     *
     * @param ptr Pointer to the memory block to free; if `nullptr`, the call is a no-op.
     *
     * @note If `ptr` was not recorded by this TrackingAllocator, a debug assertion is triggered.
     */
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

    /**
     * @brief Provide the allocator's name for debugging and identification.
     *
     * @return const char* Pointer to a null-terminated string containing the allocator's name ("TrackingAllocator").
     */
    [[nodiscard]] const char* getName() const override {
        return "TrackingAllocator";
    }

    /**
     * @brief Provide a thread-safe snapshot of the allocator's current and peak allocation statistics.
     *
     * Only the following fields of the returned `Stats` are populated:
     * - `currentAllocations`: number of active tracked allocations.
     * - `currentBytes`: total bytes currently allocated and tracked.
     * - `peakBytes`: highest observed total allocated bytes.
     *
     * @return Stats Snapshot of the tracking allocator's current allocation counts and byte usage.
     */
    [[nodiscard]] Stats getStats() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        Stats stats;
        stats.currentAllocations = allocations_.size();
        stats.currentBytes = totalAllocated_;
        stats.peakBytes = peakAllocated_;
        return stats;
    }

    /**
     * @brief Logs any tracked allocations remaining in the allocator (intended to be called at shutdown).
     *
     * If no allocations remain, logs that no memory leaks were detected. If there are remaining
     * allocations, logs the total count and each leaked pointer with its recorded size.
     */
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

/**
 * @brief Global default allocator for general-purpose allocations.
 *
 * @return IAllocator& Reference to the singleton SystemAllocator used as the default allocator.
 */
inline IAllocator& getDefaultAllocator() {
    return SystemAllocator::instance();
}

/**
 * @brief Provides a process-wide allocator optimized for audio processing.
 *
 * Returns a reference to a function-local static Allocator64 configured for cache-aligned allocations suitable for audio workloads.
 *
 * @return IAllocator& Reference to the static cache-aligned Allocator64 instance.
 */
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

    /**
     * @brief Destroys the object pointed to by `ptr` and returns its memory to the associated allocator.
     *
     * @param ptr Pointer to the object to destroy; if `nullptr`, no action is taken.
     */
    void operator()(T* ptr) const {
        if (ptr) {
            ptr->~T();
            allocator->deallocate(ptr);
        }
    }
};

/// Create a unique_ptr using a custom allocator
template<typename T, typename... Args>
/**
 * @brief Constructs an object of type `T` in memory provided by a custom allocator and returns
 * a `std::unique_ptr` that will destroy and deallocate it via `AllocatorDeleter<T>`.
 *
 * @tparam T Type of the object to create.
 * @tparam Args Constructor argument types for `T`.
 * @param allocator Allocator used to obtain memory and later deallocate it.
 * @param args Arguments forwarded to `T`'s constructor.
 * @return std::unique_ptr<T, AllocatorDeleter<T>> Owning pointer to the constructed object, or
 * `nullptr` if the allocator failed to allocate memory.
 */
[[nodiscard]] std::unique_ptr<T, AllocatorDeleter<T>> 
makeUnique(IAllocator& allocator, Args&&... args) {
    void* mem = allocator.allocate(sizeof(T), alignof(T));
    if (!mem) return nullptr;
    
    T* obj = new (mem) T(std::forward<Args>(args)...);
    return std::unique_ptr<T, AllocatorDeleter<T>>(obj, {&allocator});
}

/// Allocate and construct an array using a custom allocator
template<typename T>
/**
 * @brief Allocates memory for and value-initializes an array of objects of type `T` using the given allocator.
 *
 * @tparam T Element type.
 * @param count Number of elements to allocate and construct.
 * @return T* Pointer to the first element of the allocated array, or `nullptr` if allocation fails.
 */
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
/**
 * @brief Destroys a sequence of objects and releases their memory via the provided allocator.
 *
 * If `arr` is null this function does nothing. Otherwise it calls the destructor of each of the
 * `count` elements at `arr` and then deallocates the memory using `allocator`.
 *
 * @tparam T Element type stored in the array.
 * @param allocator Allocator used to deallocate the memory holding the array.
 * @param arr Pointer to the first element of the array to destroy and deallocate.
 * @param count Number of elements to destroy.
 */
void deallocateArray(IAllocator& allocator, T* arr, usize count) {
    if (!arr) return;
    
    for (usize i = 0; i < count; ++i) {
        arr[i].~T();
    }
    allocator.deallocate(arr);
}

} // namespace nomad::memory