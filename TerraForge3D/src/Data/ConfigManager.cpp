#include "Data/ConfigManager.h"

#include "Base/Logging/Logger.h"

#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>

namespace
{
	std::filesystem::path ResolveUserConfigDirectory()
	{
		const char* home = std::getenv("USERPROFILE");
		if (!home) home = std::getenv("HOME");
		if (home && *home) return std::filesystem::path(home) / ".terraforge3d";
		return std::filesystem::current_path() / ".terraforge3d";
	}
}

ConfigManager::ConfigManager()
	: m_UserConfigDirectory(ResolveUserConfigDirectory()),
	  m_UserConfigPath(m_UserConfigDirectory / "user_config.json")
{
	try
	{
		std::filesystem::create_directories(m_UserConfigDirectory);
	}
	catch (const std::exception& exception)
	{
		TF3D_LOG_ERROR("Failed to create user config directory '{}': {}", m_UserConfigDirectory.string(), exception.what());
	}
	Load();
	TF3D_LOG_DEBUG("User config directory: '{}'", m_UserConfigDirectory.string());
}

void ConfigManager::Load()
{
	if (!std::filesystem::exists(m_UserConfigPath)) return;

	try
	{
		std::ifstream file(m_UserConfigPath);
		nlohmann::json config;
		file >> config;
		if (config.contains("theme") && config["theme"].is_object())
		{
			const auto& theme = config["theme"];
			m_LastSavedThemeName = theme.value("name", "Default");
			m_LastSavedThemeData = theme.value("style", "");
		}
	}
	catch (const std::exception& exception)
	{
		TF3D_LOG_ERROR("Failed to load user config '{}': {}", m_UserConfigPath.string(), exception.what());
	}
}

bool ConfigManager::LoadLastUsedTheme(std::string& name, std::string& serializedStyle) const
{
	if (m_LastSavedThemeData.empty()) return false;
	name = m_LastSavedThemeName;
	serializedStyle = m_LastSavedThemeData;
	return true;
}

bool ConfigManager::SaveLastUsedThemeIfChanged(const std::string& name, const std::string& serializedStyle)
{
	if (name == m_LastSavedThemeName && serializedStyle == m_LastSavedThemeData) return false;
	const std::string previousName = m_LastSavedThemeName;
	const std::string previousStyle = m_LastSavedThemeData;
	m_LastSavedThemeName = name;
	m_LastSavedThemeData = serializedStyle;
	if (WriteConfig()) return true;

	m_LastSavedThemeName = previousName;
	m_LastSavedThemeData = previousStyle;
	return false;
}

bool ConfigManager::WriteConfig() const
{
	try
	{
		nlohmann::json config;
		config["version"] = 1;
		config["theme"] = { { "name", m_LastSavedThemeName }, { "style", m_LastSavedThemeData } };

		const auto temporaryPath = m_UserConfigPath.string() + ".tmp";
		{
			std::ofstream file(temporaryPath, std::ios::trunc);
			if (!file.is_open()) return false;
			file << config.dump(1, '\t');
		}

		std::error_code error;
		std::filesystem::remove(m_UserConfigPath, error);
		error.clear();
		std::filesystem::rename(temporaryPath, m_UserConfigPath, error);
		if (error)
		{
			TF3D_LOG_ERROR("Failed to replace user config '{}': {}", m_UserConfigPath.string(), error.message());
			return false;
		}
		return true;
	}
	catch (const std::exception& exception)
	{
		TF3D_LOG_ERROR("Failed to save user config '{}': {}", m_UserConfigPath.string(), exception.what());
		return false;
	}
}
