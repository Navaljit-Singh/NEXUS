#include "nexus/market_data/upstox_feed.hpp"
#include <chrono>
#include <random>
#include <cstdint>

namespace nexus::market_data {

UpstoxFeedClient::UpstoxFeedClient(
    std::string auth_token,
    std::vector<std::string> subscriptions,
    std::function<void(const nexus::core::MarketEvent&)> on_event)
    : auth_token_(std::move(auth_token))
    , subscriptions_(std::move(subscriptions))
    , on_event_cb_(std::move(on_event)) {}

UpstoxFeedClient::~UpstoxFeedClient() {
    disconnect();
}

void UpstoxFeedClient::connect() {
    connected_ = true;
    ws_thread_ = std::thread(&UpstoxFeedClient::ws_loop, this);
}

void UpstoxFeedClient::disconnect() {
    connected_ = false;
    if (ws_thread_.joinable()) {
        ws_thread_.join();
    }
}

bool UpstoxFeedClient::is_connected() const noexcept {
    return connected_.load(std::memory_order_relaxed);
}

void UpstoxFeedClient::ws_loop() {
    std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<double> price_dist{100.0, 200.0};

    while (connected_) {
        nexus::core::TickData tick{};
        tick.instrument_id = 1;
        const double p     = price_dist(rng);
        tick.ltp           = nexus::core::Price::from_double(p);
        tick.bid           = nexus::core::Price::from_double(p - 0.05);
        tick.ask           = nexus::core::Price::from_double(p + 0.05);
        tick.volume        = 100;

        on_event_cb_(nexus::core::MarketEvent{tick});

        std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }
}

void UpstoxFeedClient::parse_message(const std::uint8_t* /*data*/, std::size_t /*len*/) {
    // TODO: integrate actual Upstox Protobuf decode
}

} // namespace nexus::market_data
