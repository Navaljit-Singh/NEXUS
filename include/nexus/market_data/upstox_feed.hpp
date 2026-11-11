#pragma once

#include "nexus/core/event.hpp"
#include "nexus/core/logger.hpp"
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <cstdint>

namespace nexus::market_data {

class UpstoxFeedClient {
public:
    UpstoxFeedClient(
        std::string auth_token,
        std::vector<std::string> subscriptions,
        std::function<void(const nexus::core::MarketEvent&)> on_event);

    ~UpstoxFeedClient();

    void connect();
    void disconnect();
    [[nodiscard]] bool is_connected() const noexcept;

private:
    void ws_loop();
    void parse_message(const std::uint8_t* data, std::size_t len); // TODO: integrate actual Upstox Protobuf decode

    std::string auth_token_;
    std::string ws_url_{"wss://api.upstox.com/v2/feed/market-data-feed"};
    std::vector<std::string> subscriptions_;
    std::function<void(const nexus::core::MarketEvent&)> on_event_cb_;
    std::atomic<bool> connected_{false};
    std::thread ws_thread_;
};

} // namespace nexus::market_data
