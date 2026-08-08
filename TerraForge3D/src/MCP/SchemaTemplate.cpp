#include "MCP/SchemaTemplate.h"

#include "Base/Logging/Logger.h"
#include "Utils/JsonIncludeResolver.h"
#include "Utils/Utils.h"

#include <string>
#include <utility>

namespace tf3d::mcp_layer
{

    namespace
    {
        constexpr std::string_view RuntimeDirective = "$runtime";

        std::string PathText(const std::filesystem::path &path)
        {
            return path.generic_string();
        }

        void LogSchemaError(const std::filesystem::path &path, std::string_view message)
        {
            TF3D_LOG_ERROR("MCP schema '{}' failed: {}", PathText(path), message);
        }

        std::string ChildPath(std::string_view path, std::string_view child)
        {
            if (path.empty())
                return std::string(child);
            return std::string(path) + "." + std::string(child);
        }

        bool ValidateWritableNode(const nlohmann::json &value,
                                  const nlohmann::json &schema,
                                  std::string_view path,
                                  std::string &error)
        {
            if (!schema.is_object()) {
                TF3D_LOG_ERROR("MCP writable validation schema must contain JSON objects");
                error = "The writable validation schema is invalid.";
                return false;
            }

            const auto readOnly = schema.find("readOnly");
            if (readOnly != schema.end()) {
                if (!readOnly->is_boolean()) {
                    TF3D_LOG_ERROR("MCP writable validation schema field 'readOnly' must be a boolean");
                    error = "The writable validation schema is invalid.";
                    return false;
                }

                if (readOnly->get<bool>()) {
                    const std::string field = path.empty() ? "value" : std::string(path);
                    TF3D_LOG_WARN("MCP update rejected read-only field '{}'", field);
                    error = "Field '" + field + "' is read-only.";
                    return false;
                }
            }

            const auto properties = schema.find("properties");
            if (properties != schema.end()) {
                if (!properties->is_object()) {
                    TF3D_LOG_ERROR("MCP writable validation schema field 'properties' must be an object");
                    error = "The writable validation schema is invalid.";
                    return false;
                }

                if (value.is_object()) {
                    for (const auto &[key, childValue] : value.items()) {
                        const auto childSchema = properties->find(key);
                        if (childSchema == properties->end())
                            continue;

                        if (!ValidateWritableNode(
                                childValue, *childSchema, ChildPath(path, key), error))
                            return false;
                    }
                }
            }

            const auto items = schema.find("items");
            if (items != schema.end()) {
                if (!items->is_object()) {
                    TF3D_LOG_ERROR("MCP writable validation schema field 'items' must be an object");
                    error = "The writable validation schema is invalid.";
                    return false;
                }

                if (value.is_array()) {
                    for (const auto &item : value) {
                        if (!ValidateWritableNode(item, *items, path, error))
                            return false;
                    }
                }
            }

            return true;
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

        utils::JsonIncludeResolverOptions includeOptions;
        includeOptions.rootDirectory  = root;
        includeOptions.pathMode       = utils::JsonIncludePathMode::RootRelativeBarePaths;
        includeOptions.restrictToRoot = true;
        const utils::JsonIncludeResolver includeResolver(includeOptions);

        std::string includeError;
        const auto included = includeResolver.ResolveFile(*resolvedPath, &includeError);
        if (!included) {
            LogSchemaError(*resolvedPath, includeError);
            return std::nullopt;
        }

        return ResolveNode(*included, *resolvedPath, runtime);
    }

    std::optional<nlohmann::json> McpSchemaTemplate::ComposeDefault(
        std::string_view templatePath,
        McpSchemaRuntimeProvider runtime)
    {
        return McpSchemaTemplate().Compose(templatePath, std::move(runtime));
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

    bool McpSchemaTemplate::ValidateWritable(
        const nlohmann::json &value,
        const nlohmann::json &schema,
        std::string &error)
    {
        error.clear();
        return ValidateWritableNode(value, schema, {}, error);
    }

    std::optional<nlohmann::json> McpSchemaTemplate::ResolveNode(
        const nlohmann::json &node,
        const std::filesystem::path &currentFile,
        const McpSchemaRuntimeProvider &runtime) const
    {
        if (node.is_array()) {
            nlohmann::json result = nlohmann::json::array();
            for (const auto &item : node) {
                const auto resolved = ResolveNode(item, currentFile, runtime);
                if (!resolved)
                    return std::nullopt;
                result.push_back(*resolved);
            }
            return result;
        }

        if (!node.is_object())
            return node;

        nlohmann::json result = nlohmann::json::object();
        if (node.contains(RuntimeDirective)) {
            if (!node.at(RuntimeDirective).is_string()) {
                LogSchemaError(currentFile, "'$runtime' must contain a string provider name");
                return std::nullopt;
            }
            if (!runtime) {
                LogSchemaError(currentFile, "runtime provider is missing");
                return std::nullopt;
            }

            const std::string name                    = node.at(RuntimeDirective).get<std::string>();
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
            if (key == RuntimeDirective)
                continue;

            const auto resolved = ResolveNode(value, currentFile, runtime);
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

        const auto candidate           = (path.is_absolute() ? path : root / path).lexically_normal();
        const auto relative            = candidate.lexically_relative(root);
        const std::string relativeText = relative.generic_string();
        if (relative.empty() || relativeText == ".." || relativeText.starts_with("../")) {
            TF3D_LOG_ERROR("MCP schema path '{}' escapes Data/Mcp/Schemas", PathText(path));
            return std::nullopt;
        }
        return candidate;
    }

} // namespace tf3d::mcp_layer
