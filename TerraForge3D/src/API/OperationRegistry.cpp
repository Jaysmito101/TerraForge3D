#include "API/OperationRegistry.h"

#include "Base/Logging/Logger.h"

#include <algorithm>
#include <string>
#include <vector>

namespace
{
    bool IsValidOperationId(std::string_view operationId)
    {
        if (operationId.empty() || operationId.size() > 128) {
            return false;
        }

        bool previousWasSeparator = true;
        for (const unsigned char character : operationId) {
            const bool isLowercaseLetter = character >= 'a' && character <= 'z';
            const bool isDigit           = character >= '0' && character <= '9';
            const bool isSeparator       = character == '.' || character == '-' || character == '_';
            if (!isLowercaseLetter && !isDigit && !isSeparator) {
                return false;
            }
            if (isSeparator && previousWasSeparator) {
                return false;
            }
            previousWasSeparator = isSeparator;
        }
        return !previousWasSeparator;
    }

    std::optional<std::string> BuildResourcePath(std::string_view operationId)
    {
        std::vector<std::string_view> segments;
        std::size_t segmentStart = 0;
        while (segmentStart <= operationId.size()) {
            const std::size_t separator  = operationId.find('.', segmentStart);
            const std::size_t segmentEnd = separator == std::string_view::npos ? operationId.size() : separator;
            segments.push_back(operationId.substr(segmentStart, segmentEnd - segmentStart));
            if (separator == std::string_view::npos) {
                break;
            }
            segmentStart = separator + 1;
        }

        if (segments.size() < 3 || segments.front() != "tf3d") {
            return std::nullopt;
        }

        std::string path;
        for (std::size_t index = 1; index < segments.size(); ++index) {
            std::string_view segment = segments[index];
            if (index == 1 && segment == "api") {
                segment = "core";
            }

            if (!path.empty()) {
                path.push_back('/');
            }
            for (const char character : segment) {
                path.push_back(character == '_' ? '-' : character);
            }
        }
        return path;
    }

    std::string ResourceGroupPath(std::string_view resourcePath)
    {
        const std::size_t separator = resourcePath.rfind('/');
        if (separator == std::string_view::npos) {
            return {};
        }
        return std::string(resourcePath.substr(0, separator));
    }

    std::string OperationRouteShape(std::string_view path)
    {
        std::string shape;
        for (std::size_t index = 0; index < path.size();) {
            if (path[index] != '{') {
                shape.push_back(path[index]);
                ++index;
                continue;
            }

            const std::size_t end = path.find('}', index + 1);
            if (end == std::string_view::npos) {
                return {};
            }
            shape += "{}";
            index = end + 1;
        }
        return shape;
    }

    bool ReadOperationString(
        const nlohmann::json &definition,
        const char *field,
        std::string &value)
    {
        if (!definition.contains(field) || !definition.at(field).is_string()) {
            TF3D_LOG_ERROR("API operation definition field '{}' must be a string", field);
            return false;
        }

        value = definition.at(field).get<std::string>();
        if (value.empty()) {
            TF3D_LOG_ERROR("API operation definition field '{}' cannot be empty", field);
            return false;
        }
        return true;
    }

    bool IsValidPathParameterName(std::string_view name)
    {
        if (name.empty()) {
            return false;
        }
        for (const unsigned char character : name) {
            const bool isLetter = (character >= 'a' && character <= 'z') ||
                                  (character >= 'A' && character <= 'Z');
            const bool isDigit = character >= '0' && character <= '9';
            if (!isLetter && !isDigit && character != '_') {
                return false;
            }
        }
        return true;
    }

    bool ReadPathParameters(std::string_view path, std::vector<std::string> &parameters)
    {
        if (!path.starts_with("/api/v1/") || path.find("//") != std::string_view::npos) {
            return false;
        }

        for (std::size_t index = 0; index < path.size(); ++index) {
            const char character = path[index];
            if (character == '{') {
                const std::size_t end = path.find('}', index + 1);
                if (end == std::string_view::npos) {
                    return false;
                }

                const std::string_view name = path.substr(index + 1, end - index - 1);
                if (!IsValidPathParameterName(name)) {
                    return false;
                }
                if (std::find(parameters.begin(), parameters.end(), name) != parameters.end()) {
                    return false;
                }
                parameters.emplace_back(name);
                index = end;
                continue;
            }

            const bool isLetter = (character >= 'a' && character <= 'z') ||
                                  (character >= 'A' && character <= 'Z');
            const bool isDigit = character >= '0' && character <= '9';
            if (!isLetter && !isDigit && character != '/' && character != '-' &&
                character != '_' && character != '.') {
                return false;
            }
        }
        return true;
    }

