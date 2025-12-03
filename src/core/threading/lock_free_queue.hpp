/**
 * @file lock_free_queue.hpp
 * @brief Lock-free queue implementations for real-time audio
 * @author Nomad Team
 * @date 2025
 * 
 * This file provides lock-free data structures suitable for real-time audio:
 * - SPSCQueue: Single-Producer Single-Consumer queue
 * - MPSCQueue: Multi-Producer Single-Consumer queue
 * 
 * @security These queues enforce trivial copyability and trivial destructibility
 *           to prevent data races and ensure safe concurrent access. Do not store
 *           pointers to mutable shared data in these queues.
 */

#pragma once

#include "../base/types.hpp"
#include "../base/config.hpp"
#include "../base/assert.hpp"

#include <atomic>
#include <array>
#include <type_traits>
#include <new>
#include <optional>

namespace nomad::core {

/**
 * @brief Single-Producer Single-Consumer lock-free queue
 * 
 * A bounded, wait-free queue for communication between exactly one producer
 * and one consumer thread. Ideal for audio thread communication.
 * 
 * @tparam T Element type (must be trivially copyable and trivially destructible)
 * @tparam Capacity Maximum number of elements (must be power of 2)
 * 
 * @security T must be trivially copyable to ensure atomic-safe reads/writes.
 *           Do not store raw pointers to mutable shared data.
 */
template <typename T, usize Capacity>
class SPSCQueue {
    // Security: Enforce trivial copyability to prevent torn reads/writes
    static_assert(std::is_trivially_copyable_v<T>,
        "SPSCQueue<T>: T must be trivially copyable for safe lock-free operation. "
        "Storing non-trivial types may cause data races and undefined behavior.");
    
    static_assert(std::is_trivially_destructible_v<T>,
        "SPSCQueue<T>: T must be trivially destructible. "
        "Non-trivial destructors cannot be safely called in lock-free contexts.");
    
    static_assert(Capacity > 0 && (Capacity & (Capacity - 1)) == 0,
        "SPSCQueue: Capacity must be a power of 2");
    
    // Security: Warn about pointer types
    static_assert(!std::is_pointer_v<T>,
        "SPSCQueue<T>: Storing raw pointers is discouraged. "
        "Use value types or indices into a separate data structure.");

public:
    SPSCQueue() noexcept : m_head(0), m_tail(0) {}
    
    // Non-copyable, non-movable
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;
    SPSCQueue(SPSCQueue&&) = delete;
    SPSCQueue& operator=(SPSCQueue&&) = delete;
    
    /**
     * @brief Try to push an element (producer only)
     * @param value Element to push
     * @return true if successful, false if queue is full
     */
    [[nodiscard]] bool tryPush(const T& value) noexcept {
        const usize head = m_head.load(std::memory_order_relaxed);
        const usize nextHead = (head + 1) & MASK;
        
        if (nextHead == m_tail.load(std::memory_order_acquire)) {
            return false; // Queue is full
        }
        
        m_buffer[head] = value;
        m_head.store(nextHead, std::memory_order_release);
        return true;
    }
    
    /**
     * @brief Try to pop an element (consumer only)
     * @param value Output parameter for popped element
     * @return true if successful, false if queue is empty
     */
    [[nodiscard]] bool tryPop(T& value) noexcept {
        const usize tail = m_tail.load(std::memory_order_relaxed);
        
        if (tail == m_head.load(std::memory_order_acquire)) {
            return false; // Queue is empty
        }
        
        value = m_buffer[tail];
        m_tail.store((tail + 1) & MASK, std::memory_order_release);
        return true;
    }
    
    /**
     * @brief Try to pop an element (consumer only)
     * @return Optional containing element if successful, empty if queue is empty
     */
    [[nodiscard]] std::optional<T> tryPop() noexcept {
        T value;
        if (tryPop(value)) {
            return value;
        }
        return std::nullopt;
    }
    
