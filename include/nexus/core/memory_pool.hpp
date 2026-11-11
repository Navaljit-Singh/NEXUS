#pragma once

#include <nexus/core/types.hpp>

#include <cstddef>
#include <cstdint>
#include <new>
#include <vector>
#include <stdexcept>
#include <utility>

namespace nexus::core {

/**
 * @brief High-performance, zero-fragmentation fixed-size block memory pool.
 *
 * Ensures deterministic O(1) allocation and deallocation for critical structures
 * such as Order, Trade, and Event objects.
 */
template <typename T, std::size_t BlockCount = 1024>
class FixedMemoryPool {
private:
    union Node {
        alignas(alignof(T)) std::byte storage[sizeof(T)];
        Node* next;
    };

public:
    FixedMemoryPool() {
        storage_ = static_cast<Node*>(::operator new[](sizeof(Node) * BlockCount, std::align_val_t{alignof(Node)}));
        free_list_ = &storage_[0];
        for (std::size_t i = 0; i < BlockCount - 1; ++i) {
            storage_[i].next = &storage_[i + 1];
        }
        storage_[BlockCount - 1].next = nullptr;
    }

    ~FixedMemoryPool() {
        ::operator delete[](storage_, std::align_val_t{alignof(Node)});
    }

    FixedMemoryPool(const FixedMemoryPool&) = delete;
    FixedMemoryPool& operator=(const FixedMemoryPool&) = delete;

    template <typename... Args>
    [[nodiscard]] T* allocate(Args&&... args) {
        if (!free_list_) {
            throw std::bad_alloc();
        }
        Node* node = free_list_;
        free_list_ = free_list_->next;
        allocated_count_++;

        T* ptr = reinterpret_cast<T*>(node->storage);
        return new (ptr) T(std::forward<Args>(args)...);
    }

    void deallocate(T* ptr) noexcept {
        if (!ptr) return;
        ptr->~T();
        Node* node = reinterpret_cast<Node*>(ptr);
        node->next = free_list_;
        free_list_ = node;
        allocated_count_--;
    }

    [[nodiscard]] std::size_t allocated_count() const noexcept {
        return allocated_count_;
    }

    [[nodiscard]] static constexpr std::size_t capacity() noexcept {
        return BlockCount;
    }

private:
    Node* storage_{nullptr};
    Node* free_list_{nullptr};
    std::size_t allocated_count_{0};
};

} // namespace nexus::core
