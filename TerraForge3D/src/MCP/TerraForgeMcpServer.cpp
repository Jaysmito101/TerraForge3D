#include "mcp_server.h"

#include "MCP/Resources/CoreResources.h"
#include "MCP/TerraForgeMcpServer.h"
#include "MCP/Tools/CoreTools.h"
#include "MCP/McpTransport.h"

#include "Base/Logging/Logger.h"
#include "Data/VersionInfo.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <utility>

namespace
{
    int ReadMcpPort()
    {
        constexpr int defaultPort  = 9823;
        const char *configuredPort = std::getenv("TF3D_MCP_PORT");
        if (configuredPort == nullptr || configuredPort[0] == '\0')
            return defaultPort;

        char *end             = nullptr;
        const long parsedPort = std::strtol(configuredPort, &end, 10);
        if (end == configuredPort || *end != '\0' || parsedPort < 1 || parsedPort > 65535) {
            TF3D_LOG_WARN("Ignoring invalid TF3D_MCP_PORT='{}'; using {}", configuredPort, defaultPort);
            return defaultPort;
        }
        return static_cast<int>(parsedPort);
    }

    std::string FormatCommandTimestamp()
    {
        const auto now         = std::chrono::system_clock::now();
        const std::time_t time = std::chrono::system_clock::to_time_t(now);
        std::tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &time);
#else
        localtime_r(&time, &localTime);
#endif
        std::ostringstream formatted;
        formatted << std::put_time(&localTime, "%H:%M:%S");
        return formatted.str();
    }

    void InstallMcpLogBridge()
    {
        mcp::set_log_sink([](mcp::log_level level, const std::string &message) {
            switch (level) {
                case mcp::log_level::debug:
                    TF3D_LOG_DEBUG("[MCP] {}", message);
                    break;
                case mcp::log_level::info:
                    TF3D_LOG_INFO("[MCP] {}", message);
                    break;
                case mcp::log_level::warning:
                    TF3D_LOG_WARN("[MCP] {}", message);
                    break;
                case mcp::log_level::error:
                    TF3D_LOG_ERROR("[MCP] {}", message);
                    break;
            }
        });
    }

    void RemoveMcpLogBridge()
    {
        mcp::set_log_sink({});
    }

}

TerraForgeMcpServer::TerraForgeMcpServer(
    ApplicationState *applicationState,
    std::string logsDirectory)
    : logsDirectory(std::move(logsDirectory)), port(ReadMcpPort())
{
    endpoint = "http://" + host + ":" + std::to_string(port) + "/mcp";
    RegisterMcpCoreTools(actions, applicationState);
    RegisterMcpCoreResources(resources, applicationState);
}

TerraForgeMcpServer::~TerraForgeMcpServer()
{
    Stop();
}

bool TerraForgeMcpServer::Start()
{
    if (server && server->is_running())
        return true;
    InstallMcpLogBridge();
    mcp::set_log_level(mcp::log_level::debug);

    mcp::server::configuration configuration;
    configuration.host         = host;
    configuration.port         = port;
    configuration.name         = "TerraForge3D MCP";
    configuration.version      = TERR3D_VERSION_STRING;
    configuration.mcp_endpoint = "/mcp";
    configuration.sse_endpoint = "/sse";
    configuration.msg_endpoint = "/message";

    server = std::make_unique<mcp::server>(configuration);
    server->set_server_info(configuration.name, configuration.version);
    server->set_instructions(
        "TerraForge3D exposes registered actions as MCP tools and readable state as MCP resources.");

    RegisterMcpTransport(
        *server,
        actions,
        resources,
        requestQueue,
        [this](const std::string &method,
               const std::string &paramsJson,
               const std::string &sessionId) {
            try {
                RecordCommand(method, nlohmann::json::parse(paramsJson), sessionId);
            } catch (...) {
                RecordCommand(method, nlohmann::json{{"raw", paramsJson}}, sessionId);
            }
        });
    OpenCallHistory();
    if (!server->start(false)) {
        server.reset();
        CloseCallHistory();
        RemoveMcpLogBridge();
        return false;
    }

    TF3D_LOG_INFO("TerraForge3D MCP server listening on http://{}:{}{}",
                  configuration.host, configuration.port, configuration.mcp_endpoint);
    return true;
}

