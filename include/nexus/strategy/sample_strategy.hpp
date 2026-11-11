#pragma once

#include <nexus/strategy/strategy_base.hpp>
#include <nexus/strategy/indicators.hpp>

namespace nexus::strategy {

/// EMA crossover demo strategy.
/// Generates Buy/Sell log signals when the fast EMA crosses the slow EMA.
class EMACrossStrategy : public StrategyBase {
public:
    EMACrossStrategy(std::size_t fast_period = 9, std::size_t slow_period = 21)
        : StrategyBase("EMA_Cross_" + std::to_string(fast_period) + "_" + std::to_string(slow_period)),
          fast_ema_(fast_period),
          slow_ema_(slow_period) {}

    void on_tick(const nexus::core::TickData& tick) override;
    void on_bar(const nexus::core::BarData& bar) override;
    void on_book_update(const nexus::core::BookUpdate& /*update*/) override {}
    void on_order_update(const nexus::oms::Order& /*order*/) override {}

private:
    EMA  fast_ema_;
    EMA  slow_ema_;
    bool last_signal_buy_{false};
};

} // namespace nexus::strategy
