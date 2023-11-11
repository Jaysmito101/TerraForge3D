#include "Data/ResourceManager.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"

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
	Log("Loading shader: " + shader);
	auto shaderPath = FixPathSeperator(shader);
	shaderPath = m_AppState->constants.shadersDir + PATH_SEPARATOR + shaderPath + ".glsl";
	return LoadText(shaderPath, forceReload, success);
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

	Log("Compiling compute shader: " + name);
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

	Log("Compiling shader: " + name);
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