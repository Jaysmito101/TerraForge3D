#pragma once
#include "Base/Base.h"


class ApplicationState;

class ResourceManager
{
public:
	virtual ~ResourceManager();

	std::string LoadShaderSource(const std::string shader, bool forceReload = false, bool* success = nullptr);
	std::string LoadText(const std::string path, bool forceReload = false, bool* success = nullptr);

	std::shared_ptr<ComputeShader> GetComputeShader(const std::string name, const std::string source);
	std::shared_ptr<Shader> GetShader(const std::string name, const std::string vertexSource, const std::string fragmentSource);

	std::shared_ptr<ComputeShader> LoadComputeShader(const std::string shader, bool forceReload = false, bool* success = nullptr);
	std::shared_ptr<Shader> LoadShader(const std::string shader, bool forceReload = false, bool* success = nullptr);


	static inline ResourceManager* GetInstance(ApplicationState* appState = nullptr) 
	{
		if (m_Instance == nullptr)
			m_Instance = new ResourceManager(appState);

		return m_Instance;
	}

private:
	ResourceManager(ApplicationState* appSate);
	std::string FixPathSeperator(std::string path);

private:
	ApplicationState* m_AppState = nullptr;
	std::unordered_map<std::string, std::string> m_TextResources;
	std::unordered_map<std::string, std::pair<std::shared_ptr<ComputeShader>, size_t>> m_ComputeShaders;
	std::unordered_map<std::string, std::pair<std::shared_ptr<Shader>, size_t>> m_Shaders;

	
	static ResourceManager* m_Instance;
};