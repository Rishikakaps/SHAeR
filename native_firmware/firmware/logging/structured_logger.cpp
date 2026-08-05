#include "structured_logger.hpp"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <utility>

namespace shaer {

StructuredLogger::StructuredLogger(std::string log_directory)
    : log_directory_(std::move(log_directory)) {}

StructuredLogger::~StructuredLogger() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (system_log_.is_open()) {
        system_log_.flush();
        system_log_.close();
    }
}

bool StructuredLogger::open() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::error_code ec;
    std::filesystem::create_directories(log_directory_, ec);
    if (ec) {
        return false;
    }
    system_log_.open(system_log_path(), std::ios::app);
    return system_log_.good();
}

void StructuredLogger::log(
    LogLevel level,
    const std::string& channel,
    const std::string& event,
    const std::string& message,
    const std::map<std::string, std::string>& fields) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!system_log_.is_open()) {
        return;
    }
    system_log_ << line_for(level, channel, event, message, fields) << '\n';
    system_log_.flush();
}

std::string StructuredLogger::directory() const {
    return log_directory_;
}

std::string StructuredLogger::system_log_path() const {
    return log_directory_ + "/system.log";
}

std::string StructuredLogger::timestamp_utc() const {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &time);
#else
    gmtime_r(&time, &tm);
#endif
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

std::string StructuredLogger::level_name(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug: return "debug";
        case LogLevel::Info: return "info";
        case LogLevel::Warning: return "warning";
        case LogLevel::Error: return "error";
        case LogLevel::Critical: return "critical";
    }
    return "unknown";
}

std::string StructuredLogger::escape(const std::string& value) const {
    std::string escaped;
    escaped.reserve(value.size());
    for (char c : value) {
        if (c == '\\' || c == '"') {
            escaped.push_back('\\');
        }
        if (c == '\n') {
            escaped += "\\n";
        } else if (c != '\r') {
            escaped.push_back(c);
        }
    }
    return escaped;
}

std::string StructuredLogger::line_for(
    LogLevel level,
    const std::string& channel,
    const std::string& event,
    const std::string& message,
    const std::map<std::string, std::string>& fields) const {
    std::ostringstream out;
    out << "{\"ts\":\"" << timestamp_utc()
        << "\",\"level\":\"" << level_name(level)
        << "\",\"channel\":\"" << escape(channel)
        << "\",\"event\":\"" << escape(event)
        << "\",\"message\":\"" << escape(message) << "\"";
    for (const auto& field : fields) {
        out << ",\"" << escape(field.first) << "\":\"" << escape(field.second) << "\"";
    }
    out << "}";
    return out.str();
}

}  // namespace shaer
