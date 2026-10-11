#include "API/ApiServer.h"

#include <httplib.h>

#include "API/Operations/CoreOperations.h"
#include "API/Operations/GenerationOperations.h"
#include "API/Operations/SceneOperations.h"
#include "API/Operations/ViewportOperations.h"
#include "API/SchemaTemplate.h"
#include "Base/Logging/Logger.h"
#include "Data/VersionInfo.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <set>
#include <sstream>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace tf3d::api
{

    namespace
    {
        constexpr std::string_view ApiPrefix          = "/api/v1";
        constexpr std::string_view StatusOperation    = "tf3d.api.status";
        constexpr std::size_t MaxRequestLogEntries    = 100;
        constexpr std::size_t MaxRequestPreviewLength = 2048;

        enum class DirectoryKind {
            Operations,
            Skills
        };

        int ReadApiPort()
        {
            constexpr int defaultPort  = 9823;
            const char *configuredPort = std::getenv("TF3D_API_PORT");
            if (configuredPort == nullptr || configuredPort[0] == '\0') {
                return defaultPort;
            }

            char *end             = nullptr;
            const long parsedPort = std::strtol(configuredPort, &end, 10);
            if (end == configuredPort || *end != '\0' || parsedPort < 1 || parsedPort > 65535) {
                TF3D_LOG_WARN("Ignoring invalid TF3D_API_PORT='{}'; using {}", configuredPort, defaultPort);
                return defaultPort;
            }
            return static_cast<int>(parsedPort);
        }

        std::string FormatTimestamp()
        {
            const auto now         = std::chrono::system_clock::now();
            const std::time_t time = std::chrono::system_clock::to_time_t(now);
            std::tm localTime{};
#ifdef _WIN32
            localtime_s(&localTime, &time);
#else
            localtime_r(&time, &localTime);
#endif
            std::ostringstream formatted;
            formatted << std::put_time(&localTime, "%H:%M:%S");
            return formatted.str();
        }

        std::string EscapeRegexLiteral(std::string_view value)
        {
            constexpr std::string_view RegexCharacters = R"(\.^$|()[]{}*+?)";
            std::string escaped;
            escaped.reserve(value.size());
            for (const char character : value) {
                if (RegexCharacters.find(character) != std::string_view::npos) {
                    escaped.push_back('\\');
                }
                escaped.push_back(character);
            }
            return escaped;
        }

        std::vector<std::string> PathParameterNames(std::string_view path)
        {
            std::vector<std::string> names;
            for (std::size_t index = 0; index < path.size(); ++index) {
                if (path[index] != '{') {
                    continue;
                }
                const std::size_t end = path.find('}', index + 1);
                if (end == std::string_view::npos) {
                    return {};
                }
                names.emplace_back(path.substr(index + 1, end - index - 1));
                index = end;
            }
            return names;
        }

        std::string RoutePattern(std::string_view path)
        {
            std::string pattern = "^";
            for (std::size_t index = 0; index < path.size();) {
                if (path[index] == '{') {
                    const std::size_t end = path.find('}', index + 1);
                    if (end == std::string_view::npos) {
                        return {};
                    }
                    pattern += "([^/]+)";
                    index = end + 1;
                    continue;
                }

                const std::size_t parameter = path.find('{', index);
                const std::size_t length    = parameter == std::string_view::npos
                                                  ? path.size() - index
                                                  : parameter - index;
                pattern += EscapeRegexLiteral(path.substr(index, length));
                index += length;
            }
            pattern += '$';
            return pattern;
        }

        std::string StaticRoutePattern(std::string_view path)
        {
            return "^" + EscapeRegexLiteral(path) + "$";
        }

        const nlohmann::json *FindPropertySchema(const nlohmann::json &schema, std::string_view name)
        {
            const auto properties = schema.find("properties");
            if (properties == schema.end() || !properties->is_object()) {
                return nullptr;
            }

            const auto property = properties->find(std::string(name));
            return property == properties->end() ? nullptr : &*property;
        }

        nlohmann::json NonPathArgumentSchema(const OperationEntry &entry)
        {
            nlohmann::json schema                         = entry.inputSchema;
            const std::vector<std::string> pathParameters = PathParameterNames(entry.path);
            const auto properties                         = schema.find("properties");
            if (properties != schema.end() && properties->is_object()) {
                for (const std::string &parameter : pathParameters) {
                    properties->erase(parameter);
                }
            }

            const auto required = schema.find("required");
            if (required != schema.end() && required->is_array()) {
                nlohmann::json filtered = nlohmann::json::array();
                for (const auto &field : *required) {
                    if (!field.is_string()) {
                        filtered.push_back(field);
                        continue;
                    }
                    const std::string name = field.get<std::string>();
                    if (std::find(pathParameters.begin(), pathParameters.end(), name) == pathParameters.end()) {
                        filtered.push_back(name);
                    }
                }
                *required = std::move(filtered);
            }
            return schema;
        }

        std::string ResourceGroupPath(const OperationEntry &entry)
        {
            const std::size_t separator = entry.resourcePath.rfind('/');
            if (separator == std::string::npos) {
                return {};
            }
            return entry.resourcePath.substr(0, separator);
        }

        std::string OperationDocumentPath(const OperationEntry &entry)
        {
            return std::string(ApiPrefix) + "/operations/" + entry.resourcePath;
        }

        std::string OperationSchemaPath(const OperationEntry &entry)
        {
            return OperationDocumentPath(entry) + "/schema";
        }

        std::string OperationResponseSchemaPath(const OperationEntry &entry)
        {
            return OperationDocumentPath(entry) + "/response-schema";
        }

        std::string OpenApiOperationSchemaReference(const OperationEntry &entry)
        {
            return "operations/" + entry.resourcePath + "/schema";
        }

        std::string OpenApiOperationResponseSchemaReference(const OperationEntry &entry)
        {
            return "operations/" + entry.resourcePath + "/response-schema";
        }

        std::string OperationSkillPath(const OperationEntry &entry)
        {
            return std::string(ApiPrefix) + "/skills/" + entry.resourcePath + ".md";
        }

        bool HasFlag(OperationFlags flags, OperationFlags flag);

        std::string LowercaseMethod(ApiHttpMethod method)
        {
            std::string name(ApiHttpMethodName(method));
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            });
            return name;
        }

        nlohmann::json ParameterList(const OperationEntry &entry, bool pathParameters)
        {
            nlohmann::json parameters                = nlohmann::json::array();
            const std::vector<std::string> pathNames = PathParameterNames(entry.path);
            const auto properties                    = entry.inputSchema.find("properties");
            if (properties == entry.inputSchema.end() || !properties->is_object()) {
                return parameters;
            }

            for (auto property = properties->begin(); property != properties->end(); ++property) {
                const bool isPathParameter =
                    std::find(pathNames.begin(), pathNames.end(), property.key()) != pathNames.end();
                if (isPathParameter != pathParameters) {
                    continue;
                }

                bool isRequired = isPathParameter;
                if (!isRequired) {
                    const auto required = entry.inputSchema.find("required");
                    isRequired          = required != entry.inputSchema.end() && required->is_array() &&
                                 std::any_of(required->begin(), required->end(), [&property](const nlohmann::json &field) {
                                     return field.is_string() && field.get<std::string>() == property.key();
                                 });
                }
                parameters.push_back({{"name", property.key()},
                                      {"required", isRequired},
                                      {"schema", property.value()}});
            }
            return parameters;
        }

        nlohmann::json BuildDirectoryDocument(
            const std::vector<OperationEntry> &entries,
            std::string_view groupPath,
            std::string_view directoryPath,
            DirectoryKind kind)
        {
            const bool skillDirectory     = kind == DirectoryKind::Skills;
            const std::string childPrefix = groupPath.empty() ? std::string{} : std::string(groupPath) + "/";
            std::set<std::string> childGroups;
            nlohmann::json operations = nlohmann::json::array();

            for (const OperationEntry &entry : entries) {
                const std::string parentGroup = ResourceGroupPath(entry);
                if (parentGroup == groupPath) {
                    const std::string action = entry.resourcePath.substr(
                        groupPath.empty() ? 0 : groupPath.size() + 1);
                    nlohmann::json summary = {
                        {"name", action},
                        {"operationId", entry.name},
                        {"title", entry.title},
                        {"description", entry.description},
                        {"method", ApiHttpMethodName(entry.method)},
                        {"path", entry.path},
                        {"readOnly", HasFlag(entry.flags, OperationFlags::ReadOnly)},
                        {"destructive", HasFlag(entry.flags, OperationFlags::Destructive)},
                        {"longRunning", HasFlag(entry.flags, OperationFlags::LongRunning)}};
                    summary["document"]  = skillDirectory ? OperationSkillPath(entry) : OperationDocumentPath(entry);
                    summary["operation"] = OperationDocumentPath(entry);
                    summary["schema"]    = OperationSchemaPath(entry);
                    summary["skill"]     = OperationSkillPath(entry);
                    if (entry.responseSchema) {
                        summary["responseSchema"] = OperationResponseSchemaPath(entry);
                    }
                    operations.push_back(std::move(summary));
                    continue;
                }

                if (!childPrefix.empty() && !parentGroup.starts_with(childPrefix)) {
                    continue;
                }
                if (parentGroup == groupPath) {
                    continue;
                }

                const std::string remainder = parentGroup.substr(childPrefix.size());
                const std::size_t separator = remainder.find('/');
                const std::string child(remainder.substr(0, separator));
                if (!child.empty()) {
                    childGroups.insert(child);
                }
            }

            const std::string currentPath = groupPath.empty()
                                                ? std::string(directoryPath)
                                                : std::string(directoryPath) + "/" + std::string(groupPath);
            nlohmann::json children       = nlohmann::json::array();
            for (const std::string &child : childGroups) {
                const std::string childPath = groupPath.empty() ? child : std::string(groupPath) + "/" + child;
                children.push_back({{"name", child}, {"path", std::string(directoryPath) + "/" + childPath}});
            }

            return {{"path", currentPath}, {"children", std::move(children)}, {skillDirectory ? "skills" : "operations", std::move(operations)}};
        }

        std::optional<nlohmann::json> ParseParameterValue(std::string_view value, const nlohmann::json &schema)
        {
            const auto type = schema.find("type");
            if (type == schema.end() || !type->is_string()) {
                return std::nullopt;
            }

            const std::string typeName = type->get<std::string>();
            if (typeName == "string") {
                return nlohmann::json(std::string(value));
            }
            if (typeName == "boolean") {
                if (value == "true") {
                    return nlohmann::json(true);
                }
                if (value == "false") {
                    return nlohmann::json(false);
                }
                return std::nullopt;
            }
            if (typeName == "integer") {
                std::int64_t number     = 0;
                const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), number);
                if (error != std::errc{} || end != value.data() + value.size()) {
                    return std::nullopt;
                }
                return nlohmann::json(number);
            }
            if (typeName == "number") {
                double number           = 0.0;
                const auto [end, error] = std::from_chars(
                    value.data(), value.data() + value.size(), number, std::chars_format::general);
                if (error != std::errc{} || end != value.data() + value.size() || !std::isfinite(number)) {
                    return std::nullopt;
                }
                return nlohmann::json(number);
            }
            return std::nullopt;
        }

        std::string SkillName(std::string_view operationId)
        {
            std::string name;
            name.reserve(operationId.size());
            for (const unsigned char character : operationId) {
                if (std::isalnum(character) != 0) {
                    name.push_back(static_cast<char>(std::tolower(character)));
                } else if (name.empty() || name.back() != '-') {
                    name.push_back('-');
                }
            }
            while (!name.empty() && name.back() == '-') {
                name.pop_back();
            }
            return name;
        }

        std::optional<std::string> ReadTextFile(const std::filesystem::path &path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open()) {
                return std::nullopt;
            }

            std::ostringstream content;
            content << file.rdbuf();
            if (file.bad()) {
                return std::nullopt;
            }
            return content.str();
        }

        bool HasFlag(OperationFlags flags, OperationFlags flag)
        {
            return (static_cast<std::uint32_t>(flags) & static_cast<std::uint32_t>(flag)) != 0;
        }

        bool HasJsonContentType(const httplib::Request &request)
        {
            std::string contentType      = request.get_header_value("Content-Type");
            const std::size_t parameters = contentType.find(';');
            if (parameters != std::string::npos) {
                contentType.resize(parameters);
            }
            std::transform(contentType.begin(), contentType.end(), contentType.begin(), [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            });

            const std::size_t first = contentType.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
                return false;
            }
            const std::size_t last = contentType.find_last_not_of(" \t\r\n");
            return contentType.substr(first, last - first + 1) == "application/json";
        }

        struct ActiveRequestScope {
            explicit ActiveRequestScope(std::atomic<std::uint64_t> &requestCount)
                : count(requestCount)
            {
                count.fetch_add(1, std::memory_order_relaxed);
            }

            ~ActiveRequestScope()
            {
                count.fetch_sub(1, std::memory_order_relaxed);
            }

            std::atomic<std::uint64_t> &count;
        };

        nlohmann::json ProblemSchema()
        {
            nlohmann::json schema = {
                {"type", "object"},
                {"properties", nlohmann::json::object()},
                {"required", nlohmann::json::array({"type", "title", "status", "detail", "code"})}};
            schema["properties"]["type"]    = {{"type", "string"}};
            schema["properties"]["title"]   = {{"type", "string"}};
            schema["properties"]["status"]  = {{"type", "integer"}};
            schema["properties"]["detail"]  = {{"type", "string"}};
            schema["properties"]["code"]    = {{"type", "string"}};
            schema["properties"]["details"] = {{"type", "object"}};
            return schema;
        }

        void AddProblemResponse(nlohmann::json &operation, int status, std::string description)
        {
            const std::string statusKey                                                        = std::to_string(status);
            operation["responses"][statusKey]["description"]                                   = std::move(description);
            operation["responses"][statusKey]["content"]["application/problem+json"]["schema"] = {
                {"$ref", "#/components/schemas/Problem"}};
        }

        bool ValidateOperationInventory(const OperationRegistry &operations)
        {
            const std::filesystem::path definitionsRoot = ApiSchemaTemplate::DefaultRoot() / "Operations";
            std::error_code error;
            if (!std::filesystem::is_directory(definitionsRoot, error) || error) {
                TF3D_LOG_ERROR(
                    "API operation definitions directory '{}' is unavailable: {}",
                    definitionsRoot.generic_string(),
                    error ? error.message() : "directory does not exist");
                return false;
            }

            std::set<std::string> expectedNames;
            std::filesystem::recursive_directory_iterator iterator(definitionsRoot, error);
            const std::filesystem::recursive_directory_iterator end;
            if (error) {
                TF3D_LOG_ERROR(
                    "Could not enumerate API operation definitions in '{}': {}",
                    definitionsRoot.generic_string(),
                    error.message());
                return false;
            }

            while (iterator != end) {
                const std::filesystem::path definitionPath = iterator->path();
                const bool isDefinitionFile                = iterator->is_regular_file(error) &&
                                              definitionPath.extension() == ".json" &&
                                              definitionPath.parent_path().filename() == "Definitions";
                if (error) {
                    TF3D_LOG_ERROR(
                        "Could not inspect API operation definition '{}': {}",
                        definitionPath.generic_string(),
                        error.message());
                    return false;
                }

                if (isDefinitionFile) {
                    std::ifstream definitionFile(definitionPath);
                    if (!definitionFile.is_open()) {
                        TF3D_LOG_ERROR(
                            "Could not read API operation definition '{}'",
                            definitionPath.generic_string());
                        return false;
                    }

                    nlohmann::json definition;
                    try {
                        definitionFile >> definition;
                    } catch (const std::exception &exception) {
                        TF3D_LOG_ERROR(
                            "Could not parse API operation definition '{}': {}",
                            definitionPath.generic_string(),
                            exception.what());
                        return false;
                    }

                    if (!definition.is_object()) {
                        TF3D_LOG_ERROR(
                            "API operation definition '{}' must be a JSON object",
                            definitionPath.generic_string());
                        return false;
                    }

                    const auto name = definition.find("Name");
                    if (name == definition.end() || !name->is_string() ||
                        name->get_ref<const std::string &>().empty()) {
                        TF3D_LOG_ERROR(
                            "API operation definition '{}' must declare a non-empty string Name",
                            definitionPath.generic_string());
                        return false;
                    }

                    const std::string operationName = name->get<std::string>();
                    if (!expectedNames.insert(operationName).second) {
                        TF3D_LOG_ERROR("API operation name '{}' is declared more than once", operationName);
                        return false;
                    }
                }

                iterator.increment(error);
                if (error) {
                    TF3D_LOG_ERROR(
                        "Could not enumerate API operation definitions in '{}': {}",
                        definitionsRoot.generic_string(),
                        error.message());
                    return false;
                }
            }

            const std::vector<OperationEntry> registeredEntries = operations.Snapshot();
            std::set<std::string> registeredNames;
            for (const OperationEntry &entry : registeredEntries) {
                registeredNames.insert(entry.name);
            }

            for (const std::string &name : expectedNames) {
                if (!registeredNames.contains(name)) {
                    TF3D_LOG_ERROR("API operation '{}' was not registered", name);
                }
            }
            for (const std::string &name : registeredNames) {
                if (!expectedNames.contains(name)) {
                    TF3D_LOG_ERROR("API operation '{}' has no schema definition", name);
                }
            }

            if (expectedNames.empty() || expectedNames != registeredNames) {
                TF3D_LOG_ERROR(
                    "API operation registration is incomplete: {} definitions, {} registered",
                    expectedNames.size(),
                    registeredNames.size());
                return false;
            }

            TF3D_LOG_INFO("Registered all {} API operations", registeredNames.size());
            return true;
        }
    } // namespace

    ApiServer::ApiServer(tf3d::data::ApplicationState *applicationState, std::string logsDirectory)
        : applicationState(applicationState), logsDirectory(std::move(logsDirectory)), port(ReadApiPort())
    {
        endpoint = "http://" + host + ":" + std::to_string(port) + std::string(ApiPrefix);
        RegisterOperations();
        openApiDocument = BuildOpenApiDocument();
    }

    ApiServer::~ApiServer()
    {
        Stop();
    }

    void ApiServer::RegisterOperations()
    {
        RegisterCoreOperations(operations, applicationState);
        RegisterGenerationOperations(operations, applicationState);
        RegisterViewportOperations(operations, applicationState);
        RegisterSceneOperations(operations, applicationState);
        operationRegistryComplete = ValidateOperationInventory(operations);
    }

    bool ApiServer::Start()
    {
        if (!operationRegistryComplete) {
            TF3D_LOG_ERROR("API server will not start with an incomplete operation registry");
            return false;
        }

        if (server && server->is_running()) {
            return true;
        }

        startFailed.store(false, std::memory_order_release);
        server = std::make_unique<httplib::Server>();
        server->set_payload_max_length(2 * 1024 * 1024);
        server->set_read_timeout(5, 0);
        server->set_write_timeout(10, 0);
        RegisterRoutes();

        try {
            const auto apiDataDirectory = ApiSchemaTemplate::DefaultRoot().parent_path();
            std::filesystem::create_directories(apiDataDirectory);
            std::ofstream specFile(apiDataDirectory / "openapi.json", std::ios::out | std::ios::trunc);
            if (specFile.is_open()) {
                specFile << openApiDocument.dump(2) << '\n';
            }
        } catch (const std::exception &exception) {
            TF3D_LOG_WARN("Could not write the generated OpenAPI document: {}", exception.what());
        }

        if (!server->bind_to_port(host, port)) {
            TF3D_LOG_ERROR("Could not bind the API server to http://{}:{}", host, port);
            server.reset();
            return false;
        }

        OpenCallHistory();
        serverThread = std::thread([this]() {
            try {
                if (!server->listen_after_bind() && !requestQueue.IsShutdown()) {
                    startFailed.store(true, std::memory_order_release);
                    TF3D_LOG_ERROR("The API server stopped unexpectedly");
                }
            } catch (const std::exception &exception) {
                startFailed.store(true, std::memory_order_release);
                TF3D_LOG_ERROR("The API server failed: {}", exception.what());
            } catch (...) {
                startFailed.store(true, std::memory_order_release);
                TF3D_LOG_ERROR("The API server failed with an unknown error");
            }
        });

        for (int attempt = 0; attempt < 100; ++attempt) {
            if (server->is_running()) {
                TF3D_LOG_INFO("API server listening on {}", endpoint);
                return true;
            }
            if (startFailed.load(std::memory_order_acquire)) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        TF3D_LOG_ERROR("The API server did not start listening on {}", endpoint);
        Stop();
        return false;
    }

    void ApiServer::RegisterRoutes()
    {
        server->Get(StaticRoutePattern("/api/v1/operations"), [this](const httplib::Request &request, httplib::Response &response) {
            ActiveRequestScope active(activeRequests);
            response.set_content(BuildOperationsDocument({}).dump(2), "application/json");
            RecordRequest(request, response);
        });

        server->Get(StaticRoutePattern("/api/v1/openapi.json"), [this](const httplib::Request &request, httplib::Response &response) {
            ActiveRequestScope active(activeRequests);
            response.set_content(openApiDocument.dump(2), "application/json");
            RecordRequest(request, response);
        });

        server->Get(StaticRoutePattern("/api/v1/skills"), [this](const httplib::Request &request, httplib::Response &response) {
            ActiveRequestScope active(activeRequests);
            response.set_content(BuildSkillsDocument({}).dump(2), "application/json");
            RecordRequest(request, response);
        });

        server->Get(StaticRoutePattern("/api/skill.md"), [this](const httplib::Request &request, httplib::Response &response) {
            ActiveRequestScope active(activeRequests);
            const auto skillPath = ApiSchemaTemplate::DefaultRoot().parent_path() / "SKILL.md";
            const auto skill     = ReadTextFile(skillPath);
            if (!skill) {
                response.status = 404;
                response.set_content("The API skill guide is unavailable.", "text/plain");
            } else {
                response.set_content(*skill, "text/markdown; charset=utf-8");
            }
            RecordRequest(request, response);
        });

        const auto servePlayground = [this](const httplib::Request &request, httplib::Response &response) {
            ActiveRequestScope active(activeRequests);
            const auto playgroundPath = ApiSchemaTemplate::DefaultRoot().parent_path() /
                                        "Playground" / "index.html";
            const auto playground = ReadTextFile(playgroundPath);
            if (!playground) {
                response.status = 404;
                response.set_content("API playground assets are unavailable.", "text/plain");
            } else {
                response.set_content(*playground, "text/html; charset=utf-8");
            }
            RecordRequest(request, response);
        };
        server->Get(StaticRoutePattern("/api/docs"), servePlayground);
        server->Get(StaticRoutePattern("/api/docs/"), servePlayground);

        const std::vector<OperationEntry> entries = operations.Snapshot();
        std::set<std::string> groupPaths;
        for (const OperationEntry &entry : entries) {
            std::string groupPath = ResourceGroupPath(entry);
            while (!groupPath.empty()) {
                groupPaths.insert(groupPath);
                const std::size_t separator = groupPath.rfind('/');
                if (separator == std::string::npos) {
                    break;
                }
                groupPath.resize(separator);
            }
        }

        for (const std::string &groupPath : groupPaths) {
            const std::string operationDirectory = "/api/v1/operations/" + groupPath;
            const std::string skillDirectory     = "/api/v1/skills/" + groupPath;
            server->Get(StaticRoutePattern(operationDirectory), [this, groupPath](const httplib::Request &request,
                                                                                  httplib::Response &response) {
                ActiveRequestScope active(activeRequests);
                response.set_content(BuildOperationsDocument(groupPath).dump(2), "application/json");
                RecordRequest(request, response);
            });
            server->Get(StaticRoutePattern(skillDirectory), [this, groupPath](const httplib::Request &request,
                                                                              httplib::Response &response) {
                ActiveRequestScope active(activeRequests);
                response.set_content(BuildSkillsDocument(groupPath).dump(2), "application/json");
                RecordRequest(request, response);
            });
        }

        for (const OperationEntry &entry : entries) {
            const std::string operationDocumentPath = OperationDocumentPath(entry);
            server->Get(StaticRoutePattern(operationDocumentPath), [this, entry](const httplib::Request &request,
                                                                                 httplib::Response &response) {
                ActiveRequestScope active(activeRequests);
                response.set_content(BuildOperationDocument(entry).dump(2), "application/json");
                RecordRequest(request, response);
            });

            const std::string schemaPath = OperationSchemaPath(entry);
            server->Get(StaticRoutePattern(schemaPath), [this, entry](const httplib::Request &request, httplib::Response &response) {
                ActiveRequestScope active(activeRequests);
                response.set_content(BuildOperationSchema(entry, "./schema").dump(2), "application/schema+json");
                RecordRequest(request, response);
            });

            if (entry.responseSchema) {
                const std::string responseSchemaPath = OperationResponseSchemaPath(entry);
                server->Get(StaticRoutePattern(responseSchemaPath), [this, entry](const httplib::Request &request,
                                                                                  httplib::Response &response) {
                    ActiveRequestScope active(activeRequests);
                    response.set_content(
                        BuildOperationResponseSchema(entry, "./response-schema").dump(2),
                        "application/schema+json");
                    RecordRequest(request, response);
                });
            }

            const std::string skillPath = OperationSkillPath(entry);
            server->Get(StaticRoutePattern(skillPath), [this, entry](const httplib::Request &request, httplib::Response &response) {
                ActiveRequestScope active(activeRequests);
                response.set_content(BuildOperationSkill(entry), "text/markdown; charset=utf-8");
                RecordRequest(request, response);
            });

            const std::string routePattern = RoutePattern(entry.path);
            const auto handler             = [this, name = entry.name](const httplib::Request &request,
                                                           httplib::Response &response) {
                ActiveRequestScope active(activeRequests);
                WriteResult(InvokeOperation(name, request), response);
                RecordRequest(request, response);
            };
            switch (entry.method) {
                case ApiHttpMethod::Get:
                    server->Get(routePattern, handler);
                    break;
                case ApiHttpMethod::Post:
                    server->Post(routePattern, handler);
                    break;
                case ApiHttpMethod::Patch:
                    server->Patch(routePattern, handler);
                    break;
            }
        }
    }

    ApiResult ApiServer::InvokeOperation(const std::string &name, const httplib::Request &request)
    {
        const auto entry = operations.Find(name);
        if (!entry) {
            return ApiResult::Failure(ApiErrorType::InvalidArguments, "The requested operation does not exist.");
        }

        if (request.method != ApiHttpMethodName(entry->method)) {
            return ApiResult::Failure(ApiErrorType::InvalidArguments, "The HTTP method does not match this operation.");
        }

        nlohmann::json arguments                 = nlohmann::json::object();
        const std::vector<std::string> pathNames = PathParameterNames(entry->path);
        if (request.matches.size() != pathNames.size() + 1) {
            return ApiResult::Failure(ApiErrorType::InvalidArguments, "The request path parameters are invalid.");
        }

        for (std::size_t index = 0; index < pathNames.size(); ++index) {
            const std::string &name      = pathNames[index];
            const nlohmann::json *schema = FindPropertySchema(entry->inputSchema, name);
            if (schema == nullptr) {
                return ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "The operation path parameter is missing from its input schema.",
                    {{"parameter", name}});
            }
            const std::optional<nlohmann::json> value = ParseParameterValue(request.matches[index + 1].str(), *schema);
            if (!value) {
                return ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "A path parameter has the wrong type.",
                    {{"parameter", name}});
            }
            arguments[name] = *value;
        }

        if (entry->method == ApiHttpMethod::Get) {
            if (!request.body.empty()) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments, "GET operations do not accept a request body.");
            }

            std::set<std::string> queryNames;
            for (const auto &[name, rawValue] : request.params) {
                if (!queryNames.insert(name).second ||
                    std::find(pathNames.begin(), pathNames.end(), name) != pathNames.end()) {
                    return ApiResult::Failure(
                        ApiErrorType::InvalidArguments,
                        "A query parameter is duplicated or conflicts with a path parameter.",
                        {{"parameter", name}});
                }

                const nlohmann::json *schema = FindPropertySchema(entry->inputSchema, name);
                if (schema == nullptr) {
                    return ApiResult::Failure(
                        ApiErrorType::InvalidArguments,
                        "The request contains an unknown query parameter.",
                        {{"parameter", name}});
                }
                const std::optional<nlohmann::json> value = ParseParameterValue(rawValue, *schema);
                if (!value) {
                    return ApiResult::Failure(
                        ApiErrorType::InvalidArguments,
                        "A query parameter has the wrong type.",
                        {{"parameter", name}});
                }
                arguments[name] = *value;
            }
        } else {
            if (!request.params.empty()) {
                return ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "POST and PATCH operations do not accept query parameters.");
            }

            if (request.body.empty()) {
                return ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "POST and PATCH operations require a JSON request body.");
            }
            if (!HasJsonContentType(request)) {
                return ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "Operation request bodies must use application/json.");
            }

            try {
                const nlohmann::json body = nlohmann::json::parse(request.body);
                if (!body.is_object()) {
                    return ApiResult::Failure(ApiErrorType::InvalidArguments, "The request body must be a JSON object.");
                }
                for (const std::string &pathName : pathNames) {
                    if (body.contains(pathName)) {
                        return ApiResult::Failure(
                            ApiErrorType::InvalidArguments,
                            "Path parameters must be supplied in the URL, not in the request body.",
                            {{"parameter", pathName}});
                    }
                }
                for (auto field = body.begin(); field != body.end(); ++field) {
                    arguments[field.key()] = field.value();
                }
            } catch (const nlohmann::json::parse_error &) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments, "The request body is not valid JSON.");
            }
        }

        nlohmann::json validationErrors = nlohmann::json::array();
        if (!ApiSchemaTemplate::Validate(arguments, entry->inputSchema, validationErrors)) {
            return ApiResult::Failure(
                ApiErrorType::InvalidArguments,
                "The request arguments do not match the operation schema.",
                {{"errors", std::move(validationErrors)}});
        }

        return requestQueue.Execute([entry = *entry, arguments = std::move(arguments)]() {
            return entry.invoke(arguments);
        });
    }

    void ApiServer::WriteResult(const ApiResult &result, httplib::Response &response)
    {
        if (const ApiFailure *failure = result.Failure()) {
            const int status = ApiErrorTypeToHttpStatus(failure->type);
            const std::string code(ApiErrorTypeToCode(failure->type));
            const nlohmann::json problem = {
                {"type", "about:blank"},
                {"title", code},
                {"status", status},
                {"detail", failure->message},
                {"code", code},
                {"details", failure->details}};
            response.status = status;
            response.set_content(problem.dump(), "application/problem+json");
            return;
        }

        if (const ApiBinaryBody *binary = result.BinaryBody()) {
            std::string bytes;
            if (!binary->bytes.empty()) {
                bytes.assign(reinterpret_cast<const char *>(binary->bytes.data()), binary->bytes.size());
            }
            response.set_content(bytes, binary->contentType);
            if (binary->metadata.contains("Width")) {
                response.set_header("X-Image-Width", std::to_string(binary->metadata.at("Width").get<int>()));
            }
            if (binary->metadata.contains("Height")) {
                response.set_header("X-Image-Height", std::to_string(binary->metadata.at("Height").get<int>()));
            }
        } else {
            const ApiJsonBody *json = result.JsonBody();
            response.set_content(json->value.dump(), "application/json");
        }
        if (!result.operationId.empty()) {
            response.set_header("X-Operation-Id", result.operationId);
        }
    }

    nlohmann::json ApiServer::BuildOperationsDocument(std::string_view groupPath) const
    {
        return BuildDirectoryDocument(
            operations.Snapshot(), groupPath, std::string(ApiPrefix) + "/operations", DirectoryKind::Operations);
    }

    nlohmann::json ApiServer::BuildSkillsDocument(std::string_view groupPath) const
    {
        return BuildDirectoryDocument(
            operations.Snapshot(), groupPath, std::string(ApiPrefix) + "/skills", DirectoryKind::Skills);
    }

    nlohmann::json ApiServer::BuildOperationDocument(const OperationEntry &entry) const
    {
        nlohmann::json document = {
            {"operationId", entry.name},
            {"title", entry.title},
            {"description", entry.description},
            {"method", ApiHttpMethodName(entry.method)},
            {"path", entry.path},
            {"pathParameters", ParameterList(entry, true)},
            {"queryParameters", entry.method == ApiHttpMethod::Get ? ParameterList(entry, false)
                                                                   : nlohmann::json::array()},
            {"readOnly", HasFlag(entry.flags, OperationFlags::ReadOnly)},
            {"destructive", HasFlag(entry.flags, OperationFlags::Destructive)},
            {"longRunning", HasFlag(entry.flags, OperationFlags::LongRunning)},
            {"schema", OperationSchemaPath(entry)},
            {"skill", OperationSkillPath(entry)}};
        if (entry.responseSchema) {
            document["responseSchema"] = OperationResponseSchemaPath(entry);
        }
        if (entry.method != ApiHttpMethod::Get) {
            document["requestBodySchema"] = NonPathArgumentSchema(entry);
        }
        return document;
    }

    nlohmann::json ApiServer::BuildOperationSchema(const OperationEntry &entry, std::string_view idReference) const
    {
        nlohmann::json schema = entry.inputSchema;
        schema["$schema"]     = "https://json-schema.org/draft/2020-12/schema";
        schema["$id"]         = idReference;
        schema["title"]       = entry.title;
        schema["description"] = entry.description;
        return schema;
    }

    nlohmann::json ApiServer::BuildOperationResponseSchema(
        const OperationEntry &entry,
        std::string_view idReference) const
    {
        nlohmann::json schema = entry.responseSchema.value_or(nlohmann::json::object());
        schema["$schema"]     = "https://json-schema.org/draft/2020-12/schema";
        schema["$id"]         = idReference;
        schema["title"]       = entry.title + " response";
        schema["description"] = "Response returned by " + entry.name + ".";
        return schema;
    }

    std::string ApiServer::BuildOperationSkill(const OperationEntry &entry) const
    {
        const std::string method(ApiHttpMethodName(entry.method));
        const std::vector<std::string> pathNames = PathParameterNames(entry.path);
        const nlohmann::json argumentSchema      = NonPathArgumentSchema(entry);

        std::ostringstream skill;
        skill << "---\n"
              << "name: " << SkillName(entry.name) << "\n"
              << "description: " << nlohmann::json(entry.description).dump() << "\n"
              << "---\n\n"
              << "# " << entry.title << "\n\n"
              << entry.description << "\n\n"
              << "Call `" << method << " " << entry.path << "`.\n\n";

        if (!pathNames.empty()) {
            skill << "Supply these values in the URL path:\n\n";
            for (const std::string &name : pathNames) {
                const nlohmann::json *schema = FindPropertySchema(entry.inputSchema, name);
                skill << "- `" << name << "`";
                if (schema != nullptr) {
                    skill << ": " << schema->dump();
                }
                skill << '\n';
            }
            skill << '\n';
        }
        if (entry.method == ApiHttpMethod::Get) {
            skill << "Send the remaining values as URL query parameters.\n\n";
        } else {
            skill << "Send the remaining values as a JSON object with `Content-Type: application/json`.\n\n";
        }

        skill << "Resolve route paths with the public base URL from the root API skill, preserving any proxy path prefix.\n\n"
              << "The composed argument schema is available at `GET " << OperationSchemaPath(entry) << "`.\n\n"
              << "```json\n"
              << argumentSchema.dump(2)
              << "\n```\n\n";
        if (entry.responseSchema) {
            skill << "The response schema is available at `GET " << OperationResponseSchemaPath(entry)
                  << "`.\n\n";
        }
        skill << "The operation details are available at `GET " << OperationDocumentPath(entry) << "`.\n";
        return skill.str();
    }

    nlohmann::json ApiServer::BuildOpenApiDocument() const
    {
        const std::vector<OperationEntry> entries    = operations.Snapshot();
        nlohmann::json document                      = nlohmann::json::object();
        document["openapi"]                          = "3.1.1";
        document["info"]                             = {{"title", "TerraForge3D API"}, {"version", TERR3D_VERSION_STRING}};
        document["servers"]                          = nlohmann::json::array({{{"url", "../.."}}});
        document["paths"]                            = nlohmann::json::object();
        document["components"]["schemas"]["Problem"] = ProblemSchema();
        const nlohmann::json stringSchema            = {{"type", "string"}};
        const nlohmann::json booleanSchema           = {{"type", "boolean"}};

        nlohmann::json directoryChildSchema = nlohmann::json::object();
        directoryChildSchema["type"]        = "object";
        directoryChildSchema["properties"]  = {
            {"name", stringSchema},
            {"path", stringSchema}};
        directoryChildSchema["required"] = nlohmann::json::array({"name", "path"});

        nlohmann::json parameterSchema = nlohmann::json::object();
        parameterSchema["type"]        = "object";
        parameterSchema["properties"]  = {
            {"name", stringSchema},
            {"required", booleanSchema},
            {"schema", {{"type", "object"}}}};
        parameterSchema["required"] = nlohmann::json::array({"name", "required", "schema"});

        nlohmann::json summarySchema = nlohmann::json::object();
        summarySchema["type"]        = "object";
        summarySchema["properties"]  = {
            {"name", stringSchema},
            {"operationId", stringSchema},
            {"title", stringSchema},
            {"description", stringSchema},
            {"method", {{"type", "string"}, {"enum", {"GET", "POST", "PATCH"}}}},
            {"path", stringSchema},
            {"readOnly", booleanSchema},
            {"destructive", booleanSchema},
            {"longRunning", booleanSchema},
            {"document", stringSchema},
            {"operation", stringSchema},
            {"schema", stringSchema},
            {"responseSchema", stringSchema},
            {"skill", stringSchema}};

        nlohmann::json directorySchema = nlohmann::json::object();
        directorySchema["type"]        = "object";
        directorySchema["properties"]  = {
            {"path", stringSchema},
            {"children", {{"type", "array"}, {"items", directoryChildSchema}}},
            {"operations", {{"type", "array"}, {"items", summarySchema}}},
            {"skills", {{"type", "array"}, {"items", summarySchema}}}};
        directorySchema["required"]                             = nlohmann::json::array({"path", "children"});
        document["components"]["schemas"]["OperationDirectory"] = std::move(directorySchema);

        nlohmann::json operationDocumentSchema = nlohmann::json::object();
        operationDocumentSchema["type"]        = "object";
        operationDocumentSchema["properties"]  = {
            {"operationId", stringSchema},
            {"title", stringSchema},
            {"description", stringSchema},
            {"method", {{"type", "string"}, {"enum", {"GET", "POST", "PATCH"}}}},
            {"path", stringSchema},
            {"pathParameters", {{"type", "array"}, {"items", parameterSchema}}},
            {"queryParameters", {{"type", "array"}, {"items", parameterSchema}}},
            {"readOnly", booleanSchema},
            {"destructive", booleanSchema},
            {"longRunning", booleanSchema},
            {"schema", stringSchema},
            {"skill", stringSchema},
            {"responseSchema", stringSchema},
            {"requestBodySchema", {{"type", "object"}}}};
        operationDocumentSchema["required"]                    = nlohmann::json::array({"operationId", "title", "description", "method", "path", "pathParameters", "queryParameters",
                                                                                        "readOnly", "destructive", "longRunning", "schema", "skill"});
        document["components"]["schemas"]["OperationDocument"] = std::move(operationDocumentSchema);

        for (const OperationEntry &entry : entries) {
            nlohmann::json operation = nlohmann::json::object();
            operation["operationId"] = entry.name;
            operation["summary"]     = entry.title;
            operation["description"] = entry.description;
            operation["tags"]        = nlohmann::json::array({ResourceGroupPath(entry)});

            const std::vector<std::string> pathNames = PathParameterNames(entry.path);
            const auto properties                    = entry.inputSchema.find("properties");
            const auto required                      = entry.inputSchema.find("required");
            operation["parameters"]                  = nlohmann::json::array();
            for (const std::string &parameterName : pathNames) {
                const nlohmann::json *schema = FindPropertySchema(entry.inputSchema, parameterName);
                if (schema == nullptr) {
                    continue;
                }
                operation["parameters"].push_back({{"name", parameterName},
                                                   {"in", "path"},
                                                   {"required", true},
                                                   {"schema", *schema}});
            }
            if (entry.method == ApiHttpMethod::Get && properties != entry.inputSchema.end() &&
                properties->is_object()) {
                for (auto property = properties->begin(); property != properties->end(); ++property) {
                    if (std::find(pathNames.begin(), pathNames.end(), property.key()) != pathNames.end()) {
                        continue;
                    }
                    const bool isRequired = required != entry.inputSchema.end() && required->is_array() &&
                                            std::any_of(required->begin(), required->end(), [&property](const nlohmann::json &field) {
                                                return field.is_string() && field.get<std::string>() == property.key();
                                            });
                    operation["parameters"].push_back({{"name", property.key()},
                                                       {"in", "query"},
                                                       {"required", isRequired},
                                                       {"schema", property.value()}});
                }
            }

            if (entry.method != ApiHttpMethod::Get) {
                operation["requestBody"]["required"] = true;
                operation["requestBody"]["content"]["application/json"]["schema"] =
                    NonPathArgumentSchema(entry);
            }

            operation["responses"]["200"]["description"] = "Operation completed.";
            if (entry.name == "tf3d.viewport.capture") {
                operation["responses"]["200"]["content"]["image/jpeg"]["schema"] = {
                    {"type", "string"}, {"format", "binary"}};
            } else if (entry.responseSchema) {
                operation["responses"]["200"]["content"]["application/json"]["schema"] =
                    *entry.responseSchema;
            } else {
                operation["responses"]["200"]["content"]["application/json"]["schema"] = {{"type", "object"}};
            }
            if (entry.name == StatusOperation) {
                operation["responses"]["200"]["content"]["application/json"]["schema"] = {
                    {"type", "object"},
                    {"properties", {{"applicationAttached", {{"type", "boolean"}}}, {"version", {{"type", "string"}}}, {"apiVersion", {{"type", "string"}}}, {"transport", {{"type", "string"}}}}}};
            }
            AddProblemResponse(operation, 400, "Invalid request.");
            AddProblemResponse(operation, 404, "The requested resource was not found.");
            AddProblemResponse(operation, 500, "Operation failed.");
            AddProblemResponse(operation, 503, "The application is unavailable.");
            AddProblemResponse(operation, 504, "The operation timed out.");

            document["paths"][entry.path][LowercaseMethod(entry.method)] = std::move(operation);

            nlohmann::json getOperationDocument = {
                {"operationId", "getOperation_" + entry.name},
                {"summary", "Get details for " + entry.title},
                {"responses", {{"200", {{"description", "Operation details."}, {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/OperationDocument"}}}}}}}}}}}};
            document["paths"][OperationDocumentPath(entry)]["get"] = std::move(getOperationDocument);

            nlohmann::json getSchema = {
                {"operationId", "getSchema_" + entry.name},
                {"summary", "Get the composed argument schema for " + entry.title},
                {"responses", {{"200", {{"description", "Composed JSON Schema."}, {"content", {{"application/schema+json", {{"schema", BuildOperationSchema(entry, OpenApiOperationSchemaReference(entry))}}}}}}}}}};
            document["paths"][OperationSchemaPath(entry)]["get"] = std::move(getSchema);

            if (entry.responseSchema) {
                nlohmann::json getResponseSchema = {
                    {"operationId", "getResponseSchema_" + entry.name},
                    {"summary", "Get the response schema for " + entry.title},
                    {"responses", {{"200", {{"description", "Composed JSON Schema."}, {"content", {{"application/schema+json", {{"schema", BuildOperationResponseSchema(entry, OpenApiOperationResponseSchemaReference(entry))}}}}}}}}}};
                document["paths"][OperationResponseSchemaPath(entry)]["get"] = std::move(getResponseSchema);
            }

            nlohmann::json getSkill = {
                {"operationId", "getSkill_" + entry.name},
                {"summary", "Get the generated skill for " + entry.title},
                {"responses", {{"200", {{"description", "Operation skill instructions."}, {"content", {{"text/markdown", {{"schema", {{"type", "string"}}}}}}}}}}}};
            document["paths"][OperationSkillPath(entry)]["get"] = std::move(getSkill);
        }

        const auto addDirectoryRoute = [&document](const std::string &path,
                                                   const std::string &operationId,
                                                   const std::string &summary) {
            document["paths"][path]["get"] = {
                {"operationId", operationId},
                {"summary", summary},
                {"responses", {{"200", {{"description", summary}, {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/OperationDirectory"}}}}}}}}}}}};
        };

        addDirectoryRoute("/api/v1/operations", "listOperationGroups", "List top-level operation groups");
        addDirectoryRoute("/api/v1/skills", "listSkillGroups", "List top-level skill groups");
        std::set<std::string> groupPaths;
        for (const OperationEntry &entry : entries) {
            std::string groupPath = ResourceGroupPath(entry);
            while (!groupPath.empty()) {
                groupPaths.insert(groupPath);
                const std::size_t separator = groupPath.rfind('/');
                if (separator == std::string::npos) {
                    break;
                }
                groupPath.resize(separator);
            }
        }
        for (const std::string &groupPath : groupPaths) {
            addDirectoryRoute("/api/v1/operations/" + groupPath,
                              "listOperations_" + groupPath,
                              "List operations in " + groupPath);
            addDirectoryRoute("/api/v1/skills/" + groupPath,
                              "listSkills_" + groupPath,
                              "List skills in " + groupPath);
        }

        const auto addJsonGet = [&document](const std::string &path,
                                            const std::string &operationId,
                                            const std::string &summary) {
            document["paths"][path]["get"] = {
                {"operationId", operationId},
                {"summary", summary},
                {"responses", {{"200", {{"description", summary}, {"content", {{"application/json", {{"schema", {{"type", "object"}}}}}}}}}}}};
        };
        addJsonGet("/api/v1/openapi.json", "getOpenApiDocument", "Get the OpenAPI document");

        document["paths"]["/api/skill.md"]["get"] = {
            {"operationId", "getApiSkill"},
            {"summary", "Get API usage instructions"},
            {"responses", {{"200", {{"description", "API usage instructions."}, {"content", {{"text/markdown", {{"schema", {{"type", "string"}}}}}}}}}}}};
        document["paths"]["/api/docs"]["get"] = {
            {"operationId", "getApiPlayground"},
            {"summary", "Open the API playground"},
            {"responses", {{"200", {{"description", "Bundled API playground page."}, {"content", {{"text/html", {{"schema", {{"type", "string"}}}}}}}}}}}};

        return document;
    }

    void ApiServer::Update()
    {
        requestQueue.Drain();
    }

    void ApiServer::Stop()
    {
        requestQueue.Shutdown();
        if (server) {
            server->stop();
        }
        if (serverThread.joinable()) {
            serverThread.join();
        }
        server.reset();
        CloseCallHistory();
    }

    bool ApiServer::IsRunning() const
    {
        return server != nullptr && server->is_running();
    }

    ApiServerStats ApiServer::GetStats() const
    {
        ApiServerStats snapshot;
        snapshot.running        = IsRunning();
        snapshot.host           = host;
        snapshot.port           = port;
        snapshot.endpoint       = endpoint;
        snapshot.requestCount   = requestCount.load(std::memory_order_relaxed);
        snapshot.activeRequests = activeRequests.load(std::memory_order_relaxed);
        std::lock_guard<std::mutex> lock(statsMutex);
        snapshot.requestLog = requestLog;
        return snapshot;
    }

    void ApiServer::ClearRequestLog()
    {
        std::lock_guard<std::mutex> lock(statsMutex);
        requestLog.clear();
    }

    void ApiServer::OpenCallHistory()
    {
        if (logsDirectory.empty()) {
            return;
        }
        try {
            const std::filesystem::path logPath(logsDirectory);
            const std::filesystem::path callsDirectory = logPath.parent_path() / "ApiCalls";
            std::filesystem::create_directories(callsDirectory);
            const auto now     = std::chrono::system_clock::now();
            const auto nowTime = std::chrono::system_clock::to_time_t(now);
            std::tm localTime{};
#ifdef _WIN32
            localtime_s(&localTime, &nowTime);
#else
            localtime_r(&nowTime, &localTime);
#endif
            std::ostringstream filename;
            filename << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S") << ".jsonl";
            callHistoryPath = callsDirectory / filename.str();
            callHistory.open(callHistoryPath, std::ios::out | std::ios::app);
            if (!callHistory.is_open()) {
                TF3D_LOG_ERROR("Could not open API request history file '{}'", callHistoryPath.string());
                return;
            }
            callHistory << nlohmann::json{{"type", "session"},
                                          {"startedAt", FormatTimestamp()},
                                          {"application", "TerraForge3D"},
                                          {"version", TERR3D_VERSION_STRING},
                                          {"apiVersion", "v1"}}
                               .dump()
                        << '\n';
            callHistory.flush();
            TF3D_LOG_INFO("API request history file: {}", callHistoryPath.string());
        } catch (const std::exception &exception) {
            TF3D_LOG_ERROR("Could not initialize API request history: {}", exception.what());
        }
    }

    void ApiServer::CloseCallHistory()
    {
        std::lock_guard<std::mutex> lock(statsMutex);
        if (callHistory.is_open()) {
            callHistory.flush();
            callHistory.close();
        }
    }

    void ApiServer::RecordRequest(const httplib::Request &request, const httplib::Response &response)
    {
        std::string preview = request.body.substr(0, MaxRequestPreviewLength);
        if (request.body.size() > MaxRequestPreviewLength) {
            preview += "...";
        }
        ApiRequestLogEntry entry{
            FormatTimestamp(), request.method, request.path, response.status, std::move(preview)};
        const nlohmann::json history = {
            {"type", "request"},
            {"timestamp", entry.timestamp},
            {"method", entry.method},
            {"path", entry.path},
            {"status", entry.status},
            {"body", entry.bodyPreview}};

        requestCount.fetch_add(1, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lock(statsMutex);
        requestLog.push_back(std::move(entry));
        if (requestLog.size() > MaxRequestLogEntries) {
            requestLog.erase(requestLog.begin());
        }
        if (callHistory.is_open()) {
            callHistory << history.dump() << '\n';
            callHistory.flush();
        }
    }

} // namespace tf3d::api
