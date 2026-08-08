#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace tf3d::utils
{

    enum class JsonIncludePathMode {
        RelativeToIncludingFile,
        RootRelativeBarePaths,
    };

    struct JsonIncludeResolverOptions {
        std::filesystem::path rootDirectory;
        JsonIncludePathMode pathMode = JsonIncludePathMode::RelativeToIncludingFile;
        bool restrictToRoot          = false;
    };

    class JsonIncludeResolver
    {
    public:
        explicit JsonIncludeResolver(JsonIncludeResolverOptions options = {});

        std::optional<nlohmann::json> ResolveFile(
            const std::filesystem::path &path,
            std::string *error = nullptr) const;

    private:
        std::optional<nlohmann::json> ResolveFileInternal(
            const std::filesystem::path &path,
            std::vector<std::filesystem::path> &includeStack,
            std::string *error) const;

        std::optional<nlohmann::json> ResolveNode(
            const nlohmann::json &node,
            const std::filesystem::path &currentFile,
            std::vector<std::filesystem::path> &includeStack,
            std::string *error) const;

        std::optional<std::filesystem::path> ResolvePath(
            const std::filesystem::path &path,
            const std::filesystem::path *currentFile,
            std::string *error) const;

        JsonIncludeResolverOptions m_Options;
    };

} // namespace tf3d::utils
