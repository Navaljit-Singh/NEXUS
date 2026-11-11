#pragma once

#include <nexus/core/types.hpp>
#include <nexus/core/time.hpp>
#include <nexus/core/spsc_queue.hpp>

#include <string_view>
#include <thread>
#include <atomic>
#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>

namespace nexus::core {

enum class LogLevel : std::uint8_t {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    CRITICAL = 4
};

inline const char* to_string(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRIT ";
        default: return "UNKN ";
    }
}

struct LogEntry {
    std::uint64_t timestamp_ns{0};
    LogLevel level{LogLevel::INFO};
    FixedString<256> message{};

    LogEntry() = default;
    LogEntry(std::uint64_t ts, LogLevel lvl, std::string_view msg)
        : timestamp_ns(ts), level(lvl), message(msg) {}
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void init(std::string_view log_file_path, LogLevel min_level = LogLevel::DEBUG, bool echo_console = true);
    void shutdown();

    void log(LogLevel level, std::string_view msg) noexcept;

    void set_level(LogLevel level) noexcept { min_level_ = level; }
    [[nodiscard]] LogLevel level() const noexcept { return min_level_; }

    template <typename... Args>
    void log_fmt(LogLevel level, Args&&... args) {
        if (level < min_level_) return;
        std::ostringstream oss;
        (oss << ... << std::forward<Args>(args));
        log(level, oss.str());
    }

private:
    Logger();
    ~Logger();

    void worker_loop();

    static constexpr std::size_t QUEUE_CAPACITY = 32768;
    std::unique_ptr<SPSCQueue<LogEntry, QUEUE_CAPACITY>> queue_;

    std::atomic<bool> running_{false};
    std::thread worker_thread_;
    std::ofstream file_stream_;
    LogLevel min_level_{LogLevel::DEBUG};
    bool echo_console_{true};
};

} // namespace nexus::core

// Convenience logging macros
#define NEXUS_LOG_DEBUG(...) ::nexus::core::Logger::instance().log_fmt(::nexus::core::LogLevel::DEBUG, __VA_ARGS__)
#define NEXUS_LOG_INFO(...)  ::nexus::core::Logger::instance().log_fmt(::nexus::core::LogLevel::INFO, __VA_ARGS__)
#define NEXUS_LOG_WARN(...)  ::nexus::core::Logger::instance().log_fmt(::nexus::core::LogLevel::WARN, __VA_ARGS__)
#define NEXUS_LOG_ERROR(...) ::nexus::core::Logger::instance().log_fmt(::nexus::core::LogLevel::ERROR, __VA_ARGS__)
#define NEXUS_LOG_CRIT(...)  ::nexus::core::Logger::instance().log_fmt(::nexus::core::LogLevel::CRITICAL, __VA_ARGS__)
