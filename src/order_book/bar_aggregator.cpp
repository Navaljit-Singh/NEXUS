#include "nexus/order_book/bar_aggregator.hpp"

#include <algorithm>

namespace nexus::order_book {

BarAggregator::BarAggregator(nexus::core::InstrumentId id,
                              std::uint32_t interval_seconds,
                              std::function<void(const nexus::core::BarData&)> on_bar)
    : instrument_id_(id)
    , interval_seconds_(interval_seconds)
    , on_bar_cb_(std::move(on_bar)) {}

void BarAggregator::open_bar(nexus::core::TimestampNs ts, nexus::core::Price first_price) {
    bar_open_    = true;
    bar_start_ts_ = ts;
    current_bar_ = nexus::core::BarData{
        instrument_id_,   // instrument_id
        ts,               // bar_open_ts
        interval_seconds_, // interval_seconds
        first_price,      // open
        first_price,      // high
        first_price,      // low
        first_price,      // close
        0                 // volume
    };
}

void BarAggregator::close_bar() {
    on_bar_cb_(current_bar_);
    bar_open_ = false;
}

void BarAggregator::on_tick(const nexus::core::TickData& tick) {
    const nexus::core::TimestampNs bar_window_ns =
        static_cast<nexus::core::TimestampNs>(interval_seconds_) * 1'000'000'000ULL;

    if (!bar_open_) {
        open_bar(tick.exchange_ts, tick.ltp);
    } else if (tick.exchange_ts >= bar_start_ts_ + bar_window_ns) {
        close_bar();
        open_bar(tick.exchange_ts, tick.ltp);
    }

    // Update current bar OHLCV
    if (tick.ltp.raw > current_bar_.high.raw) {
        current_bar_.high = tick.ltp;
    }
    if (tick.ltp.raw < current_bar_.low.raw) {
        current_bar_.low = tick.ltp;
    }
    current_bar_.close   = tick.ltp;
    current_bar_.volume += tick.volume;
}

void BarAggregator::flush() {
    if (bar_open_) {
        close_bar();
    }
}

} // namespace nexus::order_book
