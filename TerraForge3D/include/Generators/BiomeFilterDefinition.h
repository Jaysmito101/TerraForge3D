#pragma once

#include "Base/Base.h"
#include "Misc/CustomInspector.h"

#include <filesystem>
#include <nlohmann/json.hpp>
#include <unordered_map>

class ApplicationState;
class ComputeShader;

class BiomeFilterDefinition
{
public:
	static std::shared_ptr<BiomeFilterDefinition> LoadFromFolder(
		const std::filesystem::path& folder,
		const std::filesystem::path& shaderRoot);

	bool BuildInspector(CustomInspector& inspector) const;
	std::shared_ptr<ComputeShader> GetPhaseShader(ApplicationState* appState, const std::string& phase) const;

	inline const std::string& GetID() const { return m_ID; }
	inline const std::string& GetName() const { return m_Name; }
	inline const std::string& GetCategory() const { return m_Category; }
	inline const std::string& GetDescription() const { return m_Description; }
	inline const std::string& GetImplementation() const { return m_Implementation; }
	inline const std::string& GetSearchText() const { return m_SearchText; }
	inline const nlohmann::json& GetMetadata() const { return m_Metadata; }

private:
	BiomeFilterDefinition() = default;

	std::string m_ID;
	std::string m_Name;
	std::string m_Category = "Other";
	std::string m_Description;
	std::string m_Implementation;
	std::string m_SearchText;
	std::string m_FolderShaderPath;
	std::unordered_map<std::string, std::string> m_PhaseShaderPaths;
	mutable std::unordered_map<std::string, std::shared_ptr<ComputeShader>> m_PhaseShaders;
	nlohmann::json m_Metadata;
};
