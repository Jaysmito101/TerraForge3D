#pragma once

#include "API/OperationRegistry.h"
#include "API/SchemaTemplate.h"
#include "Exporters/Serializer.h"

#include <concepts>
#include <functional>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace tf3d::api::operation_helpers
{

    inline bool ReadState(const nlohmann::json &arguments,
                          const nlohmann::json *&state,
                          ApiResult &failure)
    {
        const auto value = arguments.find("State");
        if (value == arguments.end()) {
            failure = ApiResult::Failure(
                ApiErrorType::InvalidArguments,
                "'State' is required.");
            return false;
        }
        if (!value->is_object()) {
            failure = ApiResult::Failure(
                ApiErrorType::InvalidArguments,
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
                              ApiResult &failure)
    {
        if (!state.is_object()) {
            failure = ApiResult::Failure(
                ApiErrorType::InvalidArguments,
                "'State' must be an object.");
            return false;
        }

        if (readOnlySchema != nullptr) {
            std::string readOnlyError;
            if (!ApiSchemaTemplate::ValidateWritable(
                    state, *readOnlySchema, readOnlyError)) {
                failure = ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    readOnlyError.empty() ? "State contains a read-only field." : readOnlyError);
                return false;
            }
        }
        return true;
    }

    template <typename Save, typename Load>
    ApiResult ApplySerializedState(
        const nlohmann::json &state,
        const nlohmann::json *readOnlySchema,
        Save &&save,
        Load &&load,
        std::string_view rejectionMessage = "The requested serialized state was rejected.")
    {
        ApiResult failure;
        if (!ValidateState(state, readOnlySchema, failure)) {
            return failure;
        }

        const exporters::SerializerNode current = std::invoke(save);
        const exporters::SerializerNode updates = exporters::CreateSerializerNodeFromJson(state);
        current->Merge(*updates);

        using LoadResult = std::remove_cvref_t<
            std::invoke_result_t<Load &, const exporters::SerializerNode &>>;
        if constexpr (std::same_as<LoadResult, bool>) {
            if (!std::invoke(load, current)) {
                return ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    std::string(rejectionMessage));
            }
        } else {
            std::invoke(load, current);
        }

        const exporters::SerializerNode result = std::invoke(save);
        return ApiResult::Success(result->ToJson());
    }

    template <typename Save, typename Load>
    ApiResult UpdateSerializedState(
        const nlohmann::json &arguments,
        const nlohmann::json *readOnlySchema,
        Save &&save,
        Load &&load,
        std::string_view rejectionMessage = "The requested serialized state was rejected.")
    {
        const nlohmann::json *state = nullptr;
        ApiResult failure;
        if (!ReadState(arguments, state, failure)) {
            return failure;
        }

        return ApplySerializedState(
            *state,
            readOnlySchema,
            std::forward<Save>(save),
            std::forward<Load>(load),
            rejectionMessage);
    }

    template <typename Save>
    ApiResult GetSerializedState(Save &&save)
    {
        const exporters::SerializerNode state = std::invoke(std::forward<Save>(save));
        return ApiResult::Success(state->ToJson());
    }

    template <typename Handler>
    void RegisterOperation(
        OperationRegistry &operations,
        std::string_view schemaPath,
        Handler &&handler,
        const ApiSchemaRuntimeProvider &runtime = {})
    {
        RegisterOperationFromJson(
            operations,
            ApiSchemaTemplate::ComposeDefault(schemaPath, runtime),
            std::forward<Handler>(handler));
    }

} // namespace tf3d::api::operation_helpers

#define TF3D_API_REGISTER_OPERATION(operations, schemaPath, ...) \
    ::tf3d::api::operation_helpers::RegisterOperation((operations), (schemaPath), (__VA_ARGS__))

#define TF3D_API_REGISTER_OPERATION_WITH_RUNTIME(operations, schemaPath, runtime, ...) \
    ::tf3d::api::operation_helpers::RegisterOperation(                                 \
        (operations), (schemaPath), (__VA_ARGS__), (runtime))
