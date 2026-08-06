#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Generators/NoiseAlgorithmCatalog.h"
#include "Misc/CustomInspector.h"
#include "Utils/Utils.h"
#include <array>
#include <nlohmann/json.hpp>

#define BASE_SHAPE_UI_PROPERTY(x)     m_RequireUpdation = x || m_RequireUpdation

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

        bool ShowSettings();
        void Update(GeneratorData *sourceBuffer, GeneratorData *targetBuffer, GeneratorTexture *seedTexture);

        void Load(SerializerNode data);
        SerializerNode Save();

        inline bool RequireUpdation() const
        {
            return m_RequireUpdation;
        }

    private:
        ApplicationState *m_AppState = nullptr;
        bool m_RequireUpdation       = true;
        std::shared_ptr<ComputeShader> m_Shader;
        std::shared_ptr<CustomInspector> m_Inspector;
        NoiseAlgorithmCatalog m_NoiseAlgorithms;
        std::array<float, BIOME_BASE_NOISE_OCTAVE_COUNT> m_NoiseOctaveStrengths{};
    };

} // namespace tf3d::generators
using tf3d::generators::BiomeBaseNoiseGenerator;
