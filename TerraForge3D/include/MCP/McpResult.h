#pragma once

#include "MCP/McpJson.h"

#include <string>
#include <utility>

inline constexpr const char* MCP_RESULT_REVISION = "tf3d.mcp/v1";

	struct McpResult
	{
		bool ok = true;
		Json value = Json::object();
		Json diagnostics = Json::array();
		std::string operationId;

		static McpResult Success(Json result = Json::object(), std::string operation = {})
		{
			McpResult response;
			response.value = std::move(result);
			response.operationId = std::move(operation);
			return response;
		}

		static McpResult Failure(
			std::string code,
			std::string message,
			Json details = Json::object())
		{
			McpResult response;
			response.ok = false;
			response.diagnostics.push_back({
				{"code", std::move(code)},
				{"message", std::move(message)},
				{"details", std::move(details)}
			});
			return response;
		}

		Json ToEnvelope() const
		{
			Json envelope = {
				{"ok", ok},
				{"revision", MCP_RESULT_REVISION},
				{"value", value},
				{"diagnostics", diagnostics}
			};
			if (!operationId.empty()) envelope["operationId"] = operationId;
			return envelope;
		}

		Json ToToolContent() const
		{
			Json content = Json::array();
			content.push_back({
				{"type", "text"},
				{"text", ToEnvelope().dump()}
			});
			return content;
		}

		std::string ToErrorMessage() const
		{
			return ToEnvelope().dump();
		}
	};
