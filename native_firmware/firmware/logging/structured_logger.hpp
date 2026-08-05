#pragma once

#include <fstream>
#include <map>
#include <mutex>
#include <string>

namespace shaer {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Critical,
};

class StructuredLogger {
public:
    explicit StructuredLogger(std::string log_directory);
    ~StructuredLogger();

    bool open();
    void log(
        LogLevel level,
        const std::string& channel,
        const std::string& event,
        const std::string& message,
        const std::map<std::string, std::string>& fields = {});

    std::string directory() const;
    std::string system_log_path() const;

private:
    std::string timestamp_utc() const;
    std::string level_name(LogLevel level) const;
    std::string escape(const std::string& value) const;
    std::string line_for(
        LogLevel level,
        const std::string& channel,
        const std::string& event,
        const std::string& message,
        const std::map<std::string, std::string>& fields) const;

    std::string log_directory_;
    std::ofstream system_log_;
    mutable std::mutex mutex_;
};

}  // namespace shaer