    bool IsScalarParameterSchema(const nlohmann::json &schema)
    {
        const auto type = schema.find("type");
        if (type == schema.end() || !type->is_string()) {
            return false;
        }

        const std::string typeName = type->get<std::string>();
        return typeName == "string" || typeName == "boolean" || typeName == "integer" || typeName == "number";
    }

    bool ParseOperationRoute(const nlohmann::json &definition, tf3d::api::OperationEntry &entry)
    {
        std::string methodName;
        if (!ReadOperationString(definition, "Method", methodName) ||
            !ReadOperationString(definition, "Path", entry.path)) {
            return false;
        }

        if (methodName == "GET") {
            entry.method = tf3d::api::ApiHttpMethod::Get;
        } else if (methodName == "POST") {
            entry.method = tf3d::api::ApiHttpMethod::Post;
        } else if (methodName == "PATCH") {
            entry.method = tf3d::api::ApiHttpMethod::Patch;
        } else {
            TF3D_LOG_ERROR("API operation '{}' has unsupported HTTP method '{}'", entry.name, methodName);
            return false;
        }

        std::vector<std::string> pathParameters;
        if (!ReadPathParameters(entry.path, pathParameters)) {
            TF3D_LOG_ERROR("API operation '{}' has an invalid path template", entry.name);
            return false;
        }

        const auto schemaType = entry.inputSchema.find("type");
        const auto properties = entry.inputSchema.find("properties");
        if (schemaType == entry.inputSchema.end() || !schemaType->is_string() ||
            schemaType->get<std::string>() != "object" ||
            (properties != entry.inputSchema.end() && !properties->is_object())) {
            TF3D_LOG_ERROR(
                "API operation '{}' input schema must describe an object with an optional object-valued 'properties' field",
                entry.name);
            return false;
        }

        const auto required = entry.inputSchema.find("required");
        for (const std::string &parameter : pathParameters) {
            const bool isRequired = required != entry.inputSchema.end() && required->is_array() &&
                                    std::any_of(required->begin(), required->end(), [&parameter](const nlohmann::json &field) {
                                        return field.is_string() && field.get<std::string>() == parameter;
                                    });
            if (properties == entry.inputSchema.end() || !properties->contains(parameter) || !isRequired) {
                TF3D_LOG_ERROR(
                    "API operation '{}' path parameter '{}' must be a required input schema property",
                    entry.name,
                    parameter);
                return false;
            }
            if (!IsScalarParameterSchema(properties->at(parameter))) {
                TF3D_LOG_ERROR(
                    "API operation '{}' path parameter '{}' must have a scalar schema type",
                    entry.name,
                    parameter);
                return false;
            }
        }

        if (entry.method == tf3d::api::ApiHttpMethod::Get) {
            if (properties != entry.inputSchema.end()) {
                for (auto property = properties->begin(); property != properties->end(); ++property) {
                    const bool isPathParameter =
                        std::find(pathParameters.begin(), pathParameters.end(), property.key()) != pathParameters.end();
                    if (isPathParameter || IsScalarParameterSchema(property.value())) {
                        continue;
                    }

                    TF3D_LOG_ERROR(
                        "API operation '{}' query parameter '{}' must have a scalar schema type",
                        entry.name,
                        property.key());
                    return false;
                }
            }
        }
        return true;
    }

    bool ParseOperationFlags(const nlohmann::json &definition, tf3d::api::OperationFlags &flags)
    {
        flags = tf3d::api::OperationFlags::None;
        if (!definition.contains("Flags")) {
            return true;
        }

        const auto &values = definition.at("Flags");
        if (!values.is_array()) {
            TF3D_LOG_ERROR("API operation definition field 'Flags' must be an array");
            return false;
        }

        for (const auto &value : values) {
            if (!value.is_string()) {
                TF3D_LOG_ERROR("API operation definition 'Flags' entries must be strings");
                return false;
            }

            const std::string flag = value.get<std::string>();
            if (flag == "None") {
                continue;
            } else if (flag == "ReadOnly") {
                flags = static_cast<tf3d::api::OperationFlags>(
                    static_cast<std::uint32_t>(flags) |
                    static_cast<std::uint32_t>(tf3d::api::OperationFlags::ReadOnly));
            } else if (flag == "Destructive") {
                flags = static_cast<tf3d::api::OperationFlags>(
                    static_cast<std::uint32_t>(flags) |
                    static_cast<std::uint32_t>(tf3d::api::OperationFlags::Destructive));
            } else if (flag == "LongRunning") {
                flags = static_cast<tf3d::api::OperationFlags>(
                    static_cast<std::uint32_t>(flags) |
                    static_cast<std::uint32_t>(tf3d::api::OperationFlags::LongRunning));
            } else {
                TF3D_LOG_ERROR("API operation definition has unknown flag '{}'", flag);
                return false;
            }
        }
        return true;
    }
} // namespace

