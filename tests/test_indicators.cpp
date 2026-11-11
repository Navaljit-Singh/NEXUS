// test_indicators.cpp
// Self-contained tests for technical indicators — no external test framework.

#include "nexus/strategy/indicators.hpp"

#include <cmath>
#include <iostream>
#include <cstdlib>

using nexus::strategy::SMA;
using nexus::strategy::EMA;
using nexus::strategy::RSI;
using nexus::strategy::VWAP;

#define NEXUS_CHECK(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "CHECK FAILED: " #expr " at " << __FILE__ << ":" << __LINE__ << '\n'; \
            std::abort(); \
        } \
    } while (0)

static constexpr double EPSILON = 1e-6;

static bool approx_eq(double a, double b, double eps = EPSILON) {
    return std::fabs(a - b) < eps;
}

// ---------------------------------------------------------------------------
// test_sma
// ---------------------------------------------------------------------------
static void test_sma() {
    SMA sma{3};

    // Feed 1, 2 — not ready yet
    sma.update(1.0);
    NEXUS_CHECK(!sma.is_ready());
    sma.update(2.0);
    NEXUS_CHECK(!sma.is_ready());

    // Feed 3 — ready, value = (1+2+3)/3 = 2.0
    sma.update(3.0);
    NEXUS_CHECK(sma.is_ready());
    NEXUS_CHECK(approx_eq(sma.value(), 2.0));

    // Feed 4 — value = (2+3+4)/3 = 3.0
    sma.update(4.0);
    NEXUS_CHECK(approx_eq(sma.value(), 3.0));

    // Feed 5 — value = (3+4+5)/3 = 4.0
    sma.update(5.0);
    NEXUS_CHECK(approx_eq(sma.value(), 4.0));

    std::cout << "[PASS] test_sma\n";
}

// ---------------------------------------------------------------------------
// test_ema
// ---------------------------------------------------------------------------
static void test_ema() {
    EMA ema{5};

    // Feed 10 identical values of 100.0
    for (int i = 0; i < 10; ++i) {
        ema.update(100.0);
    }

    NEXUS_CHECK(ema.is_ready());
    // If all values are the same, EMA must converge to that value
    NEXUS_CHECK(approx_eq(ema.value(), 100.0, 1e-4));

    std::cout << "[PASS] test_ema\n";
}

// ---------------------------------------------------------------------------
// test_rsi_range
// ---------------------------------------------------------------------------
static void test_rsi_range() {
    RSI rsi{14};

    // Feed 15 alternating values: 100, 101, 100, 101, ...
    for (int i = 0; i < 15; ++i) {
        rsi.update(100.0 + (i % 2) * 1.0);
    }

    NEXUS_CHECK(rsi.is_ready());
    double v = rsi.value();
    NEXUS_CHECK(v >= 0.0 && v <= 100.0);

    std::cout << "[PASS] test_rsi_range\n";
}

// ---------------------------------------------------------------------------
// test_vwap
// ---------------------------------------------------------------------------
static void test_vwap() {
    VWAP vwap{};

    vwap.update(100.0, 100);
    vwap.update(200.0, 100);

    // Expected VWAP = (100*100 + 200*100) / (100+100) = 150.0
    NEXUS_CHECK(approx_eq(vwap.value(), 150.0));

    std::cout << "[PASS] test_vwap\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    test_sma();
    test_ema();
    test_rsi_range();
    test_vwap();

    std::cout << "All Indicator tests passed.\n";
    return 0;
}
