#include "Generators/SlopeGenerator.h"

#include "Base/Shader.h"
#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Generators/GeneratorData.h"

#include <algorithm>

namespace tf3d::generators
{

    SlopeGenerator::SlopeGenerator(ApplicationState *appState, int32_t resolution)
        : m_AppState(appState)
    {
        m_Shader  = m_AppState->resourceManager->LoadComputeShader("generation/slope/slope");
        m_Texture = std::make_shared<GeneratorTexture>(std::max(resolution, 1), std::max(resolution, 1), GeneratorTextureStorage::RG32F);
    }

    void SlopeGenerator::Resize(int32_t resolution)
    {
        if (m_Texture == nullptr || resolution <= 0)
            return;
        if (m_Texture->GetWidth() == resolution && m_Texture->GetHeight() == resolution)
            return;
        m_Texture->Resize(resolution, resolution);
        m_HasData.store(false, std::memory_order_release);
    }

    bool SlopeGenerator::Compute(GeneratorData *heightmap, int32_t resolution)
    {
        if (heightmap == nullptr || !m_Shader || m_Texture == nullptr || resolution <= 0)
            return false;
        Resize(resolution);

        heightmap->Bind(0);
        m_Texture->BindForCompute(1);
        m_Shader->Bind();
        m_Shader->SetUniform1i("u_Resolution", resolution);
        m_Shader->SetUniform1f("u_SampleRadius", m_SampleRadius);

        const int workgroupSize = std::max(m_AppState->constants.gpuWorkgroupSize, 1);
        const int dispatchSize  = (resolution + workgroupSize - 1) / workgroupSize;
        m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
        m_Shader->SetMemoryBarrier();
        m_Texture->GenerateMipmaps();
        m_HasData.store(true, std::memory_order_release);
        return true;
    }

} // namespace tf3d::generators
