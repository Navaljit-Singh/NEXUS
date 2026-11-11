#pragma once

#include <nexus/core/types.hpp>
#include <nexus/core/logger.hpp>
#include <cstdint>
#include <atomic>
#include <string>

namespace nexus::risk {

struct RiskLimits {
    std::uint32_t max_order_quantity{5000};
    double        max_order_value_inr{1000000.0};
    double        max_daily_loss_inr{50000.0};
    double        price_collar_pct{5.0};
    std::uint32_t max_open_orders{50};
    std::uint32_t rate_limit_orders_per_second{20};
};

enum class RiskCheckResult : std::uint8_t {
    Approved             = 0,
    RejectedQuantity     = 1,
    RejectedValue        = 2,
    RejectedDailyLoss    = 3,
    RejectedPriceCollar  = 4,
    RejectedRateLimit    = 5,
    RejectedOpenOrders   = 6,
    KillSwitch           = 7
};

inline const char* to_string(RiskCheckResult r) noexcept {
    switch (r) {
        case RiskCheckResult::Approved:            return "Approved";
        case RiskCheckResult::RejectedQuantity:    return "RejectedQuantity";
        case RiskCheckResult::RejectedValue:       return "RejectedValue";
        case RiskCheckResult::RejectedDailyLoss:   return "RejectedDailyLoss";
        case RiskCheckResult::RejectedPriceCollar: return "RejectedPriceCollar";
        case RiskCheckResult::RejectedRateLimit:   return "RejectedRateLimit";
        case RiskCheckResult::RejectedOpenOrders:  return "RejectedOpenOrders";
        case RiskCheckResult::KillSwitch:          return "KillSwitch";
        default:                                   return "Unknown";
    }
}

class RiskManager {
public:
    explicit RiskManager(RiskLimits limits);

    RiskCheckResult check_order(nexus::core::InstrumentId id,
                                nexus::core::Side         side,
                                nexus::core::Price        price,
                                nexus::core::Quantity     qty,
                                nexus::core::Price        ref_price) noexcept;

    void on_order_filled(nexus::core::Quantity qty,
                         nexus::core::Price    price,
                         nexus::core::Side     side,
                         nexus::core::Price    avg_cost);

    void on_order_opened();
    void on_order_closed();

    void trigger_kill_switch() noexcept;

    [[nodiscard]] bool   kill_switch_active() const noexcept;
    [[nodiscard]] double realised_pnl()       const noexcept;

    void reset_daily() noexcept;

private:
    RiskLimits limits_;

    std::atomic<bool>          kill_switch_{false};
    std::atomic<std::uint32_t> open_order_count_{0};

    double        daily_pnl_{0.0};
    std::uint64_t last_order_epoch_ns_{0};
    std::uint32_t orders_this_second_{0};
};

} // namespace nexus::risk
