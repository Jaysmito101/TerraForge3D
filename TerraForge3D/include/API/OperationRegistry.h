#pragma once

#include "API/ApiResult.h"

#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tf3d::api
{

    using ApiOperationHandler = std::function<ApiResult(const nlohmann::json &)>;

    enum class ApiHttpMethod : std::uint8_t {
        Get,
        Post,
        Patch
    };

    inline constexpr std::string_view ApiHttpMethodName(ApiHttpMethod method)
    {
        switch (method) {
            case ApiHttpMethod::Get:
                return "GET";
            case ApiHttpMethod::Post:
                return "POST";
            case ApiHttpMethod::Patch:
                return "PATCH";
        }
        return "UNKNOWN";
    }

    enum class OperationFlags : std::uint32_t {
        None        = 0,
        ReadOnly    = 1u << 0,
        Destructive = 1u << 1,
        LongRunning = 1u << 2
    };

    struct OperationEntry {
        std::string name;
        std::string title;
        std::string description;
        ApiHttpMethod method = ApiHttpMethod::Post;
        std::string path;
        std::string resourcePath;
        nlohmann::json inputSchema = nlohmann::json{
            {"type", "object"},
            {"properties", nlohmann::json::object()}};
        nlohmann::json annotations = nlohmann::json::object();
        OperationFlags flags       = OperationFlags::None;
        ApiOperationHandler invoke;
    };

    std::optional<OperationEntry> CreateOperationEntryFromJson(
        const nlohmann::json &definition,
        ApiOperationHandler invoke);

    void RegisterOperationFromJson(
        class OperationRegistry &operations,
        const std::optional<nlohmann::json> &definition,
        ApiOperationHandler invoke);

    class OperationRegistry
    {
    public:
        bool Register(OperationEntry entry);
        bool Unregister(const std::string &name);
        std::optional<OperationEntry> Find(const std::string &name) const;
        std::vector<OperationEntry> Snapshot() const;
        void Clear();

    private:
        mutable std::mutex mutex;
        std::map<std::string, OperationEntry> entries;
    };

} // namespace tf3d::api
