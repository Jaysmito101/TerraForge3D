#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace tf3d::api
{

    using ApiSchemaRuntimeProvider =
        std::function<std::optional<nlohmann::json>(std::string_view name)>;

    class ApiSchemaTemplate
    {
    public:
        explicit ApiSchemaTemplate(std::filesystem::path root = {});

        std::optional<nlohmann::json> Compose(
            std::string_view templatePath,
            ApiSchemaRuntimeProvider runtime = {}) const;

        static std::optional<nlohmann::json> ComposeDefault(
            std::string_view templatePath,
            ApiSchemaRuntimeProvider runtime = {});

        static std::filesystem::path DefaultRoot();
        static void Merge(nlohmann::json &target, const nlohmann::json &source);

        static bool ValidateWritable(
            const nlohmann::json &value,
            const nlohmann::json &schema,
            std::string &error);

        static bool Validate(
            const nlohmann::json &value,
            const nlohmann::json &schema,
            nlohmann::json &errors);

    private:
        std::optional<nlohmann::json> ResolveNode(
            const nlohmann::json &node,
            const std::filesystem::path &currentFile,
            const ApiSchemaRuntimeProvider &runtime) const;

        std::optional<std::filesystem::path> ResolvePath(const std::filesystem::path &path) const;

        std::filesystem::path root;
    };

} // namespace tf3d::api
