#include "MCP/ActionRegistry.h"

#include "Base/Logging/Logger.h"

namespace
{
    using tf3d::mcp_layer::ActionFlags;

    bool ReadActionString(
        const nlohmann::json &definition,
        const char *field,
        std::string &value)
    {
        if (!definition.contains(field) || !definition.at(field).is_string()) {
            TF3D_LOG_ERROR("MCP action definition field '{}' must be a string", field);
            return false;
        }

        value = definition.at(field).get<std::string>();
        if (value.empty()) {
            TF3D_LOG_ERROR("MCP action definition field '{}' cannot be empty", field);
            return false;
        }
        return true;
    }

    bool ParseActionFlags(const nlohmann::json &definition, ActionFlags &flags)
    {
        flags = ActionFlags::None;
        if (!definition.contains("Flags"))
            return true;

        const auto &values = definition.at("Flags");
        if (!values.is_array()) {
            TF3D_LOG_ERROR("MCP action definition field 'Flags' must be an array");
            return false;
        }

        for (const auto &value : values) {
            if (!value.is_string()) {
                TF3D_LOG_ERROR("MCP action definition 'Flags' entries must be strings");
                return false;
            }

            const std::string flag = value.get<std::string>();
            if (flag == "None") {
                continue;
            } else if (flag == "ReadOnly") {
                flags = static_cast<ActionFlags>(
                    static_cast<uint32_t>(flags) |
                    static_cast<uint32_t>(ActionFlags::ReadOnly));
            } else if (flag == "Destructive") {
                flags = static_cast<ActionFlags>(
                    static_cast<uint32_t>(flags) |
                    static_cast<uint32_t>(ActionFlags::Destructive));
            } else if (flag == "LongRunning") {
                flags = static_cast<ActionFlags>(
                    static_cast<uint32_t>(flags) |
                    static_cast<uint32_t>(ActionFlags::LongRunning));
            } else {
                TF3D_LOG_ERROR("MCP action definition has unknown flag '{}'", flag);
                return false;
            }
        }
        return true;
    }
} // namespace

namespace tf3d::mcp_layer
{

    std::optional<ActionEntry> CreateActionEntryFromJson(
        const nlohmann::json &definition,
        McpActionHandler invoke)
    {
        if (!definition.is_object()) {
            TF3D_LOG_ERROR("MCP action definition must be a JSON object");
            return std::nullopt;
        }
        if (!invoke) {
            TF3D_LOG_ERROR("MCP action definition has no invocation handler");
            return std::nullopt;
        }

        ActionEntry entry;
        if (!ReadActionString(definition, "Name", entry.name) ||
            !ReadActionString(definition, "Title", entry.title) ||
            !ReadActionString(definition, "Description", entry.description)) {
            return std::nullopt;
        }

        if (!definition.contains("InputSchema") || !definition.at("InputSchema").is_object()) {
            TF3D_LOG_ERROR("MCP action definition field 'InputSchema' must be a JSON object");
            return std::nullopt;
        }
        entry.inputSchema = definition.at("InputSchema");

        if (definition.contains("Annotations")) {
            if (!definition.at("Annotations").is_object()) {
                TF3D_LOG_ERROR("MCP action definition field 'Annotations' must be a JSON object");
                return std::nullopt;
            }
            entry.annotations = definition.at("Annotations");
        }

        if (!ParseActionFlags(definition, entry.flags))
            return std::nullopt;

        entry.invoke = std::move(invoke);
        return entry;
    }

    void RegisterActionFromJson(
        ActionRegistry &actions,
        const std::optional<nlohmann::json> &definition,
        McpActionHandler invoke)
    {
        if (!definition)
            return;

        auto entry = CreateActionEntryFromJson(*definition, std::move(invoke));
        if (!entry)
            return;

        const std::string operationId = entry->name;
        McpActionHandler handler      = std::move(entry->invoke);
        entry->invoke = [operationId, handler = std::move(handler)](
                            const nlohmann::json &arguments) mutable {
            McpResult result = handler(arguments);
            if (result.operationId.empty())
                result.operationId = operationId;
            return result;
        };

        if (actions.Register(std::move(*entry)))
            return;

        TF3D_LOG_ERROR("Could not register MCP action '{}'", operationId);
    }

    bool ActionRegistry::Register(ActionEntry entry)
    {
        if (entry.name.empty() || !entry.invoke)
            return false;

        std::lock_guard<std::mutex> lock(mutex);
        std::string name = entry.name;
        return entries.emplace(std::move(name), std::move(entry)).second;
    }

    bool ActionRegistry::Unregister(const std::string &name)
    {
        std::lock_guard<std::mutex> lock(mutex);
        return entries.erase(name) != 0;
    }

    std::optional<ActionEntry> ActionRegistry::Find(const std::string &name) const
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto found = entries.find(name);
        if (found == entries.end())
            return std::nullopt;
        return found->second;
    }

    std::vector<ActionEntry> ActionRegistry::Snapshot() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<ActionEntry> snapshot;
        snapshot.reserve(entries.size());
        for (const auto &[name, entry] : entries)
            snapshot.push_back(entry);
        return snapshot;
    }

    void ActionRegistry::Clear()
    {
        std::lock_guard<std::mutex> lock(mutex);
        entries.clear();
    }

} // namespace tf3d::mcp_layer
