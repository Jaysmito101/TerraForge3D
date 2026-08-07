#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Generators/NoiseAlgorithmCatalog.h"
#include "Misc/CustomInspector.h"
#include "Utils/Utils.h"
#include <nlohmann/json.hpp>

#define BASE_NOISE_UI_PROPERTY(x)     m_RequireUpdation = x || m_RequireUpdation

#define BIOME_BASE_NOISE_OCTAVE_COUNT 10

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::generators
{

    class BiomeBaseNoiseGenerator
    {
    public:
        BiomeBaseNoiseGenerator(ApplicationState *appState);
        ~BiomeBaseNoiseGenerator();

        bool LoadConfig(const nlohmann::json &config, const std::string &source, const std::string &shaderPath);
        bool ShowSettings();
        void Update(GeneratorData *sourceBuffer, GeneratorData *targetBuffer, GeneratorTexture *seedTexture);

        void Load(SerializerNode data);
        SerializerNode Save();

        inline bool RequireUpdation() const
        {
            return m_RequireUpdation;
        }

    private:
        data::ApplicationState *m_AppState = nullptr;
        bool m_RequireUpdation             = false;
        std::shared_ptr<base::ComputeShader> m_Shader;
        std::shared_ptr<misc::CustomInspector> m_Inspector;
        NoiseAlgorithmCatalog m_NoiseAlgorithms;
        std::string m_Name        = "Base Noise";
        std::string m_ID          = "base_noise";
        std::string m_Description = "";
        std::string m_Source      = "";
        std::string m_ShaderPath  = "";
    };

} // namespace tf3d::generators
using tf3d::generators::BiomeBaseNoiseGenerator;
