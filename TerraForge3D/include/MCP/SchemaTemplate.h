#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <functional>
#include <optional>
#include <string_view>
#include <vector>

namespace tf3d::mcp_layer
{

    using McpSchemaRuntimeProvider =
        std::function<std::optional<nlohmann::json>(std::string_view name)>;

    class McpSchemaTemplate
    {
    public:
        explicit McpSchemaTemplate(std::filesystem::path root = {});

        std::optional<nlohmann::json> Compose(
            std::string_view templatePath,
            McpSchemaRuntimeProvider runtime = {}) const;

        static std::filesystem::path DefaultRoot();
        static void Merge(nlohmann::json &target, const nlohmann::json &source);

    private:
        std::optional<nlohmann::json> ComposeFile(
            const std::filesystem::path &path,
            const McpSchemaRuntimeProvider &runtime,
            std::vector<std::filesystem::path> &includeStack) const;

        std::optional<nlohmann::json> ResolveNode(
            const nlohmann::json &node,
            const std::filesystem::path &currentFile,
            const McpSchemaRuntimeProvider &runtime,
            std::vector<std::filesystem::path> &includeStack) const;

        std::optional<std::filesystem::path> ResolvePath(const std::filesystem::path &path) const;

        std::filesystem::path root;
    };

} // namespace tf3d::mcp_layer
