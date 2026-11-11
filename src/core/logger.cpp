#include "nexus/core/logger.hpp"
#include "nexus/core/time.hpp"
#include <cstdio>
#include <chrono>
#include <thread>
#include <string>

namespace nexus::core {

Logger::Logger()
    : queue_(std::make_unique<SPSCQueue<LogEntry, QUEUE_CAPACITY>>()) {}

Logger::~Logger() {
    shutdown();
}

void Logger::init(std::string_view log_file_path, LogLevel min_level, bool echo_console) {
    min_level_    = min_level;
    echo_console_ = echo_console;
    file_stream_.open(std::string(log_file_path), std::ios::out | std::ios::app);
    running_       = true;
    worker_thread_ = std::thread(&Logger::worker_loop, this);
}

void Logger::shutdown() {
    if (running_) {
        running_ = false;
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
        if (file_stream_.is_open()) {
            file_stream_.close();
        }
    }
}

void Logger::log(LogLevel level, std::string_view msg) noexcept {
    if (level < min_level_) return;
    LogEntry entry{TimeUtil::epoch_ns(), level, msg};
    (void)queue_->push(entry);
}

void Logger::worker_loop() {
    while (running_) {
        LogEntry e;
        if (queue_->pop(e)) {
            char ts_buf[64]{};
            TimeUtil::format_timestamp(e.timestamp_ns, ts_buf, sizeof(ts_buf));

            char line[512]{};
            std::snprintf(line, sizeof(line), "[%s] [%s] %s\n",
                          ts_buf, to_string(e.level), e.message.c_str());

            if (file_stream_.is_open()) {
                file_stream_ << line;
                file_stream_.flush();
            }
            if (echo_console_) {
                std::fputs(line, stdout);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds{10});
        }
    }
}

} // namespace nexus::core
