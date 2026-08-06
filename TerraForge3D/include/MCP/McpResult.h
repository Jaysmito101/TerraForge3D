#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace tf3d::mcp_layer
{

    inline constexpr const char *MCP_RESULT_REVISION = "tf3d.mcp/v1";

    enum class McpErrorType : uint8_t {
        InvalidArguments = 0,
        InvalidTask,
        Shutdown,
        TaskException,
        Timeout,
        Cancelled,
        ViewportReadFailed,
        JpegEncodeFailed,
        ViewportNotFound
    };

    inline constexpr std::string_view McpErrorTypeToCode(McpErrorType type)
    {
        if (type == McpErrorType::InvalidArguments)
            return "invalid_arguments";
        else if (type == McpErrorType::InvalidTask)
            return "invalid_task";
        else if (type == McpErrorType::Shutdown)
            return "shutdown";
        else if (type == McpErrorType::TaskException)
            return "task_exception";
        else if (type == McpErrorType::Timeout)
            return "timeout";
        else if (type == McpErrorType::Cancelled)
            return "cancelled";
        else if (type == McpErrorType::ViewportReadFailed)
            return "viewport_read_failed";
        else if (type == McpErrorType::JpegEncodeFailed)
            return "jpeg_encode_failed";
        else if (type == McpErrorType::ViewportNotFound)
            return "viewport_not_found";

        return "unknown";
    }

    struct McpResult {
        bool ok                    = true;
        nlohmann::json value       = nlohmann::json::object();
        nlohmann::json diagnostics = nlohmann::json::array();
        nlohmann::json toolContent = nlohmann::json::array();
        std::string operationId;

        static McpResult Success(nlohmann::json result = nlohmann::json::object(), std::string operation = {})
        {
            McpResult response;
            response.value       = std::move(result);
            response.operationId = std::move(operation);
            return response;
        }

        static McpResult SuccessWithToolContent(
            nlohmann::json result,
            nlohmann::json content,
            std::string operation = {})
        {
            McpResult response;
            response.value       = std::move(result);
            response.toolContent = std::move(content);
            response.operationId = std::move(operation);
            return response;
        }

        static McpResult Failure(
            McpErrorType type,
            std::string message,
            nlohmann::json details = nlohmann::json::object())
        {
            McpResult response;
            response.ok = false;
            response.diagnostics.push_back({{"code", std::string(McpErrorTypeToCode(type))},
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
            if (!toolContent.empty())
                return toolContent;

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

} // namespace tf3d::mcp_layer
