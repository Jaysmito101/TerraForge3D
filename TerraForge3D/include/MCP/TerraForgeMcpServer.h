#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class ApplicationState;

	struct McpCommandLogEntry
	{
		std::string timestamp;
		std::string method;
		std::string parameters;
		std::string requestJson;
	};

	struct McpServerStats
	{
		bool running = false;
		std::string host = "127.0.0.1";
		int port = 9823;
		std::string endpoint = "http://127.0.0.1:9823/mcp";
		std::uint64_t commandCount = 0;
		std::uint64_t activeSessions = 0;
		std::vector<McpCommandLogEntry> commandLog;
	};

	class TerraForgeMcpServer
	{
	public:
		explicit TerraForgeMcpServer(ApplicationState* applicationState);
		~TerraForgeMcpServer();

		TerraForgeMcpServer(const TerraForgeMcpServer&) = delete;
		TerraForgeMcpServer& operator=(const TerraForgeMcpServer&) = delete;

		bool Start();
		void Update();
		void Stop();
		
		bool IsRunning() const;
		McpServerStats GetStats() const;
		void ClearCommandLog();

	private:
		class Impl;
		std::unique_ptr<Impl> implementation;
	};
