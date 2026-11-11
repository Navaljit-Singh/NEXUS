#include <nexus/oms/order_manager.hpp>
#include <nexus/core/time.hpp>

#include <cstdio>

namespace nexus::oms {

// ---------------------------------------------------------------------------
// Order helpers
// ---------------------------------------------------------------------------

bool Order::is_terminal() const noexcept {
    return status == nexus::core::OrderStatus::Filled    ||
           status == nexus::core::OrderStatus::Cancelled ||
           status == nexus::core::OrderStatus::Rejected  ||
           status == nexus::core::OrderStatus::Expired;
}

// ---------------------------------------------------------------------------
// OrderManager
// ---------------------------------------------------------------------------

OrderManager::OrderManager(nexus::risk::RiskManager& risk_mgr, OnOrderUpdate on_update)
    : risk_mgr_(risk_mgr)
    , on_update_cb_(std::move(on_update)) {}

nexus::core::OrderId OrderManager::submit_order(nexus::core::InstrumentId instrument_id,
                                                 nexus::core::Side         side,
                                                 nexus::core::OrderType    type,
                                                 nexus::core::Price        price,
                                                 nexus::core::Quantity     qty,
                                                 nexus::core::Price        ref_price) {
    // Risk check (non-blocking — no mutex needed here)
    const auto result = risk_mgr_.check_order(instrument_id, side, price, qty, ref_price);
    if (result != nexus::risk::RiskCheckResult::Approved) {
        NEXUS_LOG_WARN("Order rejected by risk: ", nexus::risk::to_string(result));
        return 0;
    }

    // Allocate order ID
    const nexus::core::OrderId oid = next_order_id_.fetch_add(1, std::memory_order_relaxed);

    Order order;
    order.order_id      = oid;
    order.instrument_id = instrument_id;
    order.side          = side;
    order.type          = type;
    order.quantity      = qty;
    order.limit_price   = price;
    order.status        = nexus::core::OrderStatus::PendingNew;
    order.created_ts    = nexus::core::TimeUtil::epoch_ns();

    {
        std::lock_guard<std::mutex> lk(mutex_);
        orders_.emplace(oid, order);
    }

    // Notify risk that a new open order exists
    risk_mgr_.on_order_opened();

    // Transition to New and fire callback
    {
        std::lock_guard<std::mutex> lk(mutex_);
        auto& o  = orders_.at(oid);
        o.status = nexus::core::OrderStatus::New;
        on_update_cb_(o);
    }

    return oid;
}

bool OrderManager::cancel_order(nexus::core::OrderId order_id) {
    std::lock_guard<std::mutex> lk(mutex_);

    auto it = orders_.find(order_id);
    if (it == orders_.end()) return false;

    Order& o = it->second;
    if (o.is_terminal()) return false;

    o.status = nexus::core::OrderStatus::PendingCancel;
    on_update_cb_(o);

    o.status     = nexus::core::OrderStatus::Cancelled;
    o.updated_ts = nexus::core::TimeUtil::epoch_ns();
    on_update_cb_(o);

    risk_mgr_.on_order_closed();
    return true;
}

void OrderManager::on_fill(nexus::core::OrderId  order_id,
                            nexus::core::Price    fill_price,
                            nexus::core::Quantity fill_qty) {
    std::lock_guard<std::mutex> lk(mutex_);

    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        NEXUS_LOG_WARN("on_fill: unknown order_id=", order_id);
        return;
    }

    Order& o = it->second;

    // Running weighted-average fill price
    const nexus::core::Quantity prev_filled = o.filled_quantity;
    o.filled_quantity += fill_qty;
    o.updated_ts       = nexus::core::TimeUtil::epoch_ns();

    // avg_fill_price = (prev_avg * prev_filled + fill_price * fill_qty) / new_filled
    if (o.filled_quantity > 0) {
        const double new_avg = (o.avg_fill_price.to_double() * static_cast<double>(prev_filled) +
                                fill_price.to_double()       * static_cast<double>(fill_qty))
                               / static_cast<double>(o.filled_quantity);
        o.avg_fill_price = nexus::core::Price::from_double(new_avg);
    }

    if (o.filled_quantity >= o.quantity) {
        o.status = nexus::core::OrderStatus::Filled;
        risk_mgr_.on_order_filled(o.quantity, fill_price, o.side, o.avg_fill_price);
        risk_mgr_.on_order_closed();
    } else {
        o.status = nexus::core::OrderStatus::PartiallyFilled;
    }

    on_update_cb_(o);
}

void OrderManager::on_reject(nexus::core::OrderId order_id, std::string reason) {
    std::lock_guard<std::mutex> lk(mutex_);

    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        NEXUS_LOG_WARN("on_reject: unknown order_id=", order_id);
        return;
    }

    Order& o      = it->second;
    o.status       = nexus::core::OrderStatus::Rejected;
    o.reject_reason = std::move(reason);
    o.updated_ts   = nexus::core::TimeUtil::epoch_ns();

    on_update_cb_(o);
    risk_mgr_.on_order_closed();
}

const Order* OrderManager::find_order(nexus::core::OrderId order_id) const {
    std::lock_guard<std::mutex> lk(mutex_);

    auto it = orders_.find(order_id);
    if (it == orders_.end()) return nullptr;
    return &it->second;
}

std::size_t OrderManager::open_order_count() const noexcept {
    std::lock_guard<std::mutex> lk(mutex_);

    std::size_t count = 0;
    for (const auto& [id, order] : orders_) {
        if (!order.is_terminal()) ++count;
    }
    return count;
}

} // namespace nexus::oms
