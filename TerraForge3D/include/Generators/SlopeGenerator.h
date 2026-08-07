#pragma once

#include "Generators/GeneratorTexture.h"

#include <memory>

namespace tf3d::base
{
    class ComputeShader;
}
using tf3d::base::ComputeShader;
namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::generators
{

    class GeneratorData;

    class SlopeGenerator
    {
    public:
        SlopeGenerator(ApplicationState *appState, int32_t resolution);
        ~SlopeGenerator() = default;

        void Resize(int32_t resolution);
        bool Compute(GeneratorData *heightmap, int32_t resolution);

        inline GeneratorTexture *GetTexture() const
        {
            return m_Texture.get();
        }
        inline bool IsReady() const
        {
            return m_HasData.load(std::memory_order_acquire);
        }
        inline float GetSampleRadius() const
        {
            return m_SampleRadius;
        }
        inline void SetSampleRadius(float radius)
        {
            m_SampleRadius = radius > 0.0f ? radius : 1.0f;
        }

    private:
        ApplicationState *m_AppState = nullptr;
        std::optional<ComputeShader> m_Shader;
        std::shared_ptr<GeneratorTexture> m_Texture;
        std::atomic_bool m_HasData = false;
        float m_SampleRadius       = 1.0f;
    };

} // namespace tf3d::generators
using tf3d::generators::SlopeGenerator;
