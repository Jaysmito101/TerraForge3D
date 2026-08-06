#include "MCP/SchemaTemplate.h"

#include "Base/Logging/Logger.h"
#include "Utils/Utils.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace tf3d::mcp_layer
{

    namespace
    {
        constexpr std::string_view IncludeDirective = "$include";
        constexpr std::string_view RuntimeDirective = "$runtime";

        std::string PathText(const std::filesystem::path &path)
        {
            return path.generic_string();
        }

        bool ReadIncludePaths(const nlohmann::json &value, std::vector<std::string> &paths)
        {
            paths.clear();
            if (value.is_string()) {
                paths.push_back(value.get<std::string>());
                return true;
            }

            if (!value.is_array())
                return false;

            for (const auto &item : value) {
                if (!item.is_string())
                    return false;
                paths.push_back(item.get<std::string>());
            }
            return true;
        }

        void LogSchemaError(const std::filesystem::path &path, std::string_view message)
        {
            TF3D_LOG_ERROR("MCP schema '{}' failed: {}", PathText(path), message);
        }
    } // namespace

    McpSchemaTemplate::McpSchemaTemplate(std::filesystem::path schemaRoot)
    {
        if (schemaRoot.empty())
            schemaRoot = DefaultRoot();

        std::error_code error;
        root = std::filesystem::absolute(schemaRoot, error).lexically_normal();
        if (error) {
            root.clear();
            TF3D_LOG_ERROR("Could not resolve MCP schema root '{}': {}",
                           PathText(schemaRoot), error.message());
        }
    }

    std::filesystem::path McpSchemaTemplate::DefaultRoot()
    {
        return std::filesystem::path(GetExecutableDir()) / "Data" / "Mcp" / "Schemas";
    }

    std::optional<nlohmann::json> McpSchemaTemplate::Compose(
        std::string_view templatePath,
        McpSchemaRuntimeProvider runtime) const
    {
        if (templatePath.empty()) {
            TF3D_LOG_ERROR("MCP schema template path cannot be empty");
            return std::nullopt;
        }

        const auto resolvedPath = ResolvePath(std::filesystem::path(templatePath));
        if (!resolvedPath)
            return std::nullopt;

        std::vector<std::filesystem::path> includeStack;
        return ComposeFile(*resolvedPath, runtime, includeStack);
    }

    void McpSchemaTemplate::Merge(nlohmann::json &target, const nlohmann::json &source)
    {
        if (!target.is_object() || !source.is_object()) {
            target = source;
            return;
        }

        for (const auto &[key, value] : source.items()) {
            const auto targetValue = target.find(key);
            if (targetValue != target.end() && targetValue->is_object() && value.is_object())
                Merge(*targetValue, value);
            else
                target[key] = value;
        }
    }

    std::optional<nlohmann::json> McpSchemaTemplate::ComposeFile(
        const std::filesystem::path &path,
        const McpSchemaRuntimeProvider &runtime,
        std::vector<std::filesystem::path> &includeStack) const
    {
        const auto resolvedPath = ResolvePath(path);
        if (!resolvedPath)
            return std::nullopt;

        if (std::find(includeStack.begin(), includeStack.end(), *resolvedPath) != includeStack.end()) {
            LogSchemaError(*resolvedPath, "circular include");
            return std::nullopt;
        }

        std::ifstream file(*resolvedPath);
        if (!file.is_open()) {
            LogSchemaError(*resolvedPath, "could not open template file");
            return std::nullopt;
        }

        std::stringstream contents;
        contents << file.rdbuf();

        const nlohmann::json source = nlohmann::json::parse(contents.str(), nullptr, false);
        if (source.is_discarded()) {
            LogSchemaError(*resolvedPath, "invalid JSON");
            return std::nullopt;
        }

        includeStack.push_back(*resolvedPath);
        const auto result = ResolveNode(source, *resolvedPath, runtime, includeStack);
        includeStack.pop_back();
        return result;
    }

    std::optional<nlohmann::json> McpSchemaTemplate::ResolveNode(
        const nlohmann::json &node,
        const std::filesystem::path &currentFile,
        const McpSchemaRuntimeProvider &runtime,
        std::vector<std::filesystem::path> &includeStack) const
    {
        if (node.is_array()) {
            nlohmann::json result = nlohmann::json::array();
            for (const auto &item : node) {
                const auto resolved = ResolveNode(item, currentFile, runtime, includeStack);
                if (!resolved)
                    return std::nullopt;
                if (item.is_object() && item.contains(IncludeDirective) && resolved->is_array()) {
                    for (const auto &includedItem : *resolved)
                        result.push_back(includedItem);
                } else {
                    result.push_back(*resolved);
                }
            }
            return result;
        }

        if (!node.is_object())
            return node;

        nlohmann::json result = nlohmann::json::object();
        bool hasIncludedValue = false;
        if (node.contains(IncludeDirective)) {
            std::vector<std::string> includePaths;
            if (!ReadIncludePaths(node.at(IncludeDirective), includePaths)) {
                LogSchemaError(currentFile, "'$include' must be a string or an array of strings");
                return std::nullopt;
            }

            for (const std::string &includePath : includePaths) {
                const std::filesystem::path includeFile =
                    includePath.starts_with("./") || includePath.starts_with("../")
                        ? currentFile.parent_path() / includePath
                        : std::filesystem::path(includePath);
                const auto included = ComposeFile(includeFile, runtime, includeStack);
                if (!included)
                    return std::nullopt;

                if (!hasIncludedValue) {
                    result           = *included;
                    hasIncludedValue = true;
                } else if (result.is_object() && included->is_object()) {
                    Merge(result, *included);
                } else {
                    LogSchemaError(currentFile, "includes must compose JSON objects");
                    return std::nullopt;
                }
            }
        }

        if (node.contains(RuntimeDirective)) {
            if (!node.at(RuntimeDirective).is_string()) {
                LogSchemaError(currentFile, "'$runtime' must contain a string provider name");
                return std::nullopt;
            }
            if (!runtime) {
                LogSchemaError(currentFile, "runtime provider is missing");
                return std::nullopt;
            }

            const std::string name = node.at(RuntimeDirective).get<std::string>();
            const std::optional<nlohmann::json> value = runtime(name);
            if (!value) {
                TF3D_LOG_ERROR("MCP schema '{}' runtime value '{}' is unavailable",
                               PathText(currentFile), name);
                return std::nullopt;
            }
            result = *value;
        }

        nlohmann::json local = nlohmann::json::object();
        for (const auto &[key, value] : node.items()) {
            if (key == IncludeDirective || key == RuntimeDirective)
                continue;

            const auto resolved = ResolveNode(value, currentFile, runtime, includeStack);
            if (!resolved)
                return std::nullopt;
            local[key] = *resolved;
        }

        if (!local.empty()) {
            if (!result.is_object()) {
                LogSchemaError(currentFile, "directives cannot be combined with a non-object value");
                return std::nullopt;
            }
            Merge(result, local);
        }
        return result;
    }

    std::optional<std::filesystem::path> McpSchemaTemplate::ResolvePath(
        const std::filesystem::path &path) const
    {
        if (root.empty()) {
            TF3D_LOG_ERROR("MCP schema root is unavailable");
            return std::nullopt;
        }

        const auto candidate = (path.is_absolute() ? path : root / path).lexically_normal();
        const auto relative  = candidate.lexically_relative(root);
        const std::string relativeText = relative.generic_string();
        if (relative.empty() || relativeText == ".." || relativeText.starts_with("../")) {
            TF3D_LOG_ERROR("MCP schema path '{}' escapes Data/Mcp/Schemas", PathText(path));
            return std::nullopt;
        }
        return candidate;
    }

} // namespace tf3d::mcp_layer
