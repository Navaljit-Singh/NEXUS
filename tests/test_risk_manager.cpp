// test_risk_manager.cpp
// Self-contained tests for RiskManager — no external test framework.

#include "nexus/risk/risk_manager.hpp"

#include <iostream>
#include <cstdlib>

using nexus::risk::RiskManager;
using nexus::risk::RiskLimits;
using nexus::risk::RiskCheckResult;
using nexus::core::Price;
using nexus::core::Side;

#define NEXUS_CHECK(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "CHECK FAILED: " #expr " at " << __FILE__ << ":" << __LINE__ << '\n'; \
            std::abort(); \
        } \
    } while (0)

// ---------------------------------------------------------------------------
// test_approved
// ---------------------------------------------------------------------------
static void test_approved() {
    RiskLimits limits{};          // default limits
    RiskManager rm{limits};

    // Small order: qty=100, price=100, ref_price=100 — well within defaults
    RiskCheckResult result = rm.check_order(1, Side::Buy, Price::from_double(100.0), 100, Price::from_double(100.0));
    NEXUS_CHECK(result == RiskCheckResult::Approved);

    std::cout << "[PASS] test_approved\n";
}

// ---------------------------------------------------------------------------
// test_reject_quantity
// ---------------------------------------------------------------------------
static void test_reject_quantity() {
    RiskLimits limits{};          // default: max_order_qty = 5000
    RiskManager rm{limits};

    // qty=10000 exceeds max_order_qty (5000)
    RiskCheckResult result = rm.check_order(1, Side::Buy, Price::from_double(100.0), 10000, Price::from_double(100.0));
    NEXUS_CHECK(result == RiskCheckResult::RejectedQuantity);

    std::cout << "[PASS] test_reject_quantity\n";
}

// ---------------------------------------------------------------------------
// test_kill_switch
// ---------------------------------------------------------------------------
static void test_kill_switch() {
    RiskLimits limits{};
    RiskManager rm{limits};

    rm.trigger_kill_switch();

    RiskCheckResult result = rm.check_order(1, Side::Buy, Price::from_double(100.0), 10, Price::from_double(100.0));
    NEXUS_CHECK(result == RiskCheckResult::KillSwitch);

    std::cout << "[PASS] test_kill_switch\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    test_approved();
    test_reject_quantity();
    test_kill_switch();

    std::cout << "All RiskManager tests passed.\n";
    return 0;
}
