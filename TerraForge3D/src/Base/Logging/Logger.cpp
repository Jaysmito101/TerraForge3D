#include "Base/Logging/Logger.h"
#include "Platform.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <sstream>

namespace tf3d::base
{

    Logger::Logger(std::string logsDir)
    {
        std::filesystem::create_directories(logsDir);
        const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm localTime{};
#ifdef TERR3D_WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif
        std::ostringstream filename;
        filename << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S") << ".log";
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto fileSink    = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logsDir + PATH_SEPARATOR + filename.str(), true);
        mLogger          = std::make_shared<spdlog::logger>("terraforge3d", spdlog::sinks_init_list{consoleSink, fileSink});
        mLogger->set_level(spdlog::level::trace);
        mLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
        spdlog::set_default_logger(mLogger);
        spdlog::flush_on(spdlog::level::warn);
        mLogger->info("Logger started; file={}", logsDir + PATH_SEPARATOR + filename.str());
    }

    Logger::~Logger()
    {
        if (mLogger) {
            mLogger->info("Logger shutting down");
            mLogger->flush();
            spdlog::drop(mLogger->name());
        }
    }

} // namespace tf3d::base
