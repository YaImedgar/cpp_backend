#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
  public:
    static Logger &GetInstance() {
        static Logger instance;
        return instance;
    }

    template <typename... Ts> void Log(const Ts &...args) {
        std::lock_guard lock(mutex_);
        std::ofstream log_file(GetLogFilePath(), std::ios::app);

        if (log_file.is_open()) {
            log_file << GetTimeStamp() << ": ";
            (log_file << ... << args) << std::endl;
        } else {
            std::cerr << "Failed to open log file: " << GetLogFilePath()
                      << std::endl;
        }
    }

    void SetTimestamp(std::chrono::system_clock::time_point timestamp) {
        std::lock_guard lock(mutex_);
        manual_timestamp_ = timestamp;
    }

  private:
    Logger() = default;
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
    Logger(Logger &&) = delete;
    Logger &operator=(Logger &&) = delete;

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }

    std::string GetLogFilePath() const {
        const auto now = GetTime();
        const auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm local_time = *std::localtime(&time_t);

        /* 10% slower then using snprintf
            std::stringstream ss;
            ss << "/var/log/sample_log_" << std::put_time(&local_time,
           "%Y_%m_%d")
            << ".log";
            return ss.str();
        */

        char buffer[35];
        std::snprintf(buffer, sizeof(buffer),
                      "/var/log/sample_log_%04d_%02d_%02d.log",
                      1900 + local_time.tm_year, 1 + local_time.tm_mon,
                      local_time.tm_mday);

        return std::string(buffer);
    }

    std::chrono::system_clock::time_point GetTime() const {
        if (manual_timestamp_) {
            return *manual_timestamp_;
        }
        return std::chrono::system_clock::now();
    }

    std::mutex mutex_;
    std::optional<std::chrono::system_clock::time_point> manual_timestamp_;
};