#pragma once

#include "nexus/core/types.hpp"
#include <variant>
#include <array>
#include <utility>
#include <cstdint>

namespace nexus::core {

struct TickData {
    InstrumentId instrument_id{0};
    TimestampNs  exchange_ts{0};
    TimestampNs  recv_ts{0};
    Price        ltp{};
    Quantity     volume{0};
    Price        bid{};
    Price        ask{};
    Quantity     bid_qty{0};
    Quantity     ask_qty{0};
};

struct BarData {
    InstrumentId   instrument_id{0};
    TimestampNs    bar_open_ts{0};
    std::uint32_t  interval_seconds{1};
    Price          open{}, high{}, low{}, close{};
    Quantity       volume{0};
};

struct BookUpdate {
    InstrumentId                            instrument_id{0};
    TimestampNs                             exchange_ts{0};
    std::array<std::pair<Price, Quantity>, 5> bids{};
    std::array<std::pair<Price, Quantity>, 5> asks{};
};

using MarketEvent = std::variant<TickData, BarData, BookUpdate>;

} // namespace nexus::core
