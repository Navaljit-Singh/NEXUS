#pragma once

#include <cstdint>
#include "nexus/core/types.hpp"

namespace nexus::order_book {

struct PriceLevel {
    nexus::core::Price    price{};
    nexus::core::Quantity total_quantity{0};
    std::uint32_t         order_count{0};

    bool empty() const noexcept { return total_quantity == 0; }
};

} // namespace nexus::order_book
