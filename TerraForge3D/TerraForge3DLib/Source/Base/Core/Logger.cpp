#include "Base/Core/Logger.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>


namespace TerraForge3D
{

	Logger* Logger::mainInstance;

	Logger::Logger(std::string filePath, std::string name)
	{
		std::vector<spdlog::sink_ptr> logSinks;
		logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
		logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(filePath, true));

		logSinks[0]->set_pattern("%^[%T] %n: %v%$");
		logSinks[1]->set_pattern("[%T] [%l] %n: %v");

		mainLogger = std::make_shared<spdlog::logger>(name, begin(logSinks), end(logSinks));
		spdlog::register_logger(mainLogger);
		mainLogger->set_level(spdlog::level::trace);
		mainLogger->flush_on(spdlog::level::trace);

		mainLogger->info("Logger Initialized ({0})", filePath);
	}

	Logger::~Logger()
	{
		mainLogger->info("Logger Shutdown");
	}

	
}