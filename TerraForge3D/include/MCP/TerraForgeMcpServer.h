#pragma once

#include "MCP/ActionRegistry.h"
#include "MCP/MainThreadRequestQueue.h"
#include "MCP/ResourceRegistry.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class ApplicationState;

namespace mcp
{
    class server;
}

struct McpCommandLogEntry {
    std::string timestamp;
    std::string method;
    std::string parameters;
    std::string requestJson;
};

struct McpServerStats {
    bool running                 = false;
    std::string host             = "127.0.0.1";
    int port                     = 9823;
    std::string endpoint         = "http://127.0.0.1:9823/mcp";
    std::uint64_t commandCount   = 0;
    std::uint64_t activeSessions = 0;
    std::vector<McpCommandLogEntry> commandLog;
};

class TerraForgeMcpServer
{
public:
    TerraForgeMcpServer(ApplicationState *applicationState, std::string logsDirectory);
    ~TerraForgeMcpServer();

    TerraForgeMcpServer(const TerraForgeMcpServer &)            = delete;
    TerraForgeMcpServer &operator=(const TerraForgeMcpServer &) = delete;

    bool Start();
    void Update();
    void Stop();

    bool IsRunning() const;
    McpServerStats GetStats() const;
    void ClearCommandLog();

private:
    void OpenCallHistory();
    void CloseCallHistory();
    void RecordCommand(
        const std::string &method,
        const nlohmann::json &params,
        const std::string &sessionId);

    std::string logsDirectory;
    const std::string host = "127.0.0.1";
    int port               = 9823;
    std::string endpoint;
    ActionRegistry actions;
    ResourceRegistry resources;
    MainThreadRequestQueue requestQueue;
    std::unique_ptr<mcp::server> server;
    std::ofstream callHistory;
    std::filesystem::path callHistoryPath;
    mutable std::mutex statsMutex;
    std::uint64_t commandCount = 0;
    std::vector<McpCommandLogEntry> commandLog;
};
