#pragma once

#include <array>
#include <cstdint>
#include <memory>

namespace tf3d::base
{
    class ComputeShader;
}
using tf3d::base::ComputeShader;
namespace tf3d::base
{
    class ShaderStorageBuffer;
}
using tf3d::base::ShaderStorageBuffer;
namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::generators
{

    class GeneratorData;

    struct GeneratorDataStatisticsResult {
        static constexpr int HistogramBinCount = 256;
        static constexpr int PercentileCount   = 7;

        std::array<float, HistogramBinCount> histogram{};
        std::array<float, PercentileCount> percentiles{};
        float minimum             = 0.0f;
        float maximum             = 0.0f;
        float requestedPercentile = 0.0f;
        bool valid                = false;
        bool histogramValid       = false;
    };

    class GeneratorDataStatistics
    {
    public:
        using Result                                  = GeneratorDataStatisticsResult;
        static constexpr int HistogramBinCount        = Result::HistogramBinCount;
        static constexpr int PercentileCount          = Result::PercentileCount;
        static constexpr int PercentileStartIndex     = 2 + HistogramBinCount;
        static constexpr int RequestedPercentileIndex = PercentileStartIndex + PercentileCount;
        static constexpr int ResultWordCount          = RequestedPercentileIndex + 1;

        explicit GeneratorDataStatistics(ApplicationState *appState);
        ~GeneratorDataStatistics() = default;

        void Compute(GeneratorData *data, int resolution, int sampleStride = 4,
                     bool includeHistogram = true, float requestedPercentile = -1.0f);
        void Bind(uint32_t binding);
        Result Read() const;

    private:
        ApplicationState *m_AppState = nullptr;
        std::shared_ptr<ComputeShader> m_Shader;
        std::shared_ptr<ShaderStorageBuffer> m_ResultBuffer;
    };

} // namespace tf3d::generators
using tf3d::generators::GeneratorDataStatistics;
using tf3d::generators::GeneratorDataStatisticsResult;
