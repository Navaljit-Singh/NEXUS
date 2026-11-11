#pragma once

#include <nexus/oms/order.hpp>
#include <nexus/risk/risk_manager.hpp>
#include <nexus/core/logger.hpp>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <mutex>

namespace nexus::oms {

class OrderManager {
public:
    using OnOrderUpdate = std::function<void(const Order&)>;

    explicit OrderManager(nexus::risk::RiskManager& risk_mgr, OnOrderUpdate on_update);

    nexus::core::OrderId submit_order(nexus::core::InstrumentId instrument_id,
                                      nexus::core::Side         side,
                                      nexus::core::OrderType    type,
                                      nexus::core::Price        price,
                                      nexus::core::Quantity     qty,
                                      nexus::core::Price        ref_price);

    bool cancel_order(nexus::core::OrderId order_id);

    void on_fill(nexus::core::OrderId   order_id,
                 nexus::core::Price     fill_price,
                 nexus::core::Quantity  fill_qty);

    void on_reject(nexus::core::OrderId order_id, std::string reason);

    [[nodiscard]] const Order* find_order(nexus::core::OrderId order_id) const;

    [[nodiscard]] std::size_t open_order_count() const noexcept;

private:
    nexus::risk::RiskManager& risk_mgr_;
    OnOrderUpdate             on_update_cb_;

    std::unordered_map<nexus::core::OrderId, Order> orders_;
    std::atomic<nexus::core::OrderId>               next_order_id_{1};
    mutable std::mutex                              mutex_;
};

} // namespace nexus::oms
