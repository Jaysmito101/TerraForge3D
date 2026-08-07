#pragma once

#include "Exporters/Serializer.h"
#include "MCP/ActionRegistry.h"
#include "MCP/SchemaTemplate.h"

#include <concepts>
#include <functional>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace tf3d::mcp_layer::tool_helpers
{

    inline bool ReadState(const nlohmann::json &arguments,
                          const nlohmann::json *&state,
                          McpResult &failure)
    {
        const auto value = arguments.find("State");
        if (value == arguments.end()) {
            failure = McpResult::Failure(
                McpErrorType::InvalidArguments,
                "'State' is required.");
            return false;
        }
        if (!value->is_object()) {
            failure = McpResult::Failure(
                McpErrorType::InvalidArguments,
                "'State' must be an object.");
            return false;
        }
        state = &*value;
        return true;
    }

    inline const nlohmann::json *SchemaPointer(
        const std::optional<nlohmann::json> &schema)
    {
        return schema ? &*schema : nullptr;
    }

    inline bool ValidateState(const nlohmann::json &state,
                              const nlohmann::json *readOnlySchema,
                              McpResult &failure)
    {
        if (!state.is_object()) {
            failure = McpResult::Failure(
                McpErrorType::InvalidArguments,
                "'State' must be an object.");
            return false;
        }

        if (readOnlySchema != nullptr) {
            std::string readOnlyError;
            if (!McpSchemaTemplate::ValidateWritable(
                    state, *readOnlySchema, readOnlyError)) {
                failure = McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    readOnlyError.empty() ? "State contains a read-only field." : readOnlyError);
                return false;
            }
        }
        return true;
    }

    template <typename Save, typename Load>
    McpResult ApplySerializedState(
        const nlohmann::json &state,
        const nlohmann::json *readOnlySchema,
        Save &&save,
        Load &&load,
        std::string_view rejectionMessage = "The requested serialized state was rejected.")
    {
        McpResult failure;
        if (!ValidateState(state, readOnlySchema, failure))
            return failure;

        const exporters::SerializerNode current = std::invoke(save);
        const exporters::SerializerNode updates = exporters::CreateSerializerNodeFromJson(state);
        current->Merge(*updates);

        using LoadResult = std::remove_cvref_t<
            std::invoke_result_t<Load &, const exporters::SerializerNode &>>;
        if constexpr (std::same_as<LoadResult, bool>) {
            if (!std::invoke(load, current))
                return McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    std::string(rejectionMessage));
        } else {
            std::invoke(load, current);
        }

        const exporters::SerializerNode result = std::invoke(save);
        return McpResult::Success(result->ToJson());
    }

    template <typename Save, typename Load>
    McpResult UpdateSerializedState(
        const nlohmann::json &arguments,
        const nlohmann::json *readOnlySchema,
        Save &&save,
        Load &&load,
        std::string_view rejectionMessage = "The requested serialized state was rejected.")
    {
        const nlohmann::json *state = nullptr;
        McpResult failure;
        if (!ReadState(arguments, state, failure))
            return failure;

        return ApplySerializedState(
            *state,
            readOnlySchema,
            std::forward<Save>(save),
            std::forward<Load>(load),
            rejectionMessage);
    }

    template <typename Save>
    McpResult GetSerializedState(Save &&save)
    {
        const exporters::SerializerNode state = std::invoke(std::forward<Save>(save));
        return McpResult::Success(state->ToJson());
    }

    template <typename Handler>
    void RegisterAction(
        ActionRegistry &actions,
        std::string_view schemaPath,
        Handler &&handler,
        const McpSchemaRuntimeProvider &runtime = {})
    {
        RegisterActionFromJson(
            actions,
            McpSchemaTemplate::ComposeDefault(schemaPath, runtime),
            std::forward<Handler>(handler));
    }

} // namespace tf3d::mcp_layer::tool_helpers

#define TF3D_MCP_REGISTER_ACTION(actions, schemaPath, ...) \
    ::tf3d::mcp_layer::tool_helpers::RegisterAction((actions), (schemaPath), (__VA_ARGS__))

#define TF3D_MCP_REGISTER_ACTION_WITH_RUNTIME(actions, schemaPath, runtime, ...) \
    ::tf3d::mcp_layer::tool_helpers::RegisterAction(                             \
        (actions), (schemaPath), (__VA_ARGS__), (runtime))
