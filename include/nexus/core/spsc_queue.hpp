#pragma once

#include <nexus/core/types.hpp>

#include <atomic>
#include <cstddef>
#include <concepts>
#include <memory>
#include <new>
#include <utility>
#include <type_traits>
#include <optional>

namespace nexus::core {

/**
 * @brief Lock-free, wait-free Single-Producer Single-Consumer (SPSC) Ring Buffer.
 *
 * Designed for microsecond-level tick dispatching between threads.
 * Optimizations:
 * 1. Power-of-2 capacity allows bitwise masking instead of expensive modulo division.
 * 2. Head and Tail indices are isolated on separate cache lines (64 bytes) to eliminate false sharing.
 * 3. Producer and Consumer cache opposing indices to minimize cross-core cache bus snooping.
 * 4. Zero dynamic allocation on push/pop.
 */
template <typename T, std::size_t Capacity>
requires (Capacity > 1 && (Capacity & (Capacity - 1)) == 0) // Must be power of 2
class SPSCQueue {
public:
    using value_type = T;
    static constexpr std::size_t MASK = Capacity - 1;

    SPSCQueue() {
        buffer_ = static_cast<T*>(::operator new[](sizeof(T) * Capacity, std::align_val_t{alignof(T)}));
        for (std::size_t i = 0; i < Capacity; ++i) {
            new (&buffer_[i]) T();
        }
    }

    ~SPSCQueue() {
        for (std::size_t i = 0; i < Capacity; ++i) {
            buffer_[i].~T();
        }
        ::operator delete[](buffer_, std::align_val_t{alignof(T)});
    }

    // Non-copyable, non-movable (pinned thread communication channel)
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;
    SPSCQueue(SPSCQueue&&) = delete;
    SPSCQueue& operator=(SPSCQueue&&) = delete;

    /**
     * @brief Emplace an item into the queue. Called ONLY by producer thread.
     * @return true if successfully pushed, false if queue is full.
     */
    template <typename... Args>
    [[nodiscard]] bool emplace(Args&&... args) noexcept {
        const std::size_t current_tail = tail_.load(std::memory_order_relaxed);

        // Check if queue is full using cached head to prevent hitting shared cache line
        if (current_tail - cached_head_ >= Capacity) {
            cached_head_ = head_.load(std::memory_order_acquire);
            if (current_tail - cached_head_ >= Capacity) {
                return false; // Queue is genuinely full
            }
        }

        const std::size_t index = current_tail & MASK;
        buffer_[index] = T(std::forward<Args>(args)...);

        tail_.store(current_tail + 1, std::memory_order_release);
        return true;
    }

    /**
     * @brief Push an item into the queue by copy or move. Called ONLY by producer thread.
     */
    [[nodiscard]] bool push(const T& item) noexcept {
        return emplace(item);
    }

    [[nodiscard]] bool push(T&& item) noexcept {
        return emplace(std::move(item));
    }

    /**
     * @brief Pop an item from the queue. Called ONLY by consumer thread.
     * @param out Reference to write popped item into.
     * @return true if an item was popped, false if queue was empty.
     */
    [[nodiscard]] bool pop(T& out) noexcept {
        const std::size_t current_head = head_.load(std::memory_order_relaxed);

        // Check if queue is empty using cached tail
        if (current_head == cached_tail_) {
            cached_tail_ = tail_.load(std::memory_order_acquire);
            if (current_head == cached_tail_) {
                return false; // Queue is genuinely empty
            }
        }

        const std::size_t index = current_head & MASK;
        out = std::move(buffer_[index]);

        head_.store(current_head + 1, std::memory_order_release);
        return true;
    }

    /**
     * @brief Pop and return item as std::optional.
     */
    [[nodiscard]] std::optional<T> try_pop() noexcept {
        T item;
        if (pop(item)) {
            return item;
        }
        return std::nullopt;
    }

    /**
     * @brief Approximate number of items in queue.
     */
    [[nodiscard]] std::size_t size() const noexcept {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        const std::size_t head = head_.load(std::memory_order_relaxed);
        return (tail >= head) ? (tail - head) : 0;
    }

    [[nodiscard]] bool empty() const noexcept {
        return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] static constexpr std::size_t capacity() noexcept {
        return Capacity;
    }

private:
    T* buffer_{nullptr};

    // Producer variables aligned to separate cache line
    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> tail_{0};
    std::size_t cached_head_{0};

    // Consumer variables aligned to separate cache line
    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> head_{0};
    std::size_t cached_tail_{0};

    // Trailing padding to protect next object in memory
    alignas(CACHE_LINE_SIZE) char padding_[CACHE_LINE_SIZE - sizeof(std::size_t)];
};

} // namespace nexus::core
