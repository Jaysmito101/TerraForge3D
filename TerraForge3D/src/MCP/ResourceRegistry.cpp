#include "MCP/ResourceRegistry.h"

bool ResourceRegistry::Register(ResourceEntry entry)
{
    if (entry.uriOrTemplate.empty() || !entry.read)
        return false;

    std::lock_guard<std::mutex> lock(mutex);
    std::string uriOrTemplate = entry.uriOrTemplate;
    return entries.emplace(std::move(uriOrTemplate), std::move(entry)).second;
}

bool ResourceRegistry::Unregister(const std::string &uriOrTemplate)
{
    std::lock_guard<std::mutex> lock(mutex);
    return entries.erase(uriOrTemplate) != 0;
}

std::vector<ResourceEntry> ResourceRegistry::Snapshot() const
{
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<ResourceEntry> snapshot;
    snapshot.reserve(entries.size());
    for (const auto &[uri, entry] : entries)
        snapshot.push_back(entry);
    return snapshot;
}

void ResourceRegistry::Clear()
{
    std::lock_guard<std::mutex> lock(mutex);
    entries.clear();
}
