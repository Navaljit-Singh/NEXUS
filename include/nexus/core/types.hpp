#pragma once

#include <cstdint>
#include <string_view>
#include <cstring>
#include <array>
#include <ostream>
#include <compare>

namespace nexus::core {

// Cache line size for modern x86_64 architectures to prevent false sharing
inline constexpr std::size_t CACHE_LINE_SIZE = 64;

// Scaled integer price representation (4 decimal places: 123.45 -> 1234500)
// Prevents IEEE 754 floating point rounding errors on the hot path
using PriceRaw = std::int64_t;
using Quantity = std::uint32_t;
using OrderId = std::uint64_t;
using InstrumentId = std::uint32_t;
using TimestampNs = std::uint64_t;

inline constexpr PriceRaw PRICE_SCALE = 10000;

struct Price {
    PriceRaw raw{0};

    constexpr Price() noexcept = default;
    constexpr explicit Price(PriceRaw r) noexcept : raw(r) {}

    static constexpr Price from_double(double value) noexcept {
        return Price(static_cast<PriceRaw>(value * static_cast<double>(PRICE_SCALE) + 0.5));
    }

    constexpr double to_double() const noexcept {
        return static_cast<double>(raw) / static_cast<double>(PRICE_SCALE);
    }

    constexpr auto operator<=>(const Price&) const noexcept = default;

    constexpr Price operator+(const Price& other) const noexcept { return Price(raw + other.raw); }
    constexpr Price operator-(const Price& other) const noexcept { return Price(raw - other.raw); }
    constexpr Price operator*(int64_t factor) const noexcept { return Price(raw * factor); }
    constexpr Price operator/(int64_t divisor) const noexcept { return Price(raw / divisor); }

    friend std::ostream& operator<<(std::ostream& os, const Price& p) {
        os << p.to_double();
        return os;
    }
};

enum class Side : std::uint8_t {
    Buy = 0,
    Sell = 1,
    Unknown = 255
};

inline const char* to_string(Side side) noexcept {
    switch (side) {
        case Side::Buy: return "BUY";
        case Side::Sell: return "SELL";
        default: return "UNKNOWN";
    }
}

enum class OrderType : std::uint8_t {
    Limit = 0,
    Market = 1,
    StopLoss = 2,
    StopLossLimit = 3
};

inline const char* to_string(OrderType type) noexcept {
    switch (type) {
        case OrderType::Limit: return "LIMIT";
        case OrderType::Market: return "MARKET";
        case OrderType::StopLoss: return "STOP_LOSS";
        case OrderType::StopLossLimit: return "STOP_LOSS_LIMIT";
        default: return "UNKNOWN";
    }
}

enum class TimeInForce : std::uint8_t {
    Day = 0,
    IOC = 1,    // Immediate Or Cancel
    FOK = 2     // Fill Or Kill
};

enum class OrderStatus : std::uint8_t {
    PendingNew = 0,
    New = 1,
    PartiallyFilled = 2,
    Filled = 3,
    PendingCancel = 4,
    Cancelled = 5,
    Rejected = 6,
    Expired = 7
};

inline const char* to_string(OrderStatus status) noexcept {
    switch (status) {
        case OrderStatus::PendingNew: return "PENDING_NEW";
        case OrderStatus::New: return "NEW";
        case OrderStatus::PartiallyFilled: return "PARTIALLY_FILLED";
        case OrderStatus::Filled: return "FILLED";
        case OrderStatus::PendingCancel: return "PENDING_CANCEL";
        case OrderStatus::Cancelled: return "CANCELLED";
        case OrderStatus::Rejected: return "REJECTED";
        case OrderStatus::Expired: return "EXPIRED";
        default: return "UNKNOWN";
    }
}

// Zero-allocation stack-allocated fixed-length string for symbols (e.g. "NSE_EQ|INE002A01018")
template <std::size_t N = 32>
struct FixedString {
    std::array<char, N> data{};
    std::uint8_t length{0};

    constexpr FixedString() noexcept {
        data.fill('\0');
    }

    constexpr FixedString(std::string_view sv) noexcept {
        length = static_cast<std::uint8_t>(std::min(sv.size(), N - 1));
        for (std::size_t i = 0; i < length; ++i) {
            data[i] = sv[i];
        }
        data[length] = '\0';
    }

    constexpr std::string_view view() const noexcept {
        return std::string_view(data.data(), length);
    }

    constexpr const char* c_str() const noexcept {
        return data.data();
    }

    constexpr bool operator==(const FixedString& other) const noexcept {
        return view() == other.view();
    }

    constexpr auto operator<=>(const FixedString& other) const noexcept {
        return view() <=> other.view();
    }

    friend std::ostream& operator<<(std::ostream& os, const FixedString& fs) {
        os << fs.view();
        return os;
    }
};

using Symbol = FixedString<32>;

} // namespace nexus::core