void TerraForgeMcpServer::Update()
{
    requestQueue.Drain();
}

void TerraForgeMcpServer::Stop()
{
    requestQueue.Shutdown();
    if (server) {
        server->stop();
        server.reset();
    }
    CloseCallHistory();
    RemoveMcpLogBridge();
}

bool TerraForgeMcpServer::IsRunning() const
{
    return server != nullptr && server->is_running();
}

McpServerStats TerraForgeMcpServer::GetStats() const
{
    McpServerStats snapshot;
    snapshot.running  = IsRunning();
    snapshot.host     = host;
    snapshot.port     = port;
    snapshot.endpoint = endpoint;
    if (server)
        snapshot.activeSessions = server->get_active_sessions().size();

    std::lock_guard<std::mutex> lock(statsMutex);
    snapshot.commandCount = commandCount;
    snapshot.commandLog   = commandLog;
    return snapshot;
}

void TerraForgeMcpServer::ClearCommandLog()
{
    std::lock_guard<std::mutex> lock(statsMutex);
    commandCount = 0;
    commandLog.clear();
}

void TerraForgeMcpServer::OpenCallHistory()
{
    if (logsDirectory.empty())
        return;

    try {
        const std::filesystem::path logPath(logsDirectory);
        const std::filesystem::path callsDirectory = logPath.parent_path() / "McpCalls";
        std::filesystem::create_directories(callsDirectory);

        const auto now          = std::chrono::system_clock::now();
        const auto nowTime      = std::chrono::system_clock::to_time_t(now);
        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                      now.time_since_epoch())
                                      .count() %
                                  1000;
        std::tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &nowTime);
#else
        localtime_r(&nowTime, &localTime);
#endif
        std::ostringstream filename;
        filename << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S")
                 << "_" << std::setfill('0') << std::setw(3) << milliseconds << ".jsonl";

        callHistoryPath = callsDirectory / filename.str();
        callHistory.open(callHistoryPath, std::ios::out | std::ios::app);
        if (!callHistory.is_open()) {
            TF3D_LOG_ERROR("Could not open MCP call history file '{}'", callHistoryPath.string());
            return;
        }

        const nlohmann::json sessionHeader = {
            {"type", "session"},
            {"startedAt", FormatCommandTimestamp()},
            {"application", "TerraForge3D"},
            {"version", TERR3D_VERSION_STRING},
            {"protocolVersion", mcp::MCP_VERSION}};
        callHistory << sessionHeader.dump() << '\n';
        callHistory.flush();
        TF3D_LOG_INFO("MCP call history file: {}", callHistoryPath.string());
    } catch (const std::exception &exception) {
        TF3D_LOG_ERROR("Could not initialize MCP call history: {}", exception.what());
    }
}

void TerraForgeMcpServer::CloseCallHistory()
{
    if (callHistory.is_open()) {
        callHistory.flush();
        callHistory.close();
    }
}

void TerraForgeMcpServer::RecordCommand(
    const std::string &method,
    const nlohmann::json &params,
    const std::string &sessionId)
{
    std::string parameters;
    std::string requestJson;
    try {
        parameters             = params.dump();
        nlohmann::json request = {
            {"jsonrpc", "2.0"},
            {"method", method}};
        if (!params.empty())
            request["params"] = params;
        requestJson = request.dump(2);
    } catch (...) {
        parameters  = "<unserializable parameters>";
        requestJson = parameters;
    }

    nlohmann::json historyEntry = {
        {"type", "request"},
        {"timestamp", FormatCommandTimestamp()},
        {"sessionId", sessionId},
        {"method", method}};
    try {
        historyEntry["request"] = nlohmann::json::parse(requestJson);
    } catch (...) {
        historyEntry["request"] = requestJson;
    }

    std::lock_guard<std::mutex> lock(statsMutex);
    ++commandCount;
    commandLog.push_back({FormatCommandTimestamp(),
                          method,
                          std::move(parameters),
                          std::move(requestJson)});
    if (callHistory.is_open()) {
        callHistory << historyEntry.dump() << '\n';
        callHistory.flush();
    }
}
