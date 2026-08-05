#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <utility>

inline constexpr const char *MCP_RESULT_REVISION = "tf3d.mcp/v1";

struct McpResult {
    bool ok                    = true;
    nlohmann::json value       = nlohmann::json::object();
    nlohmann::json diagnostics = nlohmann::json::array();
    std::string operationId;

    static McpResult Success(nlohmann::json result = nlohmann::json::object(), std::string operation = {})
    {
        McpResult response;
        response.value       = std::move(result);
        response.operationId = std::move(operation);
        return response;
    }

    static McpResult Failure(
        std::string code,
        std::string message,
        nlohmann::json details = nlohmann::json::object())
    {
        McpResult response;
        response.ok = false;
        response.diagnostics.push_back({{"code", std::move(code)},
                                        {"message", std::move(message)},
                                        {"details", std::move(details)}});
        return response;
    }

    nlohmann::json ToEnvelope() const
    {
        nlohmann::json envelope = {
            {"ok", ok},
            {"revision", MCP_RESULT_REVISION},
            {"value", value},
            {"diagnostics", diagnostics}};
        if (!operationId.empty())
            envelope["operationId"] = operationId;
        return envelope;
    }

    nlohmann::json ToToolContent() const
    {
        nlohmann::json content = nlohmann::json::array();
        content.push_back({{"type", "text"},
                           {"text", ToEnvelope().dump()}});
        return content;
    }

    std::string ToErrorMessage() const
    {
        return ToEnvelope().dump();
    }
};
