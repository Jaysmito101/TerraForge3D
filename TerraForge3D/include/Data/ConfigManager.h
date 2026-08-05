#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>

class ConfigManager
{
public:
	ConfigManager();
	~ConfigManager() = default;

	bool LoadLastUsedTheme(std::string& name, std::string& serializedStyle) const;
	bool SaveLastUsedThemeIfChanged(const std::string& name, const std::string& serializedStyle);
	bool GetString(const std::string& section, const std::string& key, std::string& value) const;
	bool SetString(const std::string& section, const std::string& key, const std::string& value);
	bool GetBool(const std::string& section, const std::string& key, bool& value) const;
	bool SetBool(const std::string& section, const std::string& key, bool value);

	inline const std::filesystem::path& GetUserConfigDirectory() const { return m_UserConfigDirectory; }
	inline const std::filesystem::path& GetUserConfigPath() const { return m_UserConfigPath; }

private:
	void Load();
	bool WriteConfig() const;

	std::filesystem::path m_UserConfigDirectory;
	std::filesystem::path m_UserConfigPath;
	nlohmann::json m_Config = nlohmann::json::object();
	std::string m_LastSavedThemeName;
	std::string m_LastSavedThemeData;
};
