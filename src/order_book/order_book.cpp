#include "nexus/order_book/order_book.hpp"

#include <iomanip>
#include <ostream>

namespace nexus::order_book {

OrderBook::OrderBook(nexus::core::InstrumentId id)
    : instrument_id_(id) {}

void OrderBook::update(const nexus::core::BookUpdate& upd) {
    last_update_ts_ = upd.exchange_ts;
    for (std::size_t i = 0; i < MAX_DEPTH; ++i) {
        bids_[i] = PriceLevel{upd.bids[i].first, upd.bids[i].second, 0};
        asks_[i] = PriceLevel{upd.asks[i].first, upd.asks[i].second, 0};
    }
}

void OrderBook::update_bbo(nexus::core::Price bid, nexus::core::Quantity bid_qty,
                            nexus::core::Price ask, nexus::core::Quantity ask_qty) {
    bids_[0] = PriceLevel{bid, bid_qty, 0};
    asks_[0] = PriceLevel{ask, ask_qty, 0};
    for (std::size_t i = 1; i < MAX_DEPTH; ++i) {
        bids_[i] = PriceLevel{};
        asks_[i] = PriceLevel{};
    }
}

nexus::core::Price OrderBook::best_bid() const noexcept {
    return bids_[0].price;
}

nexus::core::Price OrderBook::best_ask() const noexcept {
    return asks_[0].price;
}

nexus::core::Price OrderBook::mid_price() const noexcept {
    return nexus::core::Price((best_bid().raw + best_ask().raw) / 2);
}

nexus::core::Price OrderBook::micro_price() const noexcept {
    // volume-weighted mid: (bid_px * ask_qty + ask_px * bid_qty) / (bid_qty + ask_qty)
    const auto bid_qty = static_cast<nexus::core::PriceRaw>(bids_[0].total_quantity);
    const auto ask_qty = static_cast<nexus::core::PriceRaw>(asks_[0].total_quantity);
    const auto total   = bid_qty + ask_qty;
    if (total == 0) {
        return mid_price(); // fallback when no size available
    }
    const nexus::core::PriceRaw num =
        best_bid().raw * ask_qty + best_ask().raw * bid_qty;
    return nexus::core::Price(num / total);
}

nexus::core::Price OrderBook::spread() const noexcept {
    return best_ask() - best_bid();
}

const std::array<PriceLevel, OrderBook::MAX_DEPTH>& OrderBook::bids() const noexcept {
    return bids_;
}

const std::array<PriceLevel, OrderBook::MAX_DEPTH>& OrderBook::asks() const noexcept {
    return asks_;
}

nexus::core::InstrumentId OrderBook::instrument_id() const noexcept {
    return instrument_id_;
}

nexus::core::TimestampNs OrderBook::last_update_ts() const noexcept {
    return last_update_ts_;
}

void OrderBook::print(std::ostream& os) const {
    os << "=== OrderBook [instrument=" << instrument_id_ << "] ===\n";
    os << std::setw(14) << "Ask Price"
       << std::setw(12) << "Ask Qty"
       << "\n";
    os << std::string(26, '-') << "\n";
    // Print asks in reverse (deepest first, best last)
    for (int i = static_cast<int>(MAX_DEPTH) - 1; i >= 0; --i) {
        if (!asks_[i].empty()) {
            os << std::setw(14) << std::fixed << std::setprecision(4)
               << asks_[i].price.to_double()
               << std::setw(12) << asks_[i].total_quantity
               << "\n";
        }
    }
    os << std::string(26, '=') << "\n";
    for (std::size_t i = 0; i < MAX_DEPTH; ++i) {
        if (!bids_[i].empty()) {
            os << std::setw(14) << std::fixed << std::setprecision(4)
               << bids_[i].price.to_double()
               << std::setw(12) << bids_[i].total_quantity
               << "\n";
        }
    }
    os << std::string(26, '-') << "\n";
    os << std::setw(14) << "Bid Price"
       << std::setw(12) << "Bid Qty"
       << "\n";
    os << "Spread: " << std::fixed << std::setprecision(4) << spread().to_double()
       << "  Mid: " << mid_price().to_double()
       << "  Micro: " << micro_price().to_double()
       << "\n";
}

} // namespace nexus::order_book