    /**
     * @brief Check if queue is empty
     * @note May be stale by the time you act on it
     */
    [[nodiscard]] bool isEmpty() const noexcept {
        return m_head.load(std::memory_order_acquire) == 
               m_tail.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Check if queue is full
     * @note May be stale by the time you act on it
     */
    [[nodiscard]] bool isFull() const noexcept {
        const usize head = m_head.load(std::memory_order_acquire);
        const usize tail = m_tail.load(std::memory_order_acquire);
        return ((head + 1) & MASK) == tail;
    }
    
    /**
     * @brief Get approximate number of elements
     * @note May be stale by the time you act on it
     */
    [[nodiscard]] usize sizeApprox() const noexcept {
        const usize head = m_head.load(std::memory_order_acquire);
        const usize tail = m_tail.load(std::memory_order_acquire);
        return (head - tail) & MASK;
    }
    
    /**
     * @brief Get queue capacity
     */
    [[nodiscard]] static constexpr usize capacity() noexcept {
        return Capacity - 1; // One slot reserved for full/empty differentiation
    }

private:
    static constexpr usize MASK = Capacity - 1;
    
    // Cache line padding to prevent false sharing
    alignas(64) std::atomic<usize> m_head;
    alignas(64) std::atomic<usize> m_tail;
    alignas(64) std::array<T, Capacity> m_buffer;
};


/**
 * @brief Multi-Producer Single-Consumer lock-free queue
 * 
 * A bounded queue allowing multiple producer threads and a single consumer.
 * Uses a ticket-based approach for producer ordering.
 * 
 * @tparam T Element type (must be trivially copyable and trivially destructible)
 * @tparam Capacity Maximum number of elements (must be power of 2)
 * 
 * @security T must be trivially copyable to ensure atomic-safe concurrent access.
 *           Multiple producers writing non-trivial types could cause torn reads.
 *           Do not store raw pointers to mutable shared data.
 */
template <typename T, usize Capacity>
class MPSCQueue {
    // Security: Enforce trivial copyability to prevent torn reads/writes from concurrent producers
    static_assert(std::is_trivially_copyable_v<T>,
        "MPSCQueue<T>: T must be trivially copyable for safe lock-free operation. "
        "Concurrent producers writing non-trivial types may cause torn reads and UB.");
    
    static_assert(std::is_trivially_destructible_v<T>,
        "MPSCQueue<T>: T must be trivially destructible. "
        "Non-trivial destructors cannot be safely called in lock-free contexts.");
    
    static_assert(Capacity > 0 && (Capacity & (Capacity - 1)) == 0,
        "MPSCQueue: Capacity must be a power of 2");
    
    // Security: Warn about pointer types - concurrent access to pointed data is unsafe
    static_assert(!std::is_pointer_v<T>,
        "MPSCQueue<T>: Storing raw pointers is prohibited for security. "
        "Concurrent producers may cause stale/torn reads of pointed data. "
        "Use value types or indices into a thread-safe data structure.");

    struct Slot {
        std::atomic<usize> sequence;
        T data;
    };

public:
    MPSCQueue() noexcept {
        for (usize i = 0; i < Capacity; ++i) {
            m_buffer[i].sequence.store(i, std::memory_order_relaxed);
        }
        m_head.store(0, std::memory_order_relaxed);
        m_tail.store(0, std::memory_order_relaxed);
    }
    
    // Non-copyable, non-movable
    MPSCQueue(const MPSCQueue&) = delete;
    MPSCQueue& operator=(const MPSCQueue&) = delete;
    MPSCQueue(MPSCQueue&&) = delete;
    MPSCQueue& operator=(MPSCQueue&&) = delete;
    
