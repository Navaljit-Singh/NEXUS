#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <ostream>

#include "nexus/core/event.hpp"
#include "nexus/core/types.hpp"
#include "nexus/order_book/level.hpp"

namespace nexus::order_book {

class OrderBook {
public:
    static constexpr std::size_t MAX_DEPTH = 5;

    explicit OrderBook(nexus::core::InstrumentId id);

    void update(const nexus::core::BookUpdate& update);
    void update_bbo(nexus::core::Price bid, nexus::core::Quantity bid_qty,
                    nexus::core::Price ask, nexus::core::Quantity ask_qty);

    [[nodiscard]] nexus::core::Price best_bid()    const noexcept;
    [[nodiscard]] nexus::core::Price best_ask()    const noexcept;
    [[nodiscard]] nexus::core::Price mid_price()   const noexcept;
    [[nodiscard]] nexus::core::Price micro_price() const noexcept; // volume-weighted mid
    [[nodiscard]] nexus::core::Price spread()      const noexcept;

    [[nodiscard]] const std::array<PriceLevel, MAX_DEPTH>& bids() const noexcept;
    [[nodiscard]] const std::array<PriceLevel, MAX_DEPTH>& asks() const noexcept;

    [[nodiscard]] nexus::core::InstrumentId  instrument_id()   const noexcept;
    [[nodiscard]] nexus::core::TimestampNs   last_update_ts()  const noexcept;

    void print(std::ostream& os) const;

private:
    nexus::core::InstrumentId           instrument_id_;
    nexus::core::TimestampNs            last_update_ts_{0};
    std::array<PriceLevel, MAX_DEPTH>   bids_{};
    std::array<PriceLevel, MAX_DEPTH>   asks_{};
};

} // namespace nexus::order_book
