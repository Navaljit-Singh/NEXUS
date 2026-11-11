#include <nexus/strategy/sample_strategy.hpp>
#include <nexus/core/logger.hpp>

namespace nexus::strategy {

void EMACrossStrategy::on_bar(const nexus::core::BarData& bar) {
    const double close = bar.close.to_double();
    fast_ema_.update(close);
    slow_ema_.update(close);

    if (!fast_ema_.is_ready() || !slow_ema_.is_ready()) {
        return; // Not enough data yet
    }

    const double fast = fast_ema_.value();
    const double slow = slow_ema_.value();

    if (fast > slow && !last_signal_buy_) {
        nexus::core::Logger::instance().log(
            nexus::core::LogLevel::INFO,
            "EMA Cross BUY signal");
        last_signal_buy_ = true;
    } else if (fast < slow && last_signal_buy_) {
        nexus::core::Logger::instance().log(
            nexus::core::LogLevel::INFO,
            "EMA Cross SELL signal");
        last_signal_buy_ = false;
    }
}

void EMACrossStrategy::on_tick(const nexus::core::TickData& tick) {
    // Lightweight stub: log the last-traded price for observability.
    // A more complete implementation would feed tick.ltp into an RSI/VWAP here.
    nexus::core::Logger::instance().log(
        nexus::core::LogLevel::DEBUG,
        "Tick ltp=" + std::to_string(tick.ltp.to_double()));
}

} // namespace nexus::strategy
