#include "Generators/GeneratorDataStatistics.h"

#include "Base/ShaderStorageBuffer.h"
#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Generators/GeneratorData.h"
#include "Utils/Utils.h"

#include <algorithm>
#include <vector>

namespace tf3d::generators
{

    GeneratorDataStatistics::GeneratorDataStatistics(ApplicationState *appState)
        : m_AppState(appState)
    {
        m_Shader       = m_AppState->resourceManager->LoadComputeShader("generation/utils/generator_data_statistics");
        m_ResultBuffer = std::make_shared<ShaderStorageBuffer>();
        std::vector<uint32_t> initial(GeneratorDataStatistics::ResultWordCount, 0u);
        initial[0] = 0xffffffffu;
        m_ResultBuffer->SetData(initial.data(), static_cast<unsigned int>(initial.size() * sizeof(uint32_t)));
    }

    void GeneratorDataStatistics::Compute(GeneratorData *data, int resolution, int sampleStride,
                                          bool includeHistogram, float requestedPercentile)
    {
        if (data == nullptr || !m_Shader || m_ResultBuffer == nullptr || resolution <= 0)
            return;

        sampleStride = std::max(sampleStride, 1);
        std::vector<uint32_t> initial(GeneratorDataStatistics::ResultWordCount, 0u);
        initial[0] = 0xffffffffu;
        m_ResultBuffer->SetData(initial.data(), static_cast<unsigned int>(initial.size() * sizeof(uint32_t)));

        data->Bind(0);
        m_ResultBuffer->Bind(1);
        m_Shader->Bind();
        m_Shader->SetUniform1i("u_Resolution", resolution);
        m_Shader->SetUniform1i("u_SampleStride", sampleStride);

        const int workgroupSize    = m_AppState->constants.gpuWorkgroupSize;
        const int sampleResolution = (resolution + sampleStride - 1) / sampleStride;
        const int dispatchSize     = (sampleResolution + workgroupSize - 1) / workgroupSize;

        m_Shader->SetUniform1i("u_Mode", 0);
        m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
        m_Shader->SetMemoryBarrier();

        if (!includeHistogram)
            return;

        m_Shader->SetUniform1i("u_Mode", 1);
        m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
        m_Shader->SetMemoryBarrier();

        m_Shader->SetUniform1i("u_HasRequestedPercentile", requestedPercentile >= 0.0f ? 1 : 0);
        m_Shader->SetUniform1f("u_RequestedPercentile", std::clamp(requestedPercentile, 0.0f, 1.0f));
        m_Shader->SetUniform1i("u_Mode", 2);
        m_Shader->Dispatch(1, 1, 1);
        m_Shader->SetMemoryBarrier();
    }

    void GeneratorDataStatistics::Bind(uint32_t binding)
    {
        if (m_ResultBuffer != nullptr)
            m_ResultBuffer->Bind(static_cast<int>(binding));
    }

    GeneratorDataStatistics::Result GeneratorDataStatistics::Read() const
    {
        Result statistics;
        if (m_ResultBuffer == nullptr)
            return statistics;

        std::array<uint32_t, GeneratorDataStatistics::ResultWordCount> result{};
        m_ResultBuffer->Bind();
        m_ResultBuffer->GetData(result.data(), static_cast<int>(result.size() * sizeof(uint32_t)));

        if (result[0] == 0xffffffffu) {
            return statistics;
        }

        statistics.minimum    = OrderedUintToFloat(result[0]);
        statistics.maximum    = OrderedUintToFloat(result[1]);
        uint32_t maximumCount = 0u;
        uint32_t totalCount   = 0u;
        for (int index = 0; index < HistogramBinCount; ++index) {
            maximumCount = std::max(maximumCount, result[2 + index]);
            totalCount += result[2 + index];
        }

        for (int index = 0; index < HistogramBinCount; ++index)
            statistics.histogram[index] = maximumCount == 0u
                                              ? 0.0f
                                              : static_cast<float>(result[2 + index]) / static_cast<float>(maximumCount);
        statistics.valid          = true;
        statistics.histogramValid = totalCount != 0u;
        for (int index = 0; index < PercentileCount; ++index)
            statistics.percentiles[index] = RawUintToFloat(result[PercentileStartIndex + index]);
        statistics.requestedPercentile = RawUintToFloat(result[RequestedPercentileIndex]);
        return statistics;
    }

} // namespace tf3d::generators
