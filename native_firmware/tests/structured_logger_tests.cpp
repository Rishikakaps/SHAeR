#include "structured_logger.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>

int main() {
    const std::string dir = "/tmp/shaer_logger_test_" + std::to_string(getpid());
    std::filesystem::remove_all(dir);

    shaer::StructuredLogger logger(dir);
    assert(logger.open());
    logger.log(shaer::LogLevel::Info, "system", "unit_test", "logger works", {
        {"boot_id", "test"},
    });

    std::ifstream file(logger.system_log_path());
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string contents = buffer.str();
    assert(contents.find("\"channel\":\"system\"") != std::string::npos);
    assert(contents.find("\"event\":\"unit_test\"") != std::string::npos);
    assert(contents.find("\"boot_id\":\"test\"") != std::string::npos);

    std::filesystem::remove_all(dir);
    return 0;
}

