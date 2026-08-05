#include "MCP/ActionRegistry.h"

bool ActionRegistry::Register(ActionEntry entry)
	{
		if (entry.name.empty() || !entry.invoke) return false;

		std::lock_guard<std::mutex> lock(mutex);
		std::string name = entry.name;
		return entries.emplace(std::move(name), std::move(entry)).second;
	}

	bool ActionRegistry::Unregister(const std::string& name)
	{
		std::lock_guard<std::mutex> lock(mutex);
		return entries.erase(name) != 0;
	}

	std::optional<ActionEntry> ActionRegistry::Find(const std::string& name) const
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto found = entries.find(name);
		if (found == entries.end()) return std::nullopt;
		return found->second;
	}

	std::vector<ActionEntry> ActionRegistry::Snapshot() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		std::vector<ActionEntry> snapshot;
		snapshot.reserve(entries.size());
		for (const auto& [name, entry] : entries) snapshot.push_back(entry);
		return snapshot;
	}

	void ActionRegistry::Clear()
	{
		std::lock_guard<std::mutex> lock(mutex);
		entries.clear();
	}
