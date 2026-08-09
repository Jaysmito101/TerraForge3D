#pragma once

#include "Base/Base.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Inspector/CustomInspector.h"
#include <nlohmann/json.hpp>
#include <string_view>

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::generators
{

    class BiomeBaseShapeGenerator
    {
    public:
        BiomeBaseShapeGenerator(ApplicationState *appState);
        ~BiomeBaseShapeGenerator() = default;
        bool LoadConfig(const nlohmann::json &config, const std::string &source, const std::string &shaderPath);
        bool ShowSettings();
        void Update(GeneratorData *buffer, GeneratorTexture *seedTexture, std::string_view profilePrefix = {});

        inline const std::string &GetName() const
        {
            return m_Name;
        }
        inline const std::string &GetID() const
        {
            return m_ID;
        }
        inline const std::string &GetDescription() const
        {
            return m_Description;
        }
        inline bool RequireUpdation() const
        {
            return m_RequireUpdation;
        }

    private:
        bool LoadInspectorFromConfig(const nlohmann::json &metaData);
        std::string BuildShaderSource(const std::string &templateSource,
                                      const std::string &uniformDeclarations);

    protected:
        ApplicationState *m_AppState = nullptr;
        std::optional<base::ComputeShader> m_Shader;
        std::shared_ptr<inspector::CustomInspector> m_Inspector;
        std::string m_Name        = "";
        std::string m_ID          = "";
        std::string m_Description = "";
        std::string m_Source      = "";
        std::string m_ShaderPath  = "";
        bool m_RequireUpdation    = true;
    };

} // namespace tf3d::generators
using tf3d::generators::BiomeBaseShapeGenerator;
