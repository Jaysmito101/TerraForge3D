#pragma once

#ifndef MCP_LOGGER_H
#define MCP_LOGGER_H

#include <functional>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>

namespace mcp
{
	enum class log_level
	{
		debug,
		info,
		warning,
		error
	};

	class logger
	{
	public:
		using sink = std::function<void(log_level, const std::string&)>;

		static logger& instance()
		{
			static logger instance;
			return instance;
		}

		void set_level(log_level level)
		{
			std::lock_guard<std::mutex> lock(mutex);
			this->level = level;
		}

		void set_sink(sink value)
		{
			std::lock_guard<std::mutex> lock(mutex);
			logSink = std::move(value);
		}

		template<typename... Args>
		void debug(Args&&... args)
		{
			Log(log_level::debug, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void info(Args&&... args)
		{
			Log(log_level::info, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void warning(Args&&... args)
		{
			Log(log_level::warning, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void error(Args&&... args)
		{
			Log(log_level::error, std::forward<Args>(args)...);
		}

	private:
		logger() = default;

		template<typename T>
		static void Append(std::stringstream& stream, T&& value)
		{
			stream << std::forward<T>(value);
		}

		template<typename T, typename... Args>
		static void Append(std::stringstream& stream, T&& value, Args&&... args)
		{
			stream << std::forward<T>(value);
			Append(stream, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void Log(log_level messageLevel, Args&&... args)
		{
			log_level configuredLevel;
			{
				std::lock_guard<std::mutex> lock(mutex);
				configuredLevel = level;
			}
			if (messageLevel < configuredLevel) return;

			std::stringstream message;
			Append(message, std::forward<Args>(args)...);

			sink destination;
			{
				std::lock_guard<std::mutex> lock(mutex);
				destination = logSink;
			}
			if (destination) destination(messageLevel, message.str());
		}

		log_level level = log_level::info;
		sink logSink;
		std::mutex mutex;
	};

	using log_sink = logger::sink;

	inline void set_log_sink(log_sink sink)
	{
		logger::instance().set_sink(std::move(sink));
	}

	inline void set_log_level(log_level level)
	{
		logger::instance().set_level(level);
	}
}

#define LOG_DEBUG(...) mcp::logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...) mcp::logger::instance().info(__VA_ARGS__)
#define LOG_WARNING(...) mcp::logger::instance().warning(__VA_ARGS__)
#define LOG_ERROR(...) mcp::logger::instance().error(__VA_ARGS__)

#endif
