#include "MCP/McpTransport.h"

#include "mcp_resource.h"
#include "mcp_server.h"
#include "mcp_tool.h"

#include "MCP/ActionRegistry.h"
#include "MCP/MainThreadRequestQueue.h"
#include "MCP/ResourceRegistry.h"

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    nlohmann::json CreateResourceContent(
        const ResourceEntry &entry,
        const McpResult &result)
    {
        if (!result.ok)
            throw std::runtime_error(result.ToErrorMessage());

        nlohmann::json content = result.value;
        if (!content.is_object()) {
            content = {
                {"uri", entry.uriOrTemplate},
                {"mimeType", entry.mimeType},
                {"text", content.dump()}};
        }
        if (!content.contains("uri"))
            content["uri"] = entry.uriOrTemplate;
        if (!content.contains("mimeType"))
            content["mimeType"] = entry.mimeType;
        return content;
    }

    class RegistryResource final : public mcp::resource
    {
    public:
        RegistryResource(
            ResourceEntry entry,
            MainThreadRequestQueue *requestQueue,
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
                {"mimeType", entry.mimeType}};
        }

        mcp::json read() const override
        {
            if (onRead)
                onRead();
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
        MainThreadRequestQueue *requestQueue;
        std::function<void()> onRead;
    };
} 

void RegisterMcpTransport(
    mcp::server &server,
    ActionRegistry &actions,
    ResourceRegistry &resources,
    MainThreadRequestQueue &requestQueue,
    McpCommandRecorder recordCommand)
{
    nlohmann::json capabilities = nlohmann::json::object();
    const auto actionEntries    = actions.Snapshot();
    const auto resourceEntries  = resources.Snapshot();
    if (!actionEntries.empty())
        capabilities["tools"] = nlohmann::json::object();
    if (!resourceEntries.empty())
        capabilities["resources"] = nlohmann::json::object();
    server.set_capabilities(capabilities);

    for (const auto &entry : actionEntries) {
        mcp::tool tool{
            entry.name,
            entry.description,
            entry.inputSchema,
            entry.annotations};

        server.register_tool(tool, [&actions, &requestQueue, recordCommand, name = entry.name](
                                       const nlohmann::json &arguments,
                                       const std::string &sessionId) {
            if (recordCommand) {
                recordCommand(
                    "tools/call",
                    nlohmann::json{{"name", name}, {"arguments", arguments}}.dump(),
                    sessionId);
            }

            const auto registeredAction = actions.Find(name);
            if (!registeredAction)
                throw std::runtime_error("MCP action is no longer registered: " + name);

            const McpResult result = requestQueue.Execute([registeredAction = *registeredAction,
                                                           arguments]() {
                return registeredAction.invoke(arguments);
            });
            if (!result.ok)
                throw std::runtime_error(result.ToErrorMessage());
            return result.ToToolContent();
        });
    }

    for (const auto &entry : resourceEntries) {
        if (!entry.isTemplate) {
            server.register_resource(
                entry.uriOrTemplate,
                std::make_shared<RegistryResource>(
                    entry,
                    &requestQueue,
                    [recordCommand, uri = entry.uriOrTemplate]() {
                        if (recordCommand)
                            recordCommand(
                                "resources/read",
                                nlohmann::json{{"uri", uri}}.dump(),
                                "");
                    }));
            continue;
        }

        server.register_resource_template(
            entry.uriOrTemplate,
            entry.name,
            entry.mimeType,
            entry.description,
            [&requestQueue, recordCommand, entry](
                const std::string &uri,
                const std::map<std::string, std::string> &parameters,
                const std::string &sessionId) {
                if (recordCommand)
                    recordCommand(
                        "resources/read",
                        nlohmann::json{{"uri", uri}}.dump(),
                        sessionId);
                const McpResult result = requestQueue.Execute([entry, uri, parameters]() {
                    return entry.read({uri, parameters});
                });
                return CreateResourceContent(entry, result);
            });
    }
}
