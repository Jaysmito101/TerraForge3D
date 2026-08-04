#pragma once

#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

struct TF3DNoiseAlgorithmEntry
{
	int value = -1;
	std::string token;
	std::string label;
	std::string description;
};

class TF3DNoiseAlgorithmCatalog
{
public:
	bool LoadFromFile(const std::string& path, std::string* error = nullptr)
	{
		m_Entries.clear();
		m_Labels.clear();
		m_DefaultValue = 0;
		m_Valid = false;

		auto fail = [error](const std::string& message)
		{
			if (error) *error = message;
			return false;
		};

		std::ifstream file(path);
		if (!file.is_open()) return fail("Could not open noise algorithm index: " + path);

		nlohmann::json document;
		try
		{
			file >> document;
		}
		catch (const nlohmann::json::exception& exception)
		{
			return fail("Could not parse noise algorithm index '" + path + "': " + exception.what());
		}

		if (!document.is_object() || !document.contains("Algorithms") || !document["Algorithms"].is_array())
			return fail("Noise algorithm index must contain an Algorithms array: " + path);

		const auto& algorithms = document["Algorithms"];
		if (algorithms.empty()) return fail("Noise algorithm index cannot be empty: " + path);

		for (size_t index = 0; index < algorithms.size(); ++index)
		{
			const auto& algorithm = algorithms[index];
			if (!algorithm.is_object() || !algorithm.contains("Id") || !algorithm["Id"].is_number_integer() ||
				!algorithm.contains("Token") || !algorithm["Token"].is_string() ||
				!algorithm.contains("Label") || !algorithm["Label"].is_string() ||
				!algorithm.contains("Description") || !algorithm["Description"].is_string())
				return fail("Invalid noise algorithm entry at index " + std::to_string(index) + ".");

			const int value = algorithm["Id"].get<int>();
			const std::string token = algorithm["Token"].get<std::string>();
			const std::string label = algorithm["Label"].get<std::string>();
			const std::string description = algorithm["Description"].get<std::string>();
			if (value != static_cast<int>(index) || value < 0 || token.empty() || label.empty() || description.empty())
				return fail("Noise algorithm IDs must be contiguous and entries must have non-empty text.");
			if (!IsValidShaderToken(token)) return fail("Invalid shader token for noise algorithm: " + token);

			for (const auto& previous : m_Entries)
			{
				if (previous.token == token || previous.label == label)
					return fail("Duplicate noise algorithm token or label: " + label);
			}

			m_Entries.push_back({ value, token, label, description });
			m_Labels.push_back(label);
		}

		if (!document.contains("Default") || !document["Default"].is_string())
			return fail("Noise algorithm index must define a string Default value.");
		if (!TryGetValue(document["Default"].get<std::string>(), m_DefaultValue))
			return fail("Noise algorithm index Default does not match an algorithm label.");

		m_Valid = true;
		return true;
	}

	bool IsValid() const { return m_Valid; }
	int Count() const { return static_cast<int>(m_Entries.size()); }
	int MaxValue() const { return m_Entries.empty() ? -1 : m_Entries.back().value; }
	int DefaultValue() const { return m_DefaultValue; }

	const std::vector<TF3DNoiseAlgorithmEntry>& Entries() const { return m_Entries; }
	const std::vector<std::string>& Labels() const { return m_Labels; }

	bool TryGetValue(const std::string& name, int& value) const
	{
		for (const auto& entry : m_Entries)
		{
			if (entry.label == name || entry.token == name)
			{
				value = entry.value;
				return true;
			}
		}
		return false;
	}

	int Value(const std::string& name) const
	{
		int value = -1;
		TryGetValue(name, value);
		return value;
	}

	std::string ShaderDefines() const
	{
		std::string defines = "// Generated from common/noise_algorithms.json\n";
		for (const auto& entry : m_Entries)
			defines += "#define TF3D_NOISE_" + entry.token + " " + std::to_string(entry.value) + "\n";
		defines += "#define TF3D_NOISE_COUNT " + std::to_string(Count()) + "\n";
		return defines;
	}

	std::string InjectShaderDefines(const std::string& source) const
	{
		const std::string defines = ShaderDefines();
		const size_t versionStart = source.find("#version");
		if (versionStart == std::string::npos) return defines + source;

		const size_t versionEnd = source.find('\n', versionStart);
		if (versionEnd == std::string::npos) return source + "\n" + defines;
		return source.substr(0, versionEnd + 1) + defines + source.substr(versionEnd + 1);
	}

	std::string Tooltip() const
	{
		std::string tooltip;
		for (const auto& entry : m_Entries)
		{
			if (!tooltip.empty()) tooltip += " ";
			tooltip += entry.label + ": " + entry.description;
		}
		return tooltip;
	}

	static std::string IndexPath(const std::string& shadersDirectory)
	{
		return (std::filesystem::path(shadersDirectory) / "common" / "noise_algorithms.json").string();
	}

private:
	static bool IsValidShaderToken(const std::string& token)
	{
		for (size_t index = 0; index < token.size(); ++index)
		{
			const char character = token[index];
			const bool isLetter = (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z');
			const bool isDigit = character >= '0' && character <= '9';
			if (!(isLetter || isDigit || character == '_') || (index == 0 && isDigit)) return false;
		}
		return true;
	}

	std::vector<TF3DNoiseAlgorithmEntry> m_Entries;
	std::vector<std::string> m_Labels;
	int m_DefaultValue = 0;
	bool m_Valid = false;
};
