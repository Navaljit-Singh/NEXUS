#pragma once

#include <nexus/core/event.hpp>
#include <nexus/core/types.hpp>
#include <string>

// Forward declare Order to avoid pulling in the full OMS header here.
namespace nexus::oms {
class Order;
} // namespace nexus::oms

namespace nexus::strategy {

struct SignalResult {
    enum class Action : uint8_t { Hold = 0, Buy = 1, Sell = 2 } action{Action::Hold};
    nexus::core::Price    target_price{};
    nexus::core::Quantity quantity{0};
    std::string           reason{};
};

class StrategyBase {
public:
    explicit StrategyBase(std::string name) : name_(std::move(name)) {}
    virtual ~StrategyBase() = default;

    virtual void on_tick(const nexus::core::TickData& tick)          = 0;
    virtual void on_bar(const nexus::core::BarData& bar)             = 0;
    virtual void on_book_update(const nexus::core::BookUpdate& update) = 0;
    virtual void on_order_update(const nexus::oms::Order& order)     = 0;

    // Default implementation dispatches via std::visit; defined in strategy_base.cpp
    virtual void on_market_event(const nexus::core::MarketEvent& event);

    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    virtual void start() {}
    virtual void stop()  {}

protected:
    std::string name_;
};

} // namespace nexus::strategy
