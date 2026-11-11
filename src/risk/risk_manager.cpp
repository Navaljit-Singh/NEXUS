#include <nexus/risk/risk_manager.hpp>
#include <nexus/core/time.hpp>

#include <cmath>
#include <cstdio>

namespace nexus::risk {

RiskManager::RiskManager(RiskLimits limits)
    : limits_(limits) {}

RiskCheckResult RiskManager::check_order(nexus::core::InstrumentId /*id*/,
                                          nexus::core::Side         /*side*/,
                                          nexus::core::Price        price,
                                          nexus::core::Quantity     qty,
                                          nexus::core::Price        ref_price) noexcept {
    // 1. Kill switch
    if (kill_switch_.load(std::memory_order_acquire)) {
        return RiskCheckResult::KillSwitch;
    }

    // 2. Open orders cap
    if (open_order_count_.load(std::memory_order_relaxed) >= limits_.max_open_orders) {
        return RiskCheckResult::RejectedOpenOrders;
    }

    // 3. Quantity limit
    if (qty >= limits_.max_order_quantity) {
        return RiskCheckResult::RejectedQuantity;
    }

    // 4. Order value limit
    const double order_value = price.to_double() * static_cast<double>(qty);
    if (order_value >= limits_.max_order_value_inr) {
        return RiskCheckResult::RejectedValue;
    }

    // 5. Price collar (skip if ref_price is zero)
    if (ref_price.raw != 0) {
        const double deviation_pct =
            std::abs(price.to_double() - ref_price.to_double()) / ref_price.to_double() * 100.0;
        if (deviation_pct >= limits_.price_collar_pct) {
            return RiskCheckResult::RejectedPriceCollar;
        }
    }

    // 6. Rate limit (orders per second)
    const std::uint64_t now_ns      = nexus::core::TimeUtil::epoch_ns();
    const std::uint64_t now_sec     = now_ns / 1'000'000'000ULL;
    const std::uint64_t last_sec    = last_order_epoch_ns_ / 1'000'000'000ULL;

    if (now_sec == last_sec) {
        if (orders_this_second_ >= limits_.rate_limit_orders_per_second) {
            return RiskCheckResult::RejectedRateLimit;
        }
        ++orders_this_second_;
    } else {
        orders_this_second_    = 1;
        last_order_epoch_ns_   = now_ns;
    }

    // 7. Daily loss limit
    if (daily_pnl_ <= -limits_.max_daily_loss_inr) {
        return RiskCheckResult::RejectedDailyLoss;
    }

    return RiskCheckResult::Approved;
}

void RiskManager::on_order_filled(nexus::core::Quantity qty,
                                   nexus::core::Price    price,
                                   nexus::core::Side     side,
                                   nexus::core::Price    avg_cost) {
    double pnl_contribution = 0.0;
    if (side == nexus::core::Side::Sell) {
        // Sold at price, cost basis was avg_cost
        pnl_contribution = (price - avg_cost).to_double() * static_cast<double>(qty);
    } else {
        // Bought at price vs avg_cost reference (closing a short, etc.)
        pnl_contribution = (avg_cost - price).to_double() * static_cast<double>(qty);
    }
    daily_pnl_ += pnl_contribution;
}

void RiskManager::on_order_opened() {
    open_order_count_.fetch_add(1, std::memory_order_relaxed);
}

void RiskManager::on_order_closed() {
    open_order_count_.fetch_sub(1, std::memory_order_relaxed);
}

void RiskManager::trigger_kill_switch() noexcept {
    kill_switch_.store(true, std::memory_order_release);
    NEXUS_LOG_CRIT("KILL SWITCH ACTIVATED — all new orders will be rejected");
}

bool RiskManager::kill_switch_active() const noexcept {
    return kill_switch_.load(std::memory_order_acquire);
}

double RiskManager::realised_pnl() const noexcept {
    return daily_pnl_;
}

void RiskManager::reset_daily() noexcept {
    daily_pnl_           = 0.0;
    orders_this_second_  = 0;
    last_order_epoch_ns_ = 0;
}

} // namespace nexus::risk
