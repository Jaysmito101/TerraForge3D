#pragma once

#include <filesystem>
#include <string>

class ConfigManager
{
public:
	ConfigManager();
	~ConfigManager() = default;

	bool LoadLastUsedTheme(std::string& name, std::string& serializedStyle) const;
	bool SaveLastUsedThemeIfChanged(const std::string& name, const std::string& serializedStyle);

	inline const std::filesystem::path& GetUserConfigDirectory() const { return m_UserConfigDirectory; }
	inline const std::filesystem::path& GetUserConfigPath() const { return m_UserConfigPath; }

private:
	void Load();
	bool WriteConfig() const;

	std::filesystem::path m_UserConfigDirectory;
	std::filesystem::path m_UserConfigPath;
	std::string m_LastSavedThemeName;
	std::string m_LastSavedThemeData;
};
