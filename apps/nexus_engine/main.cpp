#include "nexus/core/logger.hpp"
#include "nexus/core/types.hpp"
#include "nexus/market_data/upstox_feed.hpp"
#include "nexus/order_book/order_book.hpp"
#include "nexus/order_book/bar_aggregator.hpp"
#include "nexus/risk/risk_manager.hpp"
#include "nexus/oms/order_manager.hpp"
#include "nexus/strategy/sample_strategy.hpp"
#include "nexus/strategy/indicators.hpp"

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

// ---------------------------------------------------------------------------
// Global run flag and signal handler
// ---------------------------------------------------------------------------
static std::atomic<bool> g_running{true};

static void signal_handler(int /*signum*/) noexcept {
    g_running.store(false, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    // Install signal handlers
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // a. Init Logger
    nexus::core::Logger::instance().init("logs/nexus.log");

    nexus::core::Logger::instance().log(nexus::core::LogLevel::INFO,
                                        "NEXUS engine starting up");

    // b. Create RiskManager with default RiskLimits
    nexus::risk::RiskLimits limits{};
    nexus::risk::RiskManager risk_manager{limits};

    // c. Create OrderManager with the risk manager and an order update callback
    nexus::oms::OrderManager order_manager{
        risk_manager,
        [](const nexus::oms::Order& order) {
            nexus::core::Logger::instance().log_fmt(
                nexus::core::LogLevel::INFO,
                "Order update: ID=", order.order_id,
                " status=", to_string(order.status));
        }
    };

    // d. Create EMACrossStrategy
    nexus::strategy::EMACrossStrategy strategy{};

    // e. Create OrderBook for instrument_id=1
    nexus::order_book::OrderBook order_book{1};

    // f. Create BarAggregator for instrument_id=1, interval=60s, prints bar on close
    nexus::order_book::BarAggregator bar_agg{
        1,  // instrument_id
        60, // interval_seconds
        [](const nexus::core::BarData& bar) {
            std::cout << "[BAR] open=" << bar.open
                      << " high=" << bar.high
                      << " low="  << bar.low
                      << " close=" << bar.close
                      << " vol="  << bar.volume << '\n';
        }
    };

    // g. Create UpstoxFeedClient
    nexus::market_data::UpstoxFeedClient feed_client{
        "STUB",
        {"NSE_EQ|INE002A01018"},
        [&order_book, &bar_agg, &strategy](const nexus::core::MarketEvent& event) {
            // Dispatch per event type
            std::visit([&order_book, &bar_agg, &strategy](auto&& ev) {
                using T = std::decay_t<decltype(ev)>;
                if constexpr (std::is_same_v<T, nexus::core::BookUpdate>) {
                    // Update BBO from first bid/ask levels
                    if (!ev.bids.empty() && !ev.asks.empty()) {
                        order_book.update_bbo(
                            ev.bids[0].first, ev.bids[0].second,
                            ev.asks[0].first, ev.asks[0].second);
                    }
                } else if constexpr (std::is_same_v<T, nexus::core::TickData>) {
                    bar_agg.on_tick(ev);
                }
                // strategy receives all events (convert back to MarketEvent)
            }, event);

            // strategy.on_market_event for all events
            strategy.on_market_event(event);
        }
    };

    // h. Connect
    feed_client.connect();

    nexus::core::Logger::instance().log(nexus::core::LogLevel::INFO,
                                        "NEXUS engine running. Press Ctrl+C to stop.");

    // i. Main loop
    while (g_running.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }

    nexus::core::Logger::instance().log(nexus::core::LogLevel::INFO,
                                        "Shutting down NEXUS engine...");

    // j. Disconnect feed
    feed_client.disconnect();

    // k. Shutdown logger
    nexus::core::Logger::instance().shutdown();

    // l. Done
    return 0;
}
