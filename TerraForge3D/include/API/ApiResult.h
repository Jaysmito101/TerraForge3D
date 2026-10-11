#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace tf3d::api
{

    enum class ApiErrorType : std::uint8_t {
        InvalidArguments = 0,
        InvalidTask,
        Shutdown,
        TaskException,
        Timeout,
        Cancelled,
        ViewportReadFailed,
        JpegEncodeFailed,
        ViewportNotFound,
        GenerationRequestNotFound
    };

    inline constexpr std::string_view ApiErrorTypeToCode(ApiErrorType type)
    {
        switch (type) {
            case ApiErrorType::InvalidArguments:
                return "invalid_arguments";
            case ApiErrorType::InvalidTask:
                return "invalid_task";
            case ApiErrorType::Shutdown:
                return "shutdown";
            case ApiErrorType::TaskException:
                return "task_exception";
            case ApiErrorType::Timeout:
                return "timeout";
            case ApiErrorType::Cancelled:
                return "cancelled";
            case ApiErrorType::ViewportReadFailed:
                return "viewport_read_failed";
            case ApiErrorType::JpegEncodeFailed:
                return "jpeg_encode_failed";
            case ApiErrorType::ViewportNotFound:
                return "viewport_not_found";
            case ApiErrorType::GenerationRequestNotFound:
                return "generation_request_not_found";
        }
        return "unknown";
    }

    inline constexpr int ApiErrorTypeToHttpStatus(ApiErrorType type)
    {
        switch (type) {
            case ApiErrorType::InvalidArguments:
                return 400;
            case ApiErrorType::ViewportNotFound:
            case ApiErrorType::GenerationRequestNotFound:
                return 404;
            case ApiErrorType::Timeout:
                return 504;
            case ApiErrorType::Shutdown:
            case ApiErrorType::Cancelled:
                return 503;
            case ApiErrorType::InvalidTask:
            case ApiErrorType::TaskException:
            case ApiErrorType::ViewportReadFailed:
            case ApiErrorType::JpegEncodeFailed:
                return 500;
        }
        return 500;
    }

    struct ApiJsonBody {
        nlohmann::json value = nlohmann::json::object();
    };

    struct ApiBinaryBody {
        std::string contentType;
        std::vector<std::uint8_t> bytes;
        nlohmann::json metadata = nlohmann::json::object();
    };

    struct ApiFailure {
        ApiErrorType type = ApiErrorType::InvalidArguments;
        std::string message;
        nlohmann::json details = nlohmann::json::object();
    };

    class ApiResult
    {
    public:
        ApiResult() = default;

        [[nodiscard]] bool IsSuccess() const
        {
            return !std::holds_alternative<ApiFailure>(outcome);
        }

        [[nodiscard]] const ApiJsonBody *JsonBody() const
        {
            return std::get_if<ApiJsonBody>(&outcome);
        }

        [[nodiscard]] const ApiBinaryBody *BinaryBody() const
        {
            return std::get_if<ApiBinaryBody>(&outcome);
        }

        [[nodiscard]] const ApiFailure *Failure() const
        {
            return std::get_if<ApiFailure>(&outcome);
        }

        static ApiResult Success(nlohmann::json value = nlohmann::json::object(), std::string operation = {})
        {
            return ApiResult(ApiJsonBody{std::move(value)}, std::move(operation));
        }

        static ApiResult SuccessWithBinary(
            nlohmann::json metadata,
            std::string contentType,
            std::vector<std::uint8_t> bytes,
            std::string operation = {})
        {
            return ApiResult(
                ApiBinaryBody{std::move(contentType), std::move(bytes), std::move(metadata)},
                std::move(operation));
        }

        static ApiResult Failure(
            ApiErrorType type,
            std::string message,
            nlohmann::json details = nlohmann::json::object())
        {
            return ApiResult(ApiFailure{type, std::move(message), std::move(details)});
        }

        std::string operationId;

    private:
        using Outcome = std::variant<ApiJsonBody, ApiBinaryBody, ApiFailure>;

        explicit ApiResult(ApiJsonBody body, std::string operation = {})
            : operationId(std::move(operation)), outcome(std::move(body))
        {
        }

        explicit ApiResult(ApiBinaryBody body, std::string operation = {})
            : operationId(std::move(operation)), outcome(std::move(body))
        {
        }

        explicit ApiResult(ApiFailure failure)
            : outcome(std::move(failure))
        {
        }

        Outcome outcome = ApiJsonBody{};
    };

} // namespace tf3d::api
