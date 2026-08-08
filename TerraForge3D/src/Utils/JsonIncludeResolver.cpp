#include "Utils/JsonIncludeResolver.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string_view>
#include <utility>

namespace tf3d::utils
{

    namespace
    {
        constexpr std::string_view INCLUDE_DIRECTIVE = "$include";

        std::string PathText(const std::filesystem::path &path)
        {
            return path.generic_string();
        }

        std::filesystem::path NormalizePath(const std::filesystem::path &path)
        {
            std::error_code error;
            const auto absolutePath = path.is_absolute() ? path : std::filesystem::absolute(path, error);
            if (error)
                return path.lexically_normal();

            error.clear();
            const auto canonicalPath = std::filesystem::weakly_canonical(absolutePath, error);
            return error ? absolutePath.lexically_normal() : canonicalPath;
        }

        bool IsExplicitlyRelative(std::string_view path)
        {
            return path.starts_with("./") || path.starts_with("../") ||
                   path.starts_with(".\\") || path.starts_with("..\\");
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

        void SetError(std::string *error,
                      const std::filesystem::path &path,
                      std::string_view message)
        {
            if (error == nullptr)
                return;
            *error = "JSON file '" + PathText(path) + "': " + std::string(message);
        }

        void MergeObjects(nlohmann::json &target, const nlohmann::json &source)
        {
            if (!target.is_object() || !source.is_object()) {
                target = source;
                return;
            }

            for (const auto &[key, value] : source.items()) {
                const auto targetValue = target.find(key);
                if (targetValue != target.end() && targetValue->is_object() && value.is_object())
                    MergeObjects(*targetValue, value);
                else
                    target[key] = value;
            }
        }
    } // namespace

    JsonIncludeResolver::JsonIncludeResolver(JsonIncludeResolverOptions options)
        : m_Options(std::move(options))
    {
        if (!m_Options.rootDirectory.empty()) {
            m_Options.rootDirectory = NormalizePath(m_Options.rootDirectory);
        }
    }

    std::optional<nlohmann::json> JsonIncludeResolver::ResolveFile(
        const std::filesystem::path &path,
        std::string *error) const
    {
        if (error != nullptr) {
            error->clear();
        }

        std::vector<std::filesystem::path> includeStack{};
        return ResolveFileInternal(path, includeStack, error);
    }

    std::optional<nlohmann::json> JsonIncludeResolver::ResolveFileInternal(
        const std::filesystem::path &path,
        std::vector<std::filesystem::path> &includeStack,
        std::string *error) const
    {
        const auto resolvedPath = ResolvePath(path, nullptr, error);
        if (!resolvedPath) {
            return std::nullopt;
        }

        if (std::find(includeStack.begin(), includeStack.end(), *resolvedPath) != includeStack.end()) {
            std::string chain;
            for (const auto &includedPath : includeStack) {
                if (!chain.empty()) {
                    chain += " -> ";
                }
                chain += PathText(includedPath);
            }
            if (!chain.empty()) {
                chain += " -> ";
            }
            chain += PathText(*resolvedPath);
            SetError(error, *resolvedPath, "circular include (" + chain + ")");
            return std::nullopt;
        }

        std::ifstream file(*resolvedPath);
        if (!file.is_open()) {
            SetError(error, *resolvedPath, "could not open file");
            return std::nullopt;
        }

        std::stringstream contents;
        contents << file.rdbuf();
        const nlohmann::json source = nlohmann::json::parse(contents.str(), nullptr, false);
        if (source.is_discarded()) {
            SetError(error, *resolvedPath, "invalid JSON");
            return std::nullopt;
        }

        includeStack.push_back(*resolvedPath);
        const auto result = ResolveNode(source, *resolvedPath, includeStack, error);
        includeStack.pop_back();
        return result;
    }

