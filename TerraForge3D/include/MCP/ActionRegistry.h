#pragma once

#include "MCP/McpResult.h"

#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

enum class ActionFlags : uint32_t
	{
		None = 0,
		ReadOnly = 1u << 0,
		Destructive = 1u << 1,
		LongRunning = 1u << 2
	};

	struct ActionEntry
	{
		std::string name;
		std::string title;
		std::string description;
		Json inputSchema = Json{
			{"type", "object"},
			{"properties", Json::object()}
		};
		Json annotations = Json::object();
		ActionFlags flags = ActionFlags::None;
		std::function<McpResult(const Json&)> invoke;
	};

	class ActionRegistry
	{
	public:
		bool Register(ActionEntry entry);
		bool Unregister(const std::string& name);
		std::optional<ActionEntry> Find(const std::string& name) const;
		std::vector<ActionEntry> Snapshot() const;
		void Clear();

	private:
		mutable std::mutex mutex;
		std::map<std::string, ActionEntry> entries;
	};
