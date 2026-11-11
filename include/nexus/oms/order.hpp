#pragma once

#include <nexus/core/types.hpp>
#include <cstdint>
#include <string>

namespace nexus::oms {

struct Order {
    nexus::core::OrderId      order_id{0};
    nexus::core::InstrumentId instrument_id{0};
    nexus::core::Side         side{nexus::core::Side::Buy};
    nexus::core::OrderType    type{nexus::core::OrderType::Limit};
    nexus::core::TimeInForce  tif{nexus::core::TimeInForce::Day};
    nexus::core::Price        limit_price{};
    nexus::core::Price        avg_fill_price{};
    nexus::core::Quantity     quantity{0};
    nexus::core::Quantity     filled_quantity{0};
    nexus::core::OrderStatus  status{nexus::core::OrderStatus::PendingNew};
    nexus::core::TimestampNs  created_ts{0};
    nexus::core::TimestampNs  updated_ts{0};
    std::string               client_order_id{};
    std::string               exchange_order_id{};
    std::string               reject_reason{};

    [[nodiscard]] nexus::core::Quantity leaves_qty() const noexcept {
        return quantity - filled_quantity;
    }

    [[nodiscard]] bool is_terminal() const noexcept;
};

} // namespace nexus::oms
