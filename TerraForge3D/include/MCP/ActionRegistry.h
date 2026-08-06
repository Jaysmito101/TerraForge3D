#pragma once

#include "MCP/McpResult.h"

#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace tf3d::mcp_layer
{

    using McpActionHandler = std::function<McpResult(const nlohmann::json &)>;

    enum class ActionFlags : uint32_t {
        None        = 0,
        ReadOnly    = 1u << 0,
        Destructive = 1u << 1,
        LongRunning = 1u << 2
    };

    struct ActionEntry {
        std::string name;
        std::string title;
        std::string description;
        nlohmann::json inputSchema = nlohmann::json{
            {"type", "object"},
            {"properties", nlohmann::json::object()}};
        nlohmann::json annotations = nlohmann::json::object();
        ActionFlags flags          = ActionFlags::None;
        McpActionHandler invoke;
    };

    std::optional<ActionEntry> CreateActionEntryFromJson(
        const nlohmann::json &definition,
        McpActionHandler invoke);

    void RegisterActionFromJson(
        class ActionRegistry &actions,
        const std::optional<nlohmann::json> &definition,
        McpActionHandler invoke);

    class ActionRegistry
    {
    public:
        bool Register(ActionEntry entry);
        bool Unregister(const std::string &name);
        std::optional<ActionEntry> Find(const std::string &name) const;
        std::vector<ActionEntry> Snapshot() const;
        void Clear();

    private:
        mutable std::mutex mutex;
        std::map<std::string, ActionEntry> entries;
    };

} // namespace tf3d::mcp_layer