namespace tf3d::api
{

    std::optional<OperationEntry> CreateOperationEntryFromJson(
        const nlohmann::json &definition,
        ApiOperationHandler invoke)
    {
        if (!definition.is_object()) {
            TF3D_LOG_ERROR("API operation definition must be a JSON object");
            return std::nullopt;
        }
        if (!invoke) {
            TF3D_LOG_ERROR("API operation definition has no invocation handler");
            return std::nullopt;
        }

        OperationEntry entry;
        if (!ReadOperationString(definition, "Name", entry.name) ||
            !ReadOperationString(definition, "Title", entry.title) ||
            !ReadOperationString(definition, "Description", entry.description)) {
            return std::nullopt;
        }
        if (!IsValidOperationId(entry.name)) {
            TF3D_LOG_ERROR("API operation name '{}' is not a valid operation id", entry.name);
            return std::nullopt;
        }
        const auto resourcePath = BuildResourcePath(entry.name);
        if (!resourcePath) {
            TF3D_LOG_ERROR("API operation '{}' does not have a supported hierarchical operation id", entry.name);
            return std::nullopt;
        }
        entry.resourcePath = *resourcePath;

        if (!definition.contains("InputSchema") || !definition.at("InputSchema").is_object()) {
            TF3D_LOG_ERROR("API operation definition field 'InputSchema' must be a JSON object");
            return std::nullopt;
        }
        entry.inputSchema = definition.at("InputSchema");

        if (!ParseOperationRoute(definition, entry)) {
            return std::nullopt;
        }

        if (definition.contains("Annotations")) {
            if (!definition.at("Annotations").is_object()) {
                TF3D_LOG_ERROR("API operation definition field 'Annotations' must be a JSON object");
                return std::nullopt;
            }
            entry.annotations = definition.at("Annotations");
        }

        if (!ParseOperationFlags(definition, entry.flags)) {
            return std::nullopt;
        }

        entry.invoke = std::move(invoke);
        return entry;
    }

    void RegisterOperationFromJson(
        OperationRegistry &operations,
        const std::optional<nlohmann::json> &definition,
        ApiOperationHandler invoke)
    {
        if (!definition) {
            return;
        }

        auto entry = CreateOperationEntryFromJson(*definition, std::move(invoke));
        if (!entry) {
            return;
        }

        const std::string operationId = entry->name;
        ApiOperationHandler handler   = std::move(entry->invoke);
        entry->invoke                 = [operationId, handler = std::move(handler)](
                            const nlohmann::json &arguments) mutable {
            ApiResult result = handler(arguments);
            if (result.operationId.empty()) {
                result.operationId = operationId;
            }
            return result;
        };

        if (operations.Register(std::move(*entry))) {
            return;
        }

        TF3D_LOG_ERROR("Could not register API operation '{}'", operationId);
    }

    bool OperationRegistry::Register(OperationEntry entry)
    {
        if (entry.name.empty() || entry.path.empty() || entry.resourcePath.empty() || !entry.invoke) {
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex);
        const std::string groupPath  = ResourceGroupPath(entry.resourcePath);
        const std::string routeShape = OperationRouteShape(entry.path);
        for (const auto &[name, registered] : entries) {
            if (registered.method == entry.method && OperationRouteShape(registered.path) == routeShape) {
                return false;
            }

            const std::string registeredGroup = ResourceGroupPath(registered.resourcePath);
            if (registered.resourcePath == entry.resourcePath || registered.resourcePath == groupPath ||
                entry.resourcePath == registeredGroup) {
                return false;
            }
        }

        std::string name = entry.name;
        return entries.emplace(std::move(name), std::move(entry)).second;
    }

    bool OperationRegistry::Unregister(const std::string &name)
    {
        std::lock_guard<std::mutex> lock(mutex);
        return entries.erase(name) != 0;
    }

    std::optional<OperationEntry> OperationRegistry::Find(const std::string &name) const
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto found = entries.find(name);
        if (found == entries.end()) {
            return std::nullopt;
        }
        return found->second;
    }

    std::vector<OperationEntry> OperationRegistry::Snapshot() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<OperationEntry> snapshot;
        snapshot.reserve(entries.size());
        for (const auto &[name, entry] : entries) {
            snapshot.push_back(entry);
        }
        return snapshot;
    }

    void OperationRegistry::Clear()
    {
        std::lock_guard<std::mutex> lock(mutex);
        entries.clear();
    }

} // namespace tf3d::api
