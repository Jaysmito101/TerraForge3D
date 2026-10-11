#include "API/SchemaTemplate.h"

#include "Base/Logging/Logger.h"
#include "Utils/JsonIncludeResolver.h"
#include "Utils/Utils.h"

#include <algorithm>
#include <regex>
#include <string>
#include <utility>

namespace tf3d::api
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
            TF3D_LOG_ERROR("API schema '{}' failed: {}", PathText(path), message);
        }

        std::string ChildPath(std::string_view path, std::string_view child)
        {
            if (path.empty()) {
                return std::string(child);
            }
            return std::string(path) + "." + std::string(child);
        }

        bool ValidateWritableNode(const nlohmann::json &value,
                                  const nlohmann::json &schema,
                                  std::string_view path,
                                  std::string &error)
        {
            if (!schema.is_object()) {
                TF3D_LOG_ERROR("API writable validation schema must contain JSON objects");
                error = "The writable validation schema is invalid.";
                return false;
            }

            const auto readOnly = schema.find("readOnly");
            if (readOnly != schema.end()) {
                if (!readOnly->is_boolean()) {
                    TF3D_LOG_ERROR("API writable validation schema field 'readOnly' must be a boolean");
                    error = "The writable validation schema is invalid.";
                    return false;
                }

                if (readOnly->get<bool>()) {
                    const std::string field = path.empty() ? "value" : std::string(path);
                    TF3D_LOG_WARN("API update rejected read-only field '{}'", field);
                    error = "Field '" + field + "' is read-only.";
                    return false;
                }
            }

            const auto properties = schema.find("properties");
            if (properties != schema.end()) {
                if (!properties->is_object()) {
                    TF3D_LOG_ERROR("API writable validation schema field 'properties' must be an object");
                    error = "The writable validation schema is invalid.";
                    return false;
                }

                if (value.is_object()) {
                    for (const auto &[key, childValue] : value.items()) {
                        const auto childSchema = properties->find(key);
                        if (childSchema == properties->end()) {
                            continue;
                        }

                        if (!ValidateWritableNode(
                                childValue, *childSchema, ChildPath(path, key), error)) {
                            return false;
                        }
                    }
                }
            }

            const auto items = schema.find("items");
            if (items != schema.end()) {
                if (!items->is_object()) {
                    TF3D_LOG_ERROR("API writable validation schema field 'items' must be an object");
                    error = "The writable validation schema is invalid.";
                    return false;
                }

                if (value.is_array()) {
                    for (const auto &item : value) {
                        if (!ValidateWritableNode(item, *items, path, error)) {
                            return false;
                        }
                    }
                }
            }

            return true;
        }

        bool MatchesType(const nlohmann::json &value, std::string_view type)
        {
            if (type == "object") {
                return value.is_object();
            }
            if (type == "array") {
                return value.is_array();
            }
            if (type == "string") {
                return value.is_string();
            }
            if (type == "boolean") {
                return value.is_boolean();
            }
            if (type == "integer") {
                return value.is_number_integer() || value.is_number_unsigned();
            }
            if (type == "number") {
                return value.is_number();
            }
            if (type == "null") {
                return value.is_null();
            }
            return false;
        }

        void AddValidationError(
            nlohmann::json &errors,
            std::string_view path,
            std::string message)
        {
            errors.push_back({{"path", path.empty() ? "$" : std::string(path)},
                              {"message", std::move(message)}});
        }

        bool ValidateSchemaNode(
            const nlohmann::json &value,
            const nlohmann::json &schema,
            std::string_view path,
            nlohmann::json &errors)
        {
            const std::size_t initialErrorCount = errors.size();
            if (!schema.is_object()) {
                AddValidationError(errors, path, "The operation schema is invalid.");
                return false;
            }

            if (const auto type = schema.find("type"); type != schema.end()) {
                bool matches = false;
                if (type->is_string()) {
                    matches = MatchesType(value, type->get<std::string>());
                } else if (type->is_array()) {
                    for (const auto &candidate : *type) {
                        if (candidate.is_string() && MatchesType(value, candidate.get<std::string>())) {
                            matches = true;
                            break;
                        }
                    }
                }
                if (!matches) {
                    AddValidationError(errors, path, "The value has the wrong JSON type.");
                    return false;
                }
            }

            if (const auto enumValues = schema.find("enum"); enumValues != schema.end()) {
                if (!enumValues->is_array() ||
                    std::find(enumValues->begin(), enumValues->end(), value) == enumValues->end()) {
                    AddValidationError(errors, path, "The value is not one of the allowed values.");
                }
            }
            if (const auto constant = schema.find("const"); constant != schema.end() && value != *constant) {
                AddValidationError(errors, path, "The value does not match the required constant.");
            }

            if (value.is_object()) {
                if (const auto required = schema.find("required"); required != schema.end()) {
                    if (!required->is_array()) {
                        AddValidationError(errors, path, "The operation schema has an invalid required list.");
                    } else {
                        for (const auto &name : *required) {
                            if (!name.is_string()) {
                                AddValidationError(errors, path, "The operation schema has an invalid required field.");
                                continue;
                            }
                            const std::string field = name.get<std::string>();
                            if (!value.contains(field)) {
                                AddValidationError(errors, ChildPath(path, field), "This field is required.");
                            }
                        }
                    }
                }

                const auto properties = schema.find("properties");
                const auto additional = schema.find("additionalProperties");
                for (const auto &[field, fieldValue] : value.items()) {
                    bool foundProperty = false;
                    if (properties != schema.end() && properties->is_object()) {
                        const auto fieldSchema = properties->find(field);
                        if (fieldSchema != properties->end()) {
                            ValidateSchemaNode(fieldValue, *fieldSchema, ChildPath(path, field), errors);
                            foundProperty = true;
                        }
                    }
                    if (!foundProperty && additional != schema.end()) {
                        if (additional->is_boolean() && !additional->get<bool>()) {
                            AddValidationError(errors, ChildPath(path, field), "This field is not allowed.");
                        } else if (additional->is_object()) {
                            ValidateSchemaNode(fieldValue, *additional, ChildPath(path, field), errors);
                        }
                    }
                }
            }

            if (value.is_array()) {
                if (const auto items = schema.find("items"); items != schema.end()) {
                    if (!items->is_object()) {
                        AddValidationError(errors, path, "The operation schema has invalid array items.");
                    } else {
                        for (std::size_t index = 0; index < value.size(); ++index) {
                            ValidateSchemaNode(value[index], *items,
                                               std::string(path) + "[" + std::to_string(index) + "]", errors);
                        }
                    }
                }
                if (const auto minimum = schema.find("minItems"); minimum != schema.end() &&
                                                                  minimum->is_number_integer() && minimum->get<std::int64_t>() >= 0 &&
                                                                  value.size() < minimum->get<std::size_t>()) {
                    AddValidationError(errors, path, "The array contains too few items.");
                }
                if (const auto maximum = schema.find("maxItems"); maximum != schema.end() &&
                                                                  maximum->is_number_integer() && maximum->get<std::int64_t>() >= 0 &&
                                                                  value.size() > maximum->get<std::size_t>()) {
                    AddValidationError(errors, path, "The array contains too many items.");
                }
            }

            if (value.is_string()) {
                const std::size_t length = value.get_ref<const std::string &>().size();
                if (const auto minimum = schema.find("minLength"); minimum != schema.end() &&
                                                                   minimum->is_number_integer() && minimum->get<std::int64_t>() >= 0 &&
                                                                   length < minimum->get<std::size_t>()) {
                    AddValidationError(errors, path, "The string is shorter than allowed.");
                }
                if (const auto maximum = schema.find("maxLength"); maximum != schema.end() &&
                                                                   maximum->is_number_integer() && maximum->get<std::int64_t>() >= 0 &&
                                                                   length > maximum->get<std::size_t>()) {
                    AddValidationError(errors, path, "The string is longer than allowed.");
                }
                if (const auto pattern = schema.find("pattern"); pattern != schema.end() && pattern->is_string()) {
                    try {
                        if (!std::regex_search(value.get_ref<const std::string &>(), std::regex(pattern->get<std::string>()))) {
                            AddValidationError(errors, path, "The string does not match the required pattern.");
                        }
                    } catch (const std::regex_error &) {
                        AddValidationError(errors, path, "The operation schema has an invalid pattern.");
                    }
                }
            }

            if (value.is_number()) {
                const double number = value.get<double>();
                for (const auto &[keyword, message] : {
                         std::pair<std::string_view, std::string_view>{"minimum", "The number is below the allowed minimum."},
                         {"maximum", "The number is above the allowed maximum."},
                         {"exclusiveMinimum", "The number is below the allowed minimum."},
                         {"exclusiveMaximum", "The number is above the allowed maximum."}}) {
                    const auto bound = schema.find(keyword);
                    if (bound == schema.end() || !bound->is_number()) {
                        continue;
                    }
                    const double limit = bound->get<double>();
                    if ((keyword == "minimum" && number < limit) ||
                        (keyword == "maximum" && number > limit) ||
                        (keyword == "exclusiveMinimum" && number <= limit) ||
                        (keyword == "exclusiveMaximum" && number >= limit)) {
                        AddValidationError(errors, path, std::string(message));
                    }
                }
            }

            if (const auto allOf = schema.find("allOf"); allOf != schema.end() && allOf->is_array()) {
                for (const auto &child : *allOf) {
                    ValidateSchemaNode(value, child, path, errors);
                }
            }
            for (const auto &[keyword, requireExactlyOne] : {
                     std::pair<std::string_view, bool>{"anyOf", false}, {"oneOf", true}}) {
                const auto alternatives = schema.find(keyword);
                if (alternatives == schema.end() || !alternatives->is_array()) {
                    continue;
                }
                std::size_t matches = 0;
                for (const auto &alternative : *alternatives) {
                    nlohmann::json alternativeErrors = nlohmann::json::array();
                    if (ValidateSchemaNode(value, alternative, path, alternativeErrors)) {
                        ++matches;
                    }
                }
                if (matches == 0 || (requireExactlyOne && matches != 1)) {
                    AddValidationError(errors, path, "The value does not match the required schema alternatives.");
                }
            }

            return errors.size() == initialErrorCount;
        }
    } // namespace

    ApiSchemaTemplate::ApiSchemaTemplate(std::filesystem::path schemaRoot)
    {
        if (schemaRoot.empty()) {
            schemaRoot = DefaultRoot();
        }

        std::error_code error;
        root = std::filesystem::absolute(schemaRoot, error).lexically_normal();
        if (error) {
            root.clear();
            TF3D_LOG_ERROR("Could not resolve API schema root '{}': {}",
                           PathText(schemaRoot), error.message());
        }
    }

    std::filesystem::path ApiSchemaTemplate::DefaultRoot()
    {
        return std::filesystem::path(GetExecutableDir()) / "Data" / "Api" / "Schemas";
    }

    std::optional<nlohmann::json> ApiSchemaTemplate::Compose(
        std::string_view templatePath,
        ApiSchemaRuntimeProvider runtime) const
    {
        if (templatePath.empty()) {
            TF3D_LOG_ERROR("API schema template path cannot be empty");
            return std::nullopt;
        }

        const auto resolvedPath = ResolvePath(std::filesystem::path(templatePath));
        if (!resolvedPath) {
            return std::nullopt;
        }

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

    std::optional<nlohmann::json> ApiSchemaTemplate::ComposeDefault(
        std::string_view templatePath,
        ApiSchemaRuntimeProvider runtime)
    {
        return ApiSchemaTemplate().Compose(templatePath, std::move(runtime));
    }

    void ApiSchemaTemplate::Merge(nlohmann::json &target, const nlohmann::json &source)
    {
        if (!target.is_object() || !source.is_object()) {
            target = source;
            return;
        }

        for (const auto &[key, value] : source.items()) {
            const auto targetValue = target.find(key);
            if (targetValue != target.end() && targetValue->is_object() && value.is_object()) {
                Merge(*targetValue, value);
            } else {
                target[key] = value;
            }
        }
    }

    bool ApiSchemaTemplate::ValidateWritable(
        const nlohmann::json &value,
        const nlohmann::json &schema,
        std::string &error)
    {
        error.clear();
        return ValidateWritableNode(value, schema, {}, error);
    }

    bool ApiSchemaTemplate::Validate(
        const nlohmann::json &value,
        const nlohmann::json &schema,
        nlohmann::json &errors)
    {
        errors = nlohmann::json::array();
        return ValidateSchemaNode(value, schema, {}, errors);
    }

    std::optional<nlohmann::json> ApiSchemaTemplate::ResolveNode(
        const nlohmann::json &node,
        const std::filesystem::path &currentFile,
        const ApiSchemaRuntimeProvider &runtime) const
    {
        if (node.is_array()) {
            nlohmann::json result = nlohmann::json::array();
            for (const auto &item : node) {
                const auto resolved = ResolveNode(item, currentFile, runtime);
                if (!resolved) {
                    return std::nullopt;
                }
                result.push_back(*resolved);
            }
            return result;
        }

        if (!node.is_object()) {
            return node;
        }

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
                TF3D_LOG_ERROR("API schema '{}' runtime value '{}' is unavailable",
                               PathText(currentFile), name);
                return std::nullopt;
            }
            result = *value;
        }

        nlohmann::json local = nlohmann::json::object();
        for (const auto &[key, value] : node.items()) {
            if (key == RuntimeDirective) {
                continue;
            }

            const auto resolved = ResolveNode(value, currentFile, runtime);
            if (!resolved) {
                return std::nullopt;
            }
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

    std::optional<std::filesystem::path> ApiSchemaTemplate::ResolvePath(
        const std::filesystem::path &path) const
    {
        if (root.empty()) {
            TF3D_LOG_ERROR("API schema root is unavailable");
            return std::nullopt;
        }

        const auto candidate           = (path.is_absolute() ? path : root / path).lexically_normal();
        const auto relative            = candidate.lexically_relative(root);
        const std::string relativeText = relative.generic_string();
        if (relative.empty() || relativeText == ".." || relativeText.starts_with("../")) {
            TF3D_LOG_ERROR("API schema path '{}' escapes Data/Api/Schemas", PathText(path));
            return std::nullopt;
        }
        return candidate;
    }

} // namespace tf3d::api
