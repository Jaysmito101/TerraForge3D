#pragma once

#include "API/MainThreadRequestQueue.h"
#include "API/OperationRegistry.h"

#include <nlohmann/json.hpp>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace httplib
{
    struct Request;
    struct Response;
    class Server;
} // namespace httplib

namespace tf3d::data
{
    class ApplicationState;
}

namespace tf3d::api
{

    struct ApiRequestLogEntry {
        std::string timestamp;
        std::string method;
        std::string path;
        int status = 0;
        std::string bodyPreview;
    };

    struct ApiServerStats {
        bool running     = false;
        std::string host = "127.0.0.1";
        int port         = 9823;
        std::string endpoint;
        std::uint64_t requestCount   = 0;
        std::uint64_t activeRequests = 0;
        std::vector<ApiRequestLogEntry> requestLog;
    };

    class ApiServer
    {
    public:
        ApiServer(tf3d::data::ApplicationState *applicationState, std::string logsDirectory);
        ~ApiServer();

        ApiServer(const ApiServer &)            = delete;
        ApiServer &operator=(const ApiServer &) = delete;

        bool Start();
        void Update();
        void Stop();

        bool IsRunning() const;
        ApiServerStats GetStats() const;
        void ClearRequestLog();

    private:
        void RegisterOperations();
        void RegisterRoutes();
        void OpenCallHistory();
        void CloseCallHistory();
        void RecordRequest(
            const httplib::Request &request,
            const httplib::Response &response);
        ApiResult InvokeOperation(const std::string &name, const httplib::Request &request);
        void WriteResult(const ApiResult &result, httplib::Response &response);
        nlohmann::json BuildOpenApiDocument() const;
        nlohmann::json BuildOperationsDocument(std::string_view groupPath) const;
        nlohmann::json BuildSkillsDocument(std::string_view groupPath) const;
        nlohmann::json BuildOperationDocument(const OperationEntry &entry) const;
        nlohmann::json BuildOperationSchema(const OperationEntry &entry, std::string_view idReference) const;
        nlohmann::json BuildOperationResponseSchema(const OperationEntry &entry,
                                                    std::string_view idReference) const;
        std::string BuildOperationSkill(const OperationEntry &entry) const;

        tf3d::data::ApplicationState *applicationState = nullptr;
        std::string logsDirectory;
        const std::string host = "127.0.0.1";
        int port               = 9823;
        std::string endpoint;
        OperationRegistry operations;
        bool operationRegistryComplete = false;
        nlohmann::json openApiDocument = nlohmann::json::object();
        MainThreadRequestQueue requestQueue;
        std::unique_ptr<httplib::Server> server;
        std::thread serverThread;
        std::atomic<bool> startFailed             = false;
        std::atomic<std::uint64_t> requestCount   = 0;
        std::atomic<std::uint64_t> activeRequests = 0;
        std::ofstream callHistory;
        std::filesystem::path callHistoryPath;
        mutable std::mutex statsMutex;
        std::vector<ApiRequestLogEntry> requestLog;
    };

} // namespace tf3d::api
