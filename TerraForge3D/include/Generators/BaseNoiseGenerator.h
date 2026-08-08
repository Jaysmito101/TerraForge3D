#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Generators/CalculatedMaskGenerator.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Generators/MaskTool.h"
#include "Generators/NoiseAlgorithmCatalog.h"
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

    class BaseNoiseGenerator
    {
    public:
        explicit BaseNoiseGenerator(ApplicationState *appState);
        ~BaseNoiseGenerator();

        bool LoadConfig(const nlohmann::json &config, const std::string &source, const std::string &shaderPath);
        bool ShowSettings();
        void Resize(int size);
        void Update(GeneratorData *sourceBuffer, GeneratorData *targetBuffer, GeneratorTexture *seedTexture,
                    std::string_view profilePrefix = {});

        void Load(SerializerNode data);
        SerializerNode Save();

        inline bool RequireUpdation() const
        {
            return m_RequireUpdation;
        }

    private:
        data::ApplicationState *m_AppState = nullptr;
        bool m_RequireUpdation             = false;
        bool m_UseMask                     = false;
        bool m_InvertMask                  = false;
        std::optional<base::ComputeShader> m_Shader;
        std::shared_ptr<inspector::CustomInspector> m_Inspector;
        std::shared_ptr<CalculatedMaskGenerator> m_CalculatedMaskGenerator;
        std::shared_ptr<MaskTool> m_MaskTool;
        NoiseAlgorithmCatalog m_NoiseAlgorithms;
        std::string m_Name        = "Base Noise";
        std::string m_ID          = "base_noise";
        std::string m_Description = "";
        std::string m_Source      = "";
        std::string m_ShaderPath  = "";
    };

} // namespace tf3d::generators
