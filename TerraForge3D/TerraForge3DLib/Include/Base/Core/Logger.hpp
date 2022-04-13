#pragma once

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include "spdlog/spdlog.h"
#pragma warning(pop)

#include <string>
#include <memory>

namespace TerraForge3D
{

	/*
	* This class is to manager the logging via SPD Log
	*/
	class Logger
	{
	private:
		Logger(std::string filePath = "TerraForge3D.log", std::string name = "TerraForge3D");

	public:
		~Logger();

		static Logger* Create(std::string filePath = "TerraForge3D.log");
		static Logger* Get();
	public:
		std::shared_ptr<spdlog::logger> mainLogger;

	private:
		static Logger* mainInstance;
	};

}

#ifndef TF3D_DISABLE_LOGGING

#define TF3D_LOG(...)			TF3D_LOG_INFO(__VA_ARGS__)
#define TF3D_LOG_INFO(...)		::TerraForge3D::Logger::Get()->mainLogger->info(__VA_ARGS__);
#define TF3D_LOG_TRACE(...)		::TerraForge3D::Logger::Get()->mainLogger->trace(__VA_ARGS__);
#define TF3D_LOG_WARN(...)		::TerraForge3D::Logger::Get()->mainLogger->warn(__VA_ARGS__);
#define TF3D_LOG_ERROR(...)		::TerraForge3D::Logger::Get()->mainLogger->error(__VA_ARGS__);
#define TF3D_LOG_CRITICAL(...)	::TerraForge3D::Logger::Get()->mainLogger->critical(__VA_ARGS__);

#else

#define TF3D_LOG_INFO(...)
#define TF3D_LOG_TRACE(...)
#define TF3D_LOG_WARN(...)
#define TF3D_LOG_ERROR(...)
#define TF3D_LOG_CRITICAL(...)

#endif