#include <nexus/strategy/strategy_base.hpp>

namespace nexus::strategy {

void StrategyBase::on_market_event(const nexus::core::MarketEvent& event) {
    std::visit([this](const auto& e) {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, nexus::core::TickData>) {
            on_tick(e);
        } else if constexpr (std::is_same_v<T, nexus::core::BarData>) {
            on_bar(e);
        } else if constexpr (std::is_same_v<T, nexus::core::BookUpdate>) {
            on_book_update(e);
        }
    }, event);
}

} // namespace nexus::strategy
