// test_order_book.cpp
// Self-contained tests for OrderBook — no external test framework.

#include "nexus/order_book/order_book.hpp"
#include "nexus/core/event.hpp"

#include <cmath>
#include <iostream>
#include <cstdlib>

using nexus::order_book::OrderBook;
using nexus::core::BookUpdate;
using nexus::core::Price;
using nexus::core::Quantity;

#define NEXUS_CHECK(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "CHECK FAILED: " #expr " at " << __FILE__ << ":" << __LINE__ << '\n'; \
            std::abort(); \
        } \
    } while (0)

static bool approx_eq(double a, double b, double eps = 1e-4) {
    return std::fabs(a - b) < eps;
}

// ---------------------------------------------------------------------------
// test_bbo_update
// ---------------------------------------------------------------------------
static void test_bbo_update() {
    OrderBook ob{1};

    ob.update_bbo(Price::from_double(100.0), 500, Price::from_double(100.05), 300);

    NEXUS_CHECK(approx_eq(ob.best_bid().to_double(), 100.0));
    NEXUS_CHECK(approx_eq(ob.best_ask().to_double(), 100.05));
    NEXUS_CHECK(approx_eq(ob.spread().to_double(),   0.05));
    NEXUS_CHECK(approx_eq(ob.mid_price().to_double(), 100.025));

    std::cout << "[PASS] test_bbo_update\n";
}

// ---------------------------------------------------------------------------
// test_full_book_update
// ---------------------------------------------------------------------------
static void test_full_book_update() {
    OrderBook ob{1};

    BookUpdate bu{};
    bu.instrument_id = 1;
    bu.exchange_ts   = 0;

    // 5 bid levels descending: 100, 99, 98, 97, 96
    for (std::size_t i = 0; i < 5; ++i) {
        bu.bids[i] = {Price::from_double(100.0 - static_cast<double>(i)), static_cast<Quantity>(100 * (i + 1))};
    }
    // 5 ask levels ascending: 100.05, 100.10, 100.15, 100.20, 100.25
    for (std::size_t i = 0; i < 5; ++i) {
        bu.asks[i] = {Price::from_double(100.05 + 0.05 * static_cast<double>(i)), static_cast<Quantity>(200 * (i + 1))};
    }

    ob.update(bu);

    // Best bid is first element (highest)
    NEXUS_CHECK(approx_eq(ob.bids()[0].price.to_double(), 100.0));
    // Best ask is first element (lowest)
    NEXUS_CHECK(approx_eq(ob.asks()[0].price.to_double(), 100.05));

    std::cout << "[PASS] test_full_book_update\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    test_bbo_update();
    test_full_book_update();

    std::cout << "All OrderBook tests passed.\n";
    return 0;
}
