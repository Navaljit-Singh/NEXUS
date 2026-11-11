#pragma once

#include <nexus/core/types.hpp>
#include <deque>
#include <cstdint>
#include <cmath>
#include <numeric>

namespace nexus::strategy {

// ---------------------------------------------------------------------------
// EMA – Exponential Moving Average
// ---------------------------------------------------------------------------
class EMA {
public:
    explicit EMA(std::size_t period)
        : period_(period),
          alpha_(2.0 / (static_cast<double>(period) + 1.0)),
          initialized_(false),
          value_(0.0) {}

    void update(double price) {
        if (!initialized_) {
            value_       = price;
            initialized_ = true;
        } else {
            value_ = alpha_ * price + (1.0 - alpha_) * value_;
        }
    }

    [[nodiscard]] double value()    const noexcept { return value_; }
    [[nodiscard]] bool   is_ready() const noexcept { return initialized_; }

private:
    std::size_t period_;
    double      alpha_;
    bool        initialized_;
    double      value_;
};

// ---------------------------------------------------------------------------
// SMA – Simple Moving Average
// ---------------------------------------------------------------------------
class SMA {
public:
    explicit SMA(std::size_t period) : period_(period), sum_(0.0) {}

    void update(double price) {
        window_.push_back(price);
        sum_ += price;
        if (window_.size() > period_) {
            sum_ -= window_.front();
            window_.pop_front();
        }
    }

    [[nodiscard]] double value() const noexcept {
        return window_.empty() ? 0.0 : sum_ / static_cast<double>(window_.size());
    }
    [[nodiscard]] bool is_ready() const noexcept { return window_.size() >= period_; }

private:
    std::size_t         period_;
    double              sum_;
    std::deque<double>  window_;
};

// ---------------------------------------------------------------------------
// RSI – Relative Strength Index (Wilder's smoothing method)
// ---------------------------------------------------------------------------
class RSI {
public:
    explicit RSI(std::size_t period = 14)
        : period_(period),
          avg_gain_(0.0),
          avg_loss_(0.0),
          last_price_(0.0),
          count_(0) {}

    void update(double price) {
        if (count_ == 0) {
            last_price_ = price;
            count_++;
            return;
        }
        double change     = price - last_price_;
        last_price_       = price;
        count_++;
        double gain = (change > 0.0) ? change  : 0.0;
        double loss = (change < 0.0) ? -change : 0.0;
        if (count_ <= period_ + 1) {
            avg_gain_ += gain / static_cast<double>(period_);
            avg_loss_ += loss / static_cast<double>(period_);
        } else {
            avg_gain_ = (avg_gain_ * static_cast<double>(period_ - 1) + gain) / static_cast<double>(period_);
            avg_loss_ = (avg_loss_ * static_cast<double>(period_ - 1) + loss) / static_cast<double>(period_);
        }
    }

    [[nodiscard]] double value() const noexcept {
        if (avg_loss_ < 1e-10) return 100.0;
        return 100.0 - 100.0 / (1.0 + avg_gain_ / avg_loss_);
    }
    [[nodiscard]] bool is_ready() const noexcept { return count_ > period_; }

private:
    std::size_t period_;
    double      avg_gain_;
    double      avg_loss_;
    double      last_price_;
    std::size_t count_;
};

// ---------------------------------------------------------------------------
// VWAP – Volume-Weighted Average Price (session-level, reset each session)
// ---------------------------------------------------------------------------
class VWAP {
public:
    VWAP() : cumulative_pv_(0.0), cumulative_volume_(0.0) {}

    void update(double price, double volume) {
        cumulative_pv_     += price * volume;
        cumulative_volume_ += volume;
    }

    void reset() {
        cumulative_pv_     = 0.0;
        cumulative_volume_ = 0.0;
    }

    [[nodiscard]] double value() const noexcept {
        return (cumulative_volume_ < 1e-10) ? 0.0 : cumulative_pv_ / cumulative_volume_;
    }
    [[nodiscard]] bool is_ready() const noexcept { return cumulative_volume_ > 0.0; }

private:
    double cumulative_pv_;
    double cumulative_volume_;
};

} // namespace nexus::strategy
