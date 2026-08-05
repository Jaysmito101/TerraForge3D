#include "Data/ConfigManager.h"

#include "Base/Logging/Logger.h"

#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>

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
	m_Config = nlohmann::json::object();
	if (!std::filesystem::exists(m_UserConfigPath)) return;

	try
	{
		std::ifstream file(m_UserConfigPath);
		nlohmann::json config;
		file >> config;
		if (!config.is_object())
			throw std::runtime_error("user config root must be a JSON object");

		m_Config = std::move(config);
		if (m_Config.contains("theme") && m_Config["theme"].is_object())
		{
			const auto& theme = m_Config["theme"];
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

bool ConfigManager::GetString(const std::string& section, const std::string& key, std::string& value) const
{
	const auto sectionIt = m_Config.find(section);
	if (sectionIt == m_Config.end() || !sectionIt->is_object()) return false;

	const auto valueIt = sectionIt->find(key);
	if (valueIt == sectionIt->end() || !valueIt->is_string()) return false;

	value = valueIt->get<std::string>();
	return true;
}

bool ConfigManager::SetString(const std::string& section, const std::string& key, const std::string& value)
{
	const nlohmann::json previousConfig = m_Config;
	if (!m_Config.is_object()) m_Config = nlohmann::json::object();

	auto& sectionObject = m_Config[section];
	if (!sectionObject.is_object()) sectionObject = nlohmann::json::object();
	const auto existingValue = sectionObject.find(key);
	if (existingValue != sectionObject.end() && existingValue->is_string() && existingValue->get<std::string>() == value)
		return true;

	sectionObject[key] = value;
	if (WriteConfig()) return true;

	m_Config = previousConfig;
	return false;
}

bool ConfigManager::GetBool(const std::string& section, const std::string& key, bool& value) const
{
	const auto sectionIt = m_Config.find(section);
	if (sectionIt == m_Config.end() || !sectionIt->is_object()) return false;

	const auto valueIt = sectionIt->find(key);
	if (valueIt == sectionIt->end() || !valueIt->is_boolean()) return false;

	value = valueIt->get<bool>();
	return true;
}

bool ConfigManager::SetBool(const std::string& section, const std::string& key, bool value)
{
	const nlohmann::json previousConfig = m_Config;
	if (!m_Config.is_object()) m_Config = nlohmann::json::object();

	auto& sectionObject = m_Config[section];
	if (!sectionObject.is_object()) sectionObject = nlohmann::json::object();
	const auto existingValue = sectionObject.find(key);
	if (existingValue != sectionObject.end() && existingValue->is_boolean() && existingValue->get<bool>() == value)
		return true;

	sectionObject[key] = value;
	if (WriteConfig()) return true;

	m_Config = previousConfig;
	return false;
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
		nlohmann::json config = m_Config.is_object() ? m_Config : nlohmann::json::object();
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
