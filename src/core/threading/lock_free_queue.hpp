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
    /**
 * @brief Constructs an empty queue and initializes internal head and tail indices to zero.
 *
 * The resulting queue is empty and ready for single-producer / single-consumer use.
 */
SPSCQueue() noexcept : m_head(0), m_tail(0) {}
    
    /**
 * @brief Deleted copy constructor; instances of SPSCQueue are not copyable.
 *
 * Ensures the queue cannot be copied or duplicated, preventing accidental sharing
 * of internal atomic state between objects.
 */
    SPSCQueue(const SPSCQueue&) = delete;
    /**
 * @brief Disabled copy-assignment.
 *
 * Prevents assigning one SPSCQueue to another; the queue type is not copy-assignable to preserve exclusive ownership and lock-free correctness.
 */
SPSCQueue& operator=(const SPSCQueue&) = delete;
    /**
 * @brief Deleted move constructor.
 *
 * Disables move construction for SPSCQueue; instances cannot be moved.
 */
SPSCQueue(SPSCQueue&&) = delete;
    /**
 * @brief Delete move assignment operator to prevent moving a queue instance.
 *
 * Instances of SPSCQueue are intentionally non-movable to preserve internal
 * lock-free invariants and fixed storage layout.
 */
SPSCQueue& operator=(SPSCQueue&&) = delete;
    
    /**
     * @brief Attempts to enqueue an element into the queue (producer-only).
     *
     * @param value Element to enqueue.
     * @return `true` if the element was enqueued, `false` if the queue was full.
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
     * @brief Attempt to remove the next element from the queue and store it in `value`.
     *
     * @param[out] value Destination for the removed element.
     * @return `true` if an element was removed and written to `value`, `false` if the queue was empty.
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
     * @brief Attempts to pop the next element from the queue (consumer thread only).
     *
     * @return std::optional<T> containing the element if one was available, `std::nullopt` if the queue is empty.
     */
    [[nodiscard]] std::optional<T> tryPop() noexcept {
        T value;
        if (tryPop(value)) {
            return value;
        }
        return std::nullopt;
    }
    
    /**
     * @brief Reports whether the queue currently contains no elements.
     *
     * The result reflects the head and tail indices at the time of the call and may be stale immediately afterwards.
     *
     * @return `true` if the queue contains no elements at the time of the check, `false` otherwise.
     */
    [[nodiscard]] bool isEmpty() const noexcept {
        return m_head.load(std::memory_order_acquire) == 
               m_tail.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Reports whether the queue is currently full.
     *
     * The result reflects a snapshot and may be stale by the time the caller acts.
     *
     * @return `true` if the queue is full (the reserved slot prevents head from advancing), `false` otherwise.
     */
    [[nodiscard]] bool isFull() const noexcept {
        const usize head = m_head.load(std::memory_order_acquire);
        const usize tail = m_tail.load(std::memory_order_acquire);
        return ((head + 1) & MASK) == tail;
    }
    
    /**
     * @brief Reports the approximate number of elements currently stored in the queue.
     *
     * The value is a snapshot and may be stale immediately due to concurrent producer/consumer activity.
     *
     * @return usize Approximate count of elements in the queue.
     */
    [[nodiscard]] usize sizeApprox() const noexcept {
        const usize head = m_head.load(std::memory_order_acquire);
        const usize tail = m_tail.load(std::memory_order_acquire);
        return (head - tail) & MASK;
    }
    
    /**
     * @brief Maximum number of elements the queue can hold.
     *
     * @return usize The usable capacity (Capacity - 1). One slot is reserved to distinguish full from empty.
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
    /**
     * @brief Initializes the queue internal state for use by producers and the consumer.
     *
     * Initializes each slot's sequence counter to its index and sets the head and tail indices to zero,
     * preparing the ring buffer for concurrent push/pop operations.
     */
    MPSCQueue() noexcept {
        for (usize i = 0; i < Capacity; ++i) {
            m_buffer[i].sequence.store(i, std::memory_order_relaxed);
        }
        m_head.store(0, std::memory_order_relaxed);
        m_tail.store(0, std::memory_order_relaxed);
    }
    
    /**
 * @brief Deleted copy constructor; copying an MPSCQueue is prohibited.
 *
 * Enforces non-copyable semantics to prevent multiple owners of the queue's internal state.
 */
    MPSCQueue(const MPSCQueue&) = delete;
    /**
 * @brief Disable copy assignment for MPSCQueue.
 *
 * Copy assignment is deleted to prevent duplicating queue instances; MPSCQueue is non-copyable.
 */
MPSCQueue& operator=(const MPSCQueue&) = delete;
    /**
 * @brief Deleted move constructor to make the queue non-movable.
 *
 * Prevents move construction so instances retain stable identity and ownership semantics required by the lock-free implementation.
 */
MPSCQueue(MPSCQueue&&) = delete;
    /**
 * @brief Disabled move-assignment operator that prevents moving an MPSCQueue instance.
 *
 * @note MPSCQueue objects are intentionally neither copyable nor movable to preserve internal concurrency
 * invariants and memory-layout guarantees.
 */
MPSCQueue& operator=(MPSCQueue&&) = delete;
    
    /**
     * @brief Enqueues an element from any producer thread.
     *
     * @param value Element to enqueue; it will be copied into the queue.
     * @return true if the element was enqueued, false otherwise.
     *
     * @note The element type `T` must be trivially copyable.
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
     * @brief Attempts to pop the next element from the queue (consumer thread only).
     *
     * @return std::optional<T> containing the element if one was available, `std::nullopt` if the queue is empty.
     */
    [[nodiscard]] std::optional<T> tryPop() noexcept {
        T value;
        if (tryPop(value)) {
            return value;
        }
        return std::nullopt;
    }
    
    /**
     * @brief Reports whether the queue currently contains no elements.
     *
     * The result reflects the head and tail indices at the time of the call and may be stale immediately afterwards.
     *
     * @return `true` if the queue contains no elements at the time of the check, `false` otherwise.
     */
    [[nodiscard]] bool isEmpty() const noexcept {
        return m_head.load(std::memory_order_acquire) == 
               m_tail.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Reports an approximate number of elements currently in the queue.
     *
     * The value may be stale due to concurrent producers/consumer.
     *
     * @return usize Approximate count of elements; may be stale.
     */
    [[nodiscard]] usize sizeApprox() const noexcept {
        const usize head = m_head.load(std::memory_order_acquire);
        const usize tail = m_tail.load(std::memory_order_acquire);
        return head - tail;
    }
    
    /**
     * @brief Report the maximum number of elements the queue can contain.
     *
     * @return size_t The maximum number of elements the queue can hold (equal to `Capacity`).
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
    std::array<u8, MaxSize - 8> data;  /**
 * @brief Initializes an empty message with type and size set to 0 and payload bytes zeroed.
 *
 * Constructs a Message with `type = 0`, `size = 0`, and `data` filled with zeros.
 */
    
    Message() noexcept : type(0), size(0), data{} {}
    
    /**
     * @brief Create a message with typed payload
     * @tparam PayloadT Payload type (must fit in data array)
     */
    template <typename PayloadT>
    /**
     * @brief Constructs a Message containing a trivially copyable payload.
     *
     * Creates a Message whose `type` is set to `msgType`, whose `size` is set to `sizeof(PayloadT)`,
     * and whose payload bytes are copied into the internal data buffer.
     *
     * @tparam PayloadT Type of the payload; must be trivially copyable and fit within the message payload buffer.
     * @param msgType 32-bit message type identifier.
     * @param payload Payload value to embed in the message; its bytes are memcpy'd into the message buffer.
     * @return Message A Message with `type`, `size`, and `data` populated from the provided payload.
     *
     * @note Compile-time checks enforce `std::is_trivially_copyable_v<PayloadT>` and
     *       `sizeof(PayloadT) <= MaxSize - 8`.
     */
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
    /**
     * @brief Extracts a typed payload from the message.
     *
     * If the stored payload size is at least sizeof(PayloadT), copies that many bytes into and returns a `PayloadT` value.
     * If the stored size is smaller than `sizeof(PayloadT)`, returns a default-constructed `PayloadT`.
     *
     * @tparam PayloadT Trivially copyable type to extract; a static assertion enforces this.
     * @return PayloadT The extracted payload or a default-constructed `PayloadT` when the stored size is insufficient.
     */
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