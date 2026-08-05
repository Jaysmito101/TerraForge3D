#pragma once

#include "MCP/McpResult.h"

#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

struct ResourceRequest {
    std::string uri;
    std::map<std::string, std::string> parameters;
};

using ResourceReader = std::function<McpResult(const ResourceRequest &)>;

struct ResourceEntry {
    std::string uriOrTemplate;
    std::string name;
    std::string description;
    std::string mimeType = "application/json";
    bool isTemplate      = false;
    ResourceReader read;
};

class ResourceRegistry
{
public:
    bool Register(ResourceEntry entry);
    bool Unregister(const std::string &uriOrTemplate);
    std::vector<ResourceEntry> Snapshot() const;
    void Clear();

private:
    mutable std::mutex mutex;
    std::map<std::string, ResourceEntry> entries;
};
