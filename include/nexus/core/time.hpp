#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <array>
#include <ctime>
#include <iomanip>
#include <sstream>

#if defined(__x86_64__) || defined(_M_X64)
#include <x86intrin.h>
#endif

namespace nexus::core {

class TimeUtil {
public:
    // High-resolution monotonic clock in nanoseconds
    [[nodiscard]] static inline std::uint64_t monotonic_ns() noexcept {
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count()
        );
    }

    // High-resolution real-world wall clock (epoch nanoseconds)
    [[nodiscard]] static inline std::uint64_t epoch_ns() noexcept {
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
    }

    // High-resolution real-world wall clock (epoch microseconds)
    [[nodiscard]] static inline std::uint64_t epoch_us() noexcept {
        return epoch_ns() / 1000ULL;
    }

    // Low-overhead CPU cycle counter (RDTSC)
    [[nodiscard]] static inline std::uint64_t rdtsc() noexcept {
#if defined(__x86_64__) || defined(_M_X64)
        return __rdtsc();
#else
        return monotonic_ns();
#endif
    }

    // Format epoch nanoseconds into UTC ISO8601 string: YYYY-MM-DDTHH:MM:SS.mmmmmmZ
    static void format_timestamp(std::uint64_t epoch_ns_val, char* out_buf, std::size_t buf_len) noexcept {
        if (buf_len < 32) return;
        const auto epoch_s = static_cast<std::time_t>(epoch_ns_val / 1000000000ULL);
        const auto us_remainder = static_cast<std::uint32_t>((epoch_ns_val / 1000ULL) % 1000000ULL);

        std::tm tm_buf{};
#if defined(_WIN32)
        gmtime_s(&tm_buf, &epoch_s);
#else
        gmtime_r(&epoch_s, &tm_buf);
#endif
        std::snprintf(out_buf, buf_len, "%04d-%02d-%02dT%02d:%02d:%02d.%06uZ",
                      tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
                      tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, us_remainder);
    }
};

// RAII Latency Timer for benchmarking hot paths
template <typename Callback>
class ScopedTimer {
public:
    explicit ScopedTimer(Callback cb) : callback_(std::move(cb)), start_ns_(TimeUtil::monotonic_ns()) {}

    ~ScopedTimer() {
        const auto elapsed = TimeUtil::monotonic_ns() - start_ns_;
        callback_(elapsed);
    }

private:
    Callback callback_;
    std::uint64_t start_ns_;
};

} // namespace nexus::core
