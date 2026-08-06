#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Generators/NoiseAlgorithmCatalog.h"
#include "Misc/CustomInspector.h"
#include "Utils/Utils.h"
#include <nlohmann/json.hpp>

#define BASE_SHAPE_UI_PROPERTY(x) m_RequireUpdation = x || m_RequireUpdation

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::generators
{

    static bool s_TempBool = false;

    class BiomeBaseShapeGenerator
    {
    public:
        BiomeBaseShapeGenerator(ApplicationState *appState);
        ~BiomeBaseShapeGenerator();
        bool LoadConfig(const std::string &config);
        bool LoadConfig(const nlohmann::json &config, const std::string &source, const std::string &shaderPath);
        bool ShowSettings();
        void Update(GeneratorData *buffer, GeneratorTexture *seedTexture);
        void Load(SerializerNode data);
        SerializerNode Save();

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
        inline const std::string &GetSource() const
        {
            return m_Source;
        }
        inline bool RequireUpdation() const
        {
            return m_RequireUpdation;
        }

    private:
        nlohmann::json ParseData(const std::string &config);
        bool LoadInspectorFromConfig(const nlohmann::json &metaData);
        std::string BuildShaderSource();

    protected:
        ApplicationState *m_AppState = nullptr;
        std::shared_ptr<ComputeShader> m_Shader;
        std::shared_ptr<CustomInspector> m_Inspector;
        std::string m_Name        = "";
        std::string m_ID          = "";
        std::string m_Description = "";
        std::string m_Source      = "";
        std::string m_ShaderPath  = "";
        NoiseAlgorithmCatalog m_NoiseAlgorithms;
        bool m_RequireUpdation = true;
    };

} // namespace tf3d::generators
using tf3d::generators::BiomeBaseShapeGenerator;
