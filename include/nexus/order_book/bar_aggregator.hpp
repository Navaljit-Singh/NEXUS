#pragma once

#include <cstdint>
#include <functional>

#include "nexus/core/event.hpp"
#include "nexus/core/types.hpp"

namespace nexus::order_book {

class BarAggregator {
public:
    BarAggregator(nexus::core::InstrumentId id,
                  std::uint32_t interval_seconds,
                  std::function<void(const nexus::core::BarData&)> on_bar);

    void on_tick(const nexus::core::TickData& tick);
    void flush(); // force-close current bar

private:
    nexus::core::InstrumentId                           instrument_id_;
    std::uint32_t                                       interval_seconds_;
    std::function<void(const nexus::core::BarData&)>    on_bar_cb_;
    nexus::core::BarData                                current_bar_{};
    bool                                                bar_open_{false};
    nexus::core::TimestampNs                            bar_start_ts_{0};

    void open_bar(nexus::core::TimestampNs ts, nexus::core::Price first_price);
    void close_bar();
};

} // namespace nexus::order_book
