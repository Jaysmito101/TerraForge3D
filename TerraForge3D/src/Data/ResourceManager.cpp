#include "Data/ResourceManager.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"

#include <filesystem>
#include <algorithm>
#include <functional>
#include <sstream>
#include <vector>

namespace
{
	std::string NormalizeShaderPath(std::string path)
	{
		for (char& c : path)
		{
			if (c == '\\') c = '/';
		}
		return path;
	}
}

ResourceManager* ResourceManager::m_Instance = nullptr;

ResourceManager::ResourceManager(ApplicationState* appState) 
{
	m_AppState = appState;
}

// replace all / with \\ or / based on os
std::string ResourceManager::FixPathSeperator(std::string path)
{
	for (char& c : path)
	{
		if (c == '/' || c == '\\')
		{
			c = PATH_SEPARATOR[0];
		}
	}
	return path;
}


ResourceManager::~ResourceManager()
{

}

std::string ResourceManager::LoadShaderSource(const std::string shader, bool forceReload, bool* success)
{
	TF3D_LOG_INFO("Loading shader: '{}'", shader);
	auto shaderPath = FixPathSeperator(shader);
	shaderPath = m_AppState->constants.shadersDir + PATH_SEPARATOR + shaderPath + ".glsl";
	bool loaded = false;
	auto source = LoadText(shaderPath, forceReload, &loaded);
	if (!loaded)
	{
		if (success) *success = false;
		return "";
	}

	return PreprocessShaderSource(source, shader + ".glsl", success);
}

std::string ResourceManager::PreprocessShaderSource(const std::string& source, const std::string& sourcePath, bool* successOut)
{
	bool success = true;
	if (!successOut) successOut = &success;
	*successOut = true;

	std::vector<std::string> includeStack;
	std::function<std::string(const std::string&, const std::string&)> expand;
	expand = [&](const std::string& currentSource, const std::string& currentPath) -> std::string
	{
		std::istringstream input(currentSource);
		std::ostringstream output;
		std::string line;
		const auto normalizedCurrentPath = NormalizeShaderPath(currentPath);

		while (std::getline(input, line))
		{
			std::string trimmed = line;
			const auto first = trimmed.find_first_not_of(" \t");
			if (first != std::string::npos) trimmed.erase(0, first);

			if (trimmed.rfind("#include", 0) != 0)
			{
				output << line << '\n';
				continue;
			}

			const auto open = trimmed.find('"', 8);
			const auto close = open == std::string::npos ? std::string::npos : trimmed.find('"', open + 1);
			if (open == std::string::npos || close == std::string::npos)
			{
				TF3D_LOG_ERROR("Malformed shader include in '{}'", normalizedCurrentPath);
				*successOut = false;
				return "";
			}

			const std::string includeName = NormalizeShaderPath(trimmed.substr(open + 1, close - open - 1));
			const auto currentDirectory = std::filesystem::path(normalizedCurrentPath).parent_path();
			const auto relativeCandidate = std::filesystem::path(m_AppState->constants.shadersDir) / currentDirectory / includeName;
			const auto rootCandidate = std::filesystem::path(m_AppState->constants.shadersDir) / includeName;
			std::filesystem::path includePath = relativeCandidate;
			if (!FileExists(includePath.string())) includePath = rootCandidate;

			if (!FileExists(includePath.string()))
			{
				TF3D_LOG_ERROR("Could not resolve shader include '{}' from '{}'", includeName, normalizedCurrentPath);
				*successOut = false;
				return "";
			}

			const auto canonicalPath = std::filesystem::weakly_canonical(includePath).string();
			if (std::find(includeStack.begin(), includeStack.end(), canonicalPath) != includeStack.end())
			{
				TF3D_LOG_ERROR("Circular shader include '{}'", includeName);
				*successOut = false;
				return "";
			}

			bool includeLoaded = false;
			const auto includeSource = LoadText(includePath.string(), true, &includeLoaded);
			if (!includeLoaded)
			{
				*successOut = false;
				return "";
			}

			includeStack.push_back(canonicalPath);
			output << expand(includeSource, std::filesystem::relative(includePath, m_AppState->constants.shadersDir).generic_string());
			includeStack.pop_back();
			if (!*successOut) return "";
		}

		return output.str();
	};

	includeStack.push_back(std::filesystem::weakly_canonical(std::filesystem::path(m_AppState->constants.shadersDir) / sourcePath).string());
	return expand(source, sourcePath);
}

std::string ResourceManager::LoadText(const std::string path, bool forceReload, bool* successOut)
{
	bool success = false;
	if (!successOut) successOut = &success;

	if (!forceReload && m_TextResources.find(path) != m_TextResources.end()) 
	{
		*successOut = true;
		return m_TextResources[path];
	}

	if (!FileExists(path)) 
	{
		*successOut = false;
		return "";
	}
	
	auto data = ReadShaderSourceFile(path, successOut);

	if (*successOut)
	{
		m_TextResources[path] = data;
	}

	return data;
}



std::shared_ptr<ComputeShader> ResourceManager::GetComputeShader(const std::string name, const std::string source)
{
	auto hash = std::hash<std::string>{}(source);
	if (m_ComputeShaders.find(name) != m_ComputeShaders.end())
	{
		if (hash == m_ComputeShaders[name].second)
		{
			return m_ComputeShaders[name].first;
		}
	}

	TF3D_LOG_DEBUG("Compiling compute shader '{}'", name);
	auto shader = std::make_shared<ComputeShader>(source);
	m_ComputeShaders[name] = std::make_pair(shader, hash);

	return shader;
}

std::shared_ptr<Shader> ResourceManager::GetShader(const std::string name, const std::string vertexSource, const std::string fragmentSource)
{
	auto hash = std::hash<std::string>{}(vertexSource + fragmentSource);

	if (m_Shaders.find(name) != m_Shaders.end())
	{
		if (hash == m_Shaders[name].second)
		{
			return m_Shaders[name].first;
		}
	}

	TF3D_LOG_DEBUG("Compiling shader '{}'", name);
	auto shader = std::make_shared<Shader>(vertexSource, fragmentSource);
	m_Shaders[name] = std::make_pair(shader, hash);

	return shader;
}

std::shared_ptr<Shader> ResourceManager::LoadShader(const std::string shader, bool forceReload, bool* successOut)
{
	bool success = false;
	if (!successOut) successOut = &success;

	auto vertexSource = LoadShaderSource(shader + "/vert", forceReload, successOut);
	if (!*successOut) return nullptr;

	auto fragmentSource = LoadShaderSource(shader + "/frag", forceReload, successOut);
	if (!*successOut) return nullptr;

	return GetShader(shader, vertexSource, fragmentSource);
}

std::shared_ptr<ComputeShader> ResourceManager::LoadComputeShader(const std::string shader, bool forceReload, bool* successOut)
{
	bool success = false;
	if (!successOut) successOut = &success;

	auto source = LoadShaderSource(shader, forceReload, successOut);
	if (!*successOut) return nullptr;

	return GetComputeShader(shader, source);
}
