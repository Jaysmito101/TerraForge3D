#pragma once
#include "Base/Base.h"

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::data
{

    class ResourceManager
    {
    public:
        virtual ~ResourceManager();

        std::string LoadShaderSource(const std::string shader, bool forceReload = false, bool *success = nullptr);
        std::string PreprocessShaderSource(const std::string &source, const std::string &sourcePath, bool *success = nullptr);
        std::string LoadText(const std::string path, bool forceReload = false, bool *success = nullptr);

        std::optional<base::ComputeShader> GetComputeShader(const std::string name, const std::string source);
        std::optional<base::GraphicsShader> GetGraphicsShader(const std::string name, const std::string vertexSource, const std::string fragmentSource);

        std::optional<base::ComputeShader> LoadComputeShader(const std::string shader, bool forceReload = false, bool *success = nullptr);
        std::optional<base::GraphicsShader> LoadGraphicsShader(const std::string shader, bool forceReload = false, bool *success = nullptr);

        static inline ResourceManager *GetInstance(ApplicationState *appState = nullptr)
        {
            if (m_Instance == nullptr)
                m_Instance = new ResourceManager(appState);

            return m_Instance;
        }

    private:
        ResourceManager(ApplicationState *appSate);
        std::string FixPathSeperator(std::string path);

    private:
        ApplicationState *m_AppState = nullptr;
        std::unordered_map<std::string, std::string> m_TextResources;

        static ResourceManager *m_Instance;
    };

} // namespace tf3d::data
using tf3d::data::ResourceManager;
