#pragma once

#include <memory>
#include <spdlog/spdlog.h>
#include <string>

namespace spdlog
{
    class logger;
}

class Logger
{
public:
    Logger(std::string logDir);
    ~Logger();

    Logger(const Logger &)            = delete;
    Logger &operator=(const Logger &) = delete;

private:
    std::shared_ptr<spdlog::logger> mLogger;
};

#define TF3D_LOG_TRACE(...)    SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::trace, __VA_ARGS__)
#define TF3D_LOG_DEBUG(...)    SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::debug, __VA_ARGS__)
#define TF3D_LOG_INFO(...)     SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::info, __VA_ARGS__)
#define TF3D_LOG_WARN(...)     SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::warn, __VA_ARGS__)
#define TF3D_LOG_ERROR(...)    SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::err, __VA_ARGS__)
#define TF3D_LOG_CRITICAL(...) SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::critical, __VA_ARGS__)

#define Log(...)               TF3D_LOG_INFO(__VA_ARGS__)