    /**
     * @brief Try to push an element (any producer thread)
     * @param value Element to push (will be copied)
     * @return true if successful, false if queue is full
     * 
     * @security Value is copied atomically. T must be trivially copyable.
     */
    [[nodiscard]] bool tryPush(const T& value) noexcept {
        Slot* slot;
        usize pos = m_head.load(std::memory_order_relaxed);
        
        for (;;) {
            slot = &m_buffer[pos & MASK];
            usize seq = slot->sequence.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
            
            if (diff == 0) {
                // Slot is available, try to claim it
                if (m_head.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
                // Queue is full
                return false;
            } else {
                // Another producer claimed this slot, retry
                pos = m_head.load(std::memory_order_relaxed);
            }
        }
        
        // We own this slot, write the data
        slot->data = value;
        slot->sequence.store(pos + 1, std::memory_order_release);
        return true;
    }
    
    /**
     * @brief Try to pop an element (consumer thread only)
     * @param value Output parameter for popped element
     * @return true if successful, false if queue is empty
     */
    [[nodiscard]] bool tryPop(T& value) noexcept {
        Slot* slot;
        usize pos = m_tail.load(std::memory_order_relaxed);
        
        for (;;) {
            slot = &m_buffer[pos & MASK];
            usize seq = slot->sequence.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);
            
            if (diff == 0) {
                // Slot is ready to be consumed
                if (m_tail.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
                // Queue is empty or slot not yet written
                return false;
            } else {
                pos = m_tail.load(std::memory_order_relaxed);
            }
        }
        
        // Read the data
        value = slot->data;
        slot->sequence.store(pos + Capacity, std::memory_order_release);
        return true;
    }
    
    /**
     * @brief Try to pop an element (consumer thread only)
     * @return Optional containing element if successful, empty if queue is empty
     */
    [[nodiscard]] std::optional<T> tryPop() noexcept {
        T value;
        if (tryPop(value)) {
            return value;
        }
        return std::nullopt;
    }
    
    /**
     * @brief Check if queue appears empty
     * @note May be stale by the time you act on it
     */
    [[nodiscard]] bool isEmpty() const noexcept {
        return m_head.load(std::memory_order_acquire) == 
               m_tail.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Get approximate number of elements
     * @note May be stale by the time you act on it
     */
    [[nodiscard]] usize sizeApprox() const noexcept {
        const usize head = m_head.load(std::memory_order_acquire);
        const usize tail = m_tail.load(std::memory_order_acquire);
        return head - tail;
    }
    
    /**
     * @brief Get queue capacity
     */
    [[nodiscard]] static constexpr usize capacity() noexcept {
        return Capacity;
    }

private:
    static constexpr usize MASK = Capacity - 1;
    
    // Cache line padding to prevent false sharing
    alignas(64) std::atomic<usize> m_head;
    alignas(64) std::atomic<usize> m_tail;
    alignas(64) std::array<Slot, Capacity> m_buffer;
};


/**
 * @brief Type-safe message wrapper for queue communication
 * 
 * Use this to send typed messages through lock-free queues when you need
 * to communicate more than simple values.
 * 
 * @tparam MaxSize Maximum message size in bytes
 */
template <usize MaxSize = 64>
struct alignas(8) Message {
    static_assert(MaxSize >= 8, "Message size must be at least 8 bytes");
    
    u32 type;           ///< Application-defined message type
    u32 size;           ///< Actual data size
    std::array<u8, MaxSize - 8> data;  ///< Message payload
    
    Message() noexcept : type(0), size(0), data{} {}
    
    /**
     * @brief Create a message with typed payload
     * @tparam PayloadT Payload type (must fit in data array)
     */
    template <typename PayloadT>
    static Message create(u32 msgType, const PayloadT& payload) noexcept {
        static_assert(std::is_trivially_copyable_v<PayloadT>,
            "Message payload must be trivially copyable");
        static_assert(sizeof(PayloadT) <= MaxSize - 8,
            "Payload too large for Message");
        
        Message msg;
        msg.type = msgType;
        msg.size = sizeof(PayloadT);
        std::memcpy(msg.data.data(), &payload, sizeof(PayloadT));
        return msg;
    }
    
    /**
     * @brief Extract typed payload from message
     */
    template <typename PayloadT>
    [[nodiscard]] PayloadT payload() const noexcept {
        static_assert(std::is_trivially_copyable_v<PayloadT>,
            "Message payload must be trivially copyable");
        
        PayloadT result{};
        if (size >= sizeof(PayloadT)) {
            std::memcpy(&result, data.data(), sizeof(PayloadT));
        }
        return result;
    }
};

// Verify Message is suitable for lock-free queues
static_assert(std::is_trivially_copyable_v<Message<64>>,
    "Message must be trivially copyable");
static_assert(std::is_trivially_destructible_v<Message<64>>,
    "Message must be trivially destructible");

} // namespace nomad::core
