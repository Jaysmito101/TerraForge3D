#include "mcp_server.h"
#include "mcp_resource.h"
#include "mcp_tool.h"

#include "MCP/ActionRegistry.h"
#include "MCP/MainThreadRequestQueue.h"
#include "MCP/ResourceRegistry.h"
#include "MCP/TerraForgeMcpServer.h"

#include "Data/VersionInfo.h"
#include "Base/Logging/Logger.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace
{
	int ReadMcpPort()
	{
		constexpr int defaultPort = 9823;
		const char* configuredPort = std::getenv("TF3D_MCP_PORT");
		if (configuredPort == nullptr || configuredPort[0] == '\0') return defaultPort;

		char* end = nullptr;
		const long parsedPort = std::strtol(configuredPort, &end, 10);
		if (end == configuredPort || *end != '\0' || parsedPort < 1 || parsedPort > 65535)
		{
			TF3D_LOG_WARN("Ignoring invalid TF3D_MCP_PORT='{}'; using {}", configuredPort, defaultPort);
			return defaultPort;
		}
		return static_cast<int>(parsedPort);
	}

	std::string FormatCommandTimestamp()
	{
		const auto now = std::chrono::system_clock::now();
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

	nlohmann::json CreateResourceContent(
		const ResourceEntry& entry,
		const McpResult& result)
	{
		if (!result.ok) throw std::runtime_error(result.ToErrorMessage());

		nlohmann::json content = result.value;
		if (!content.is_object())
		{
			content = {
				{"uri", entry.uriOrTemplate},
				{"mimeType", entry.mimeType},
				{"text", content.dump()}
			};
		}
		if (!content.contains("uri")) content["uri"] = entry.uriOrTemplate;
		if (!content.contains("mimeType")) content["mimeType"] = entry.mimeType;
		return content;
	}

	class RegistryResource final : public mcp::resource
	{
	public:
		RegistryResource(
			ResourceEntry entry,
			MainThreadRequestQueue* requestQueue,
			std::function<void()> onRead)
			: entry(std::move(entry)), requestQueue(requestQueue), onRead(std::move(onRead))
		{
		}

		mcp::json get_metadata() const override
		{
			return {
				{"uri", entry.uriOrTemplate},
				{"name", entry.name},
				{"description", entry.description},
				{"mimeType", entry.mimeType}
			};
		}

		mcp::json read() const override
		{
			if (onRead) onRead();
			const auto result = requestQueue->Execute([entry = entry]() {
				return entry.read({entry.uriOrTemplate, {}});
			});
			return CreateResourceContent(entry, result);
		}

		bool is_modified() const override
		{
			return false;
		}

		std::string get_uri() const override
		{
			return entry.uriOrTemplate;
		}

	private:
		ResourceEntry entry;
		MainThreadRequestQueue* requestQueue;
		std::function<void()> onRead;
	};
}

class TerraForgeMcpServer::Impl
	{
	public:
		Impl(ApplicationState* applicationState, std::string logsDirectory)
			: applicationState(applicationState), logsDirectory(std::move(logsDirectory)), port(ReadMcpPort())
		{
			endpoint = "http://" + host + ":" + std::to_string(port) + "/mcp";
			RegisterCoreEntries();
		}

		~Impl()
		{
			Stop();
		}

		bool Start()
		{
			if (server && server->is_running()) return true;
			// Keep cpp-mcp's internal diagnostics quiet; actionable MCP requests are
			// recorded through TerraForge3D's logger and command history below.
			mcp::set_log_level(mcp::log_level::error);

			mcp::server::configuration configuration;
			configuration.host = host;
			configuration.port = port;
			configuration.name = "TerraForge3D MCP";
			configuration.version = TERR3D_VERSION_STRING;
			configuration.mcp_endpoint = "/mcp";
			configuration.sse_endpoint = "/sse";
			configuration.msg_endpoint = "/message";

			server = std::make_unique<mcp::server>(configuration);
			server->set_server_info(configuration.name, configuration.version);
			server->set_instructions(
				"TerraForge3D exposes registered actions as MCP tools and readable state as MCP resources.");

			RegisterEntries();
			OpenCallHistory();
			if (!server->start(false))
			{
				server.reset();
				CloseCallHistory();
				return false;
			}

			TF3D_LOG_INFO("TerraForge3D MCP server listening on http://{}:{}{}",
				configuration.host, configuration.port, configuration.mcp_endpoint);
			return true;
		}

		void Update()
		{
			requestQueue.Drain();
		}

		void Stop()
		{
			requestQueue.Shutdown();
			if (server)
			{
				server->stop();
				server.reset();
			}
			CloseCallHistory();
		}

		bool IsRunning() const
		{
			return server != nullptr && server->is_running();
		}

		McpServerStats GetStats() const
		{
			McpServerStats snapshot;
			snapshot.running = IsRunning();
			snapshot.host = host;
			snapshot.port = port;
			snapshot.endpoint = endpoint;
			if (server) snapshot.activeSessions = server->get_active_sessions().size();

			std::lock_guard<std::mutex> lock(statsMutex);
			snapshot.commandCount = commandCount;
			snapshot.commandLog = commandLog;
			return snapshot;
		}

		void ClearCommandLog()
		{
			std::lock_guard<std::mutex> lock(statsMutex);
			commandCount = 0;
			commandLog.clear();
		}

	private:
		void OpenCallHistory()
		{
			if (logsDirectory.empty()) return;

			try
			{
				const std::filesystem::path logPath(logsDirectory);
				const std::filesystem::path callsDirectory = logPath.parent_path() / "McpCalls";
				std::filesystem::create_directories(callsDirectory);

				const auto now = std::chrono::system_clock::now();
				const auto nowTime = std::chrono::system_clock::to_time_t(now);
				const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
					now.time_since_epoch()).count() % 1000;
				std::tm localTime{};
#ifdef _WIN32
				localtime_s(&localTime, &nowTime);
#else
				localtime_r(&nowTime, &localTime);
#endif
				std::ostringstream filename;
				filename << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S")
					<< "_" << std::setfill('0') << std::setw(3) << milliseconds << ".jsonl";

				callHistoryPath = callsDirectory / filename.str();
				callHistory.open(callHistoryPath, std::ios::out | std::ios::app);
				if (!callHistory.is_open())
				{
					TF3D_LOG_ERROR("Could not open MCP call history file '{}'", callHistoryPath.string());
					return;
				}

				const nlohmann::json sessionHeader = {
					{"type", "session"},
					{"startedAt", FormatCommandTimestamp()},
					{"application", "TerraForge3D"},
					{"version", TERR3D_VERSION_STRING},
					{"protocolVersion", mcp::MCP_VERSION}
				};
				callHistory << sessionHeader.dump() << '\n';
				callHistory.flush();
				TF3D_LOG_INFO("MCP call history file: {}", callHistoryPath.string());
			}
			catch (const std::exception& exception)
			{
				TF3D_LOG_ERROR("Could not initialize MCP call history: {}", exception.what());
			}
		}

		void CloseCallHistory()
		{
			if (callHistory.is_open())
			{
				callHistory.flush();
				callHistory.close();
			}
		}

		void RecordCommand(
			const std::string& method,
			const nlohmann::json& params,
			const std::string& sessionId)
		{
			std::string parameters;
			std::string requestJson;
			try
			{
				parameters = params.dump();
				nlohmann::json request = {
					{"jsonrpc", "2.0"},
					{"method", method}
				};
				if (!params.empty()) request["params"] = params;
				requestJson = request.dump(2);
			}
			catch (...)
			{
				parameters = "<unserializable parameters>";
				requestJson = parameters;
			}

			nlohmann::json historyEntry = {
				{"type", "request"},
				{"timestamp", FormatCommandTimestamp()},
				{"sessionId", sessionId},
				{"method", method}
			};
			try
			{
			historyEntry["request"] = nlohmann::json::parse(requestJson);
			}
			catch (...)
			{
				historyEntry["request"] = requestJson;
			}

			std::lock_guard<std::mutex> lock(statsMutex);
			++commandCount;
			commandLog.push_back({
				FormatCommandTimestamp(),
				method,
				std::move(parameters),
				std::move(requestJson)
			});
			TF3D_LOG_INFO("[MCP] {} {}", method, commandLog.back().parameters);
			if (callHistory.is_open())
			{
				callHistory << historyEntry.dump() << '\n';
				callHistory.flush();
			}
		}

		void RegisterCoreEntries()
		{
			actions.Register({
				"tf3d.mcp.status",
				"MCP status",
				"Return the TerraForge3D MCP bridge status and protocol revision.",
				nlohmann::json	{
					{"type", "object"},
					{"properties", nlohmann::json::object()}
				},
				nlohmann::json{{"readOnlyHint", true}},
				ActionFlags::ReadOnly,
				[this](const nlohmann::json&) {
					return McpResult::Success({
						{"applicationAttached", applicationState != nullptr},
						{"version", TERR3D_VERSION_STRING},
						{"protocolVersion", mcp::MCP_VERSION},
						{"transport", "streamable-http"}
					});
				}
			});

			resources.Register({
				"terraforge://mcp/status",
				"MCP status",
				"Current TerraForge3D MCP bridge status.",
				"application/json",
				false,
				[this](const ResourceRequest& request) {
					const nlohmann::json status = {
						{"applicationAttached", applicationState != nullptr},
						{"version", TERR3D_VERSION_STRING},
						{"protocolVersion", mcp::MCP_VERSION},
						{"transport", "streamable-http"}
					};
					return McpResult::Success({
						{"uri", request.uri},
						{"mimeType", "application/json"},
						{"text", status.dump()}
					});
				}
			});
		}

		void RegisterEntries()
		{
			nlohmann::json capabilities = nlohmann::json::object();
			const auto actionEntries = actions.Snapshot();
			const auto resourceEntries = resources.Snapshot();
			if (!actionEntries.empty()) capabilities["tools"] = nlohmann::json::object();
			if (!resourceEntries.empty()) capabilities["resources"] = nlohmann::json::object();
			server->set_capabilities(capabilities);

			for (const auto& entry : actionEntries)
			{
				mcp::tool tool{
					entry.name,
					entry.description,
					entry.inputSchema,
					entry.annotations
				};

				server->register_tool(tool, [this, name = entry.name](const nlohmann::json& arguments, const std::string& sessionId) {
					RecordCommand(
						"tools/call",
						nlohmann::json{
							{"name", name},
							{"arguments", nlohmann::json::parse(arguments.dump())}
						},
						sessionId);
					const auto registeredAction = actions.Find(name);
					if (!registeredAction)
					{
						throw std::runtime_error("MCP action is no longer registered: " + name);
					}

					const McpResult result = requestQueue.Execute([
						registeredAction = *registeredAction,
						arguments]() {
						return registeredAction.invoke(arguments);
					});
					if (!result.ok) throw std::runtime_error(result.ToErrorMessage());
					return result.ToToolContent();
				});
			}

			for (const auto& entry : resourceEntries)
			{
				if (!entry.isTemplate)
				{
					server->register_resource(
						entry.uriOrTemplate,
						std::make_shared<RegistryResource>(
							entry,
							&requestQueue,
							[this, uri = entry.uriOrTemplate]() {
								RecordCommand(
									"resources/read",
									nlohmann::json{{"uri", uri}},
									"");
							}));
					continue;
				}

				server->register_resource_template(
					entry.uriOrTemplate,
					entry.name,
					entry.mimeType,
					entry.description,
					[this, entry](
						const std::string& uri,
						const std::map<std::string, std::string>& parameters,
						const std::string& sessionId) {
						RecordCommand(
							"resources/read",
							nlohmann::json{{"uri", uri}},
							sessionId);
						const McpResult result = requestQueue.Execute([
							entry,
							uri,
							parameters]() {
							return entry.read({uri, parameters});
						});
						return CreateResourceContent(entry, result);
					});
			}
		}

		ApplicationState* applicationState;
		std::string logsDirectory;
		const std::string host = "127.0.0.1";
		int port;
		std::string endpoint;
		ActionRegistry actions;
		ResourceRegistry resources;
		MainThreadRequestQueue requestQueue;
		std::unique_ptr<mcp::server> server;
		std::ofstream callHistory;
		std::filesystem::path callHistoryPath;
		mutable std::mutex statsMutex;
		std::uint64_t commandCount = 0;
		std::vector<McpCommandLogEntry> commandLog;
	};

TerraForgeMcpServer::TerraForgeMcpServer(ApplicationState* applicationState, std::string logsDirectory)
		: implementation(std::make_unique<Impl>(applicationState, std::move(logsDirectory)))
	{
	}

	TerraForgeMcpServer::~TerraForgeMcpServer() = default;

	bool TerraForgeMcpServer::Start()
	{
		return implementation->Start();
	}

	void TerraForgeMcpServer::Update()
	{
		implementation->Update();
	}

	void TerraForgeMcpServer::Stop()
	{
		implementation->Stop();
	}

	bool TerraForgeMcpServer::IsRunning() const
	{
		return implementation->IsRunning();
	}

	McpServerStats TerraForgeMcpServer::GetStats() const
	{
		return implementation->GetStats();
	}

	void TerraForgeMcpServer::ClearCommandLog()
	{
		implementation->ClearCommandLog();
	}