    std::optional<nlohmann::json> JsonIncludeResolver::ResolveNode(
        const nlohmann::json &node,
        const std::filesystem::path &currentFile,
        std::vector<std::filesystem::path> &includeStack,
        std::string *error) const
    {
        if (node.is_array()) {
            nlohmann::json result = nlohmann::json::array();
            for (const auto &item : node) {
                const auto resolved = ResolveNode(item, currentFile, includeStack, error);
                if (!resolved) {
                    return std::nullopt;
                }

                if (item.is_object() && item.contains(INCLUDE_DIRECTIVE) && resolved->is_array()) {
                    for (const auto &includedItem : *resolved) {
                        result.push_back(includedItem);
                    }
                } else {
                    result.push_back(*resolved);
                }
            }
            return result;
        }

        if (!node.is_object()) {
            return node;
        }

        nlohmann::json result = nlohmann::json::object();
        bool hasIncludedValue = false;
        if (node.contains(INCLUDE_DIRECTIVE)) {
            std::vector<std::string> includePaths;
            if (!ReadIncludePaths(node.at(INCLUDE_DIRECTIVE), includePaths)) {
                SetError(error, currentFile, "'$include' must be a string or an array of strings");
                return std::nullopt;
            }

            for (const std::string &includePath : includePaths) {
                const auto includeFile = ResolvePath(
                    std::filesystem::path(includePath), &currentFile, error);
                if (!includeFile)
                    return std::nullopt;

                const auto included = ResolveFileInternal(*includeFile, includeStack, error);
                if (!included)
                    return std::nullopt;

                if (!hasIncludedValue) {
                    result           = *included;
                    hasIncludedValue = true;
                } else if (result.is_object() && included->is_object()) {
                    MergeObjects(result, *included);
                } else {
                    SetError(error, currentFile, "multiple includes must compose JSON objects");
                    return std::nullopt;
                }
            }
        }

        nlohmann::json local = nlohmann::json::object();
        for (const auto &[key, value] : node.items()) {
            if (key == INCLUDE_DIRECTIVE)
                continue;

            const auto resolved = ResolveNode(value, currentFile, includeStack, error);
            if (!resolved)
                return std::nullopt;
            local[key] = *resolved;
        }

        if (!local.empty()) {
            if (!result.is_object()) {
                SetError(error, currentFile, "'$include' cannot be combined with a non-object value");
                return std::nullopt;
            }
            MergeObjects(result, local);
        }
        return result;
    }

    std::optional<std::filesystem::path> JsonIncludeResolver::ResolvePath(
        const std::filesystem::path &path,
        const std::filesystem::path *currentFile,
        std::string *error) const
    {
        std::filesystem::path candidate = path;
        if (!candidate.is_absolute()) {
            const std::string pathText = path.generic_string();
            const bool useRoot         = m_Options.pathMode == JsonIncludePathMode::RootRelativeBarePaths &&
                                 !IsExplicitlyRelative(pathText);
            if (useRoot) {
                if (m_Options.rootDirectory.empty()) {
                    SetError(error, path, "root-relative include has no root directory");
                    return std::nullopt;
                }
                candidate = m_Options.rootDirectory / candidate;
            } else if (currentFile != nullptr) {
                candidate = currentFile->parent_path() / candidate;
            } else if (!m_Options.rootDirectory.empty()) {
                candidate = m_Options.rootDirectory / candidate;
            }
        }

        candidate = NormalizePath(candidate);
        if (m_Options.restrictToRoot) {
            if (m_Options.rootDirectory.empty()) {
                SetError(error, candidate, "path restriction has no root directory");
                return std::nullopt;
            }

            const auto relative            = candidate.lexically_relative(m_Options.rootDirectory);
            const std::string relativeText = relative.generic_string();
            if (relative.empty() || relativeText == ".." || relativeText.starts_with("../")) {
                SetError(error, candidate, "path escapes the configured include root");
                return std::nullopt;
            }
        }

        return candidate;
    }

} // namespace tf3d::utils
