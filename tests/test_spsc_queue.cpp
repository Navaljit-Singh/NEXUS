// test_spsc_queue.cpp
// Self-contained tests for SPSCQueue — no external test framework.

#include "nexus/core/spsc_queue.hpp"

#include <atomic>
#include <thread>
#include <iostream>
#include <cstdlib>

using nexus::core::SPSCQueue;

#define NEXUS_CHECK(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "CHECK FAILED: " #expr " at " << __FILE__ << ":" << __LINE__ << '\n'; \
            std::abort(); \
        } \
    } while (0)

// ---------------------------------------------------------------------------
// test_basic_push_pop
// ---------------------------------------------------------------------------
static void test_basic_push_pop() {
    SPSCQueue<int, 1024> q;

    NEXUS_CHECK(q.push(10));
    NEXUS_CHECK(q.push(20));
    NEXUS_CHECK(q.push(30));

    int v{};
    NEXUS_CHECK(q.pop(v) && v == 10);
    NEXUS_CHECK(q.pop(v) && v == 20);
    NEXUS_CHECK(q.pop(v) && v == 30);

    std::cout << "[PASS] test_basic_push_pop\n";
}

// ---------------------------------------------------------------------------
// test_full_queue
// ---------------------------------------------------------------------------
static void test_full_queue() {
    SPSCQueue<int, 1024> q;

    // Fill all slots up to capacity
    for (std::size_t i = 0; i < q.capacity(); ++i) {
        NEXUS_CHECK(q.push(static_cast<int>(i)));
    }
    // The queue is now full; next push must fail
    NEXUS_CHECK(!q.push(9999));

    std::cout << "[PASS] test_full_queue\n";
}

// ---------------------------------------------------------------------------
// test_empty_pop
// ---------------------------------------------------------------------------
static void test_empty_pop() {
    SPSCQueue<int, 1024> q;
    int v{};
    NEXUS_CHECK(!q.pop(v));

    std::cout << "[PASS] test_empty_pop\n";
}

// ---------------------------------------------------------------------------
// test_threaded
// ---------------------------------------------------------------------------
static void test_threaded() {
    SPSCQueue<int, 1024> q;
    constexpr int N = 10000;
    std::atomic<long long> consumed_sum{0};

    std::thread producer([&] {
        for (int i = 1; i <= N; ++i) {
            while (!q.push(i)) {
                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([&] {
        int count = 0;
        long long sum = 0;
        while (count < N) {
            int v{};
            if (q.pop(v)) {
                sum += v;
                ++count;
            } else {
                std::this_thread::yield();
            }
        }
        consumed_sum.store(sum, std::memory_order_relaxed);
    });

    producer.join();
    consumer.join();

    constexpr long long expected = static_cast<long long>(N) * (N + 1) / 2;
    NEXUS_CHECK(consumed_sum.load() == expected);

    std::cout << "[PASS] test_threaded\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    test_basic_push_pop();
    test_full_queue();
    test_empty_pop();
    test_threaded();

    std::cout << "All SPSC tests passed.\n";
    return 0;
}
