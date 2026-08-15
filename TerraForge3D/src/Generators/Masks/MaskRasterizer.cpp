#include "Generators/Masks/MaskRasterizer.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Generators/Masks/BaseMaskGenerator.h"
#include "Profiler.h"

#include <glm/common.hpp>

namespace tf3d::generators
{

    MaskRasterizer::MaskRasterizer(tf3d::data::ApplicationState *appState,
                                   const BaseMaskGenerator &baseGenerator)
        : m_AppState(appState)
    {
        if (m_AppState == nullptr)
            return;

        m_StrokeSettingsBuffer = std::make_shared<ShaderStorageBuffer>();
        m_StrokeRangesBuffer   = std::make_shared<ShaderStorageBuffer>();
        m_StrokePointsBuffer   = std::make_shared<ShaderStorageBuffer>();
        m_CopyShader           = m_AppState->resourceManager->LoadComputeShader("generation/utils/mask_copy");

        if (!baseGenerator.GetShaderSource().empty()) {
            m_Shader = m_AppState->resourceManager->GetComputeShader("MaskRasterizer", baseGenerator.GetShaderSource());
            if (!m_Shader.has_value())
                TF3D_LOG_ERROR("Mask rasterizer shader compilation failed; mask rendering is unavailable.");
        } else {
            TF3D_LOG_ERROR("Mask rasterizer could not be created because the base mask shader source is empty.");
        }
    }

    void MaskRasterizer::UploadStrokes(const std::vector<MaskStroke> &strokes, const MaskStroke *activeStroke)
    {
        std::vector<glm::vec4> settings;
        std::vector<glm::ivec4> ranges;
        std::vector<glm::vec4> points;

        const auto appendStroke = [&](const MaskStroke &stroke) {
            if (stroke.points.empty()) {
                return;
            }
            settings.emplace_back(stroke.strength, stroke.size, stroke.falloff, static_cast<float>(stroke.mode));
            ranges.emplace_back(static_cast<int>(points.size()), static_cast<int>(stroke.points.size()), 0, 0);
            for (const auto &point : stroke.points) {
                points.emplace_back(point.x, point.y, 0.0f, 0.0f);
            }
        };

        for (const auto &stroke : strokes) {
            appendStroke(stroke);
        }

        if (activeStroke != nullptr) {
            appendStroke(*activeStroke);
        }

        const glm::vec4 emptySettings(0.0f);
        const glm::ivec4 emptyRange(0);
        const glm::vec4 emptyPoint(0.0f);
        m_StrokeSettingsBuffer->SetData(settings.empty() ? (void *)&emptySettings : (void *)settings.data(),
                                        static_cast<unsigned int>((settings.empty() ? 1 : settings.size()) * sizeof(glm::vec4)));
        m_StrokeRangesBuffer->SetData(ranges.empty() ? (void *)&emptyRange : (void *)ranges.data(),
                                      static_cast<unsigned int>((ranges.empty() ? 1 : ranges.size()) * sizeof(glm::ivec4)));
        m_StrokePointsBuffer->SetData(points.empty() ? (void *)&emptyPoint : (void *)points.data(),
                                      static_cast<unsigned int>((points.empty() ? 1 : points.size()) * sizeof(glm::vec4)));
        m_StrokeDataValid = true;
    }

    bool MaskRasterizer::Dispatch(GeneratorData *sourceData,
                                  const BaseMaskGenerator &baseGenerator,
                                  const BaseMaskGenerator::State &baseState,
                                  GeneratorTexture *destination,
                                  GeneratorTexture *baseTexture,
                                  int strokeCount,
                                  bool useCachedBase)
    {
        if (destination == nullptr || !m_Shader.has_value() || m_AppState == nullptr)
            return false;
        const bool hasBase = baseState.runtimeMode >= 0;
        if (hasBase && !baseGenerator.IsAvailable())
            return false;
        if (hasBase && !useCachedBase && sourceData == nullptr)
            return false;

        if (!m_StrokeDataValid)
            return false;

        if (hasBase && !useCachedBase)
            sourceData->Bind(0);
        destination->BindForCompute(1);
        if (useCachedBase) {
            if (baseTexture == nullptr)
                return false;
            baseTexture->Bind(5);
        }

        m_StrokeSettingsBuffer->Bind(2);
        m_StrokeRangesBuffer->Bind(3);
        m_StrokePointsBuffer->Bind(4);

        m_Shader->Bind();
        baseGenerator.ApplyToShader(baseState, *m_Shader);
        m_Shader->SetUniform1i("u_Resolution", destination->GetWidth());
        m_Shader->SetUniform1i("u_UseCachedBase", useCachedBase ? 1 : 0);
        m_Shader->SetUniform1i("u_BaseMask", 5);
        m_Shader->SetUniform1i("u_StrokeCount", strokeCount);
        m_Shader->SetUniform1f("u_TileSize", m_AppState->mainMap.tileSize);

        const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
        const auto dispatchSize  = (destination->GetWidth() + workgroupSize - 1) / workgroupSize;
        m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
        m_Shader->SetMemoryBarrier();
        return true;
    }

    bool MaskRasterizer::Render(GeneratorData *sourceData,
                                const BaseMaskGenerator &baseGenerator,
                                const BaseMaskGenerator::State &baseState,
                                const std::vector<MaskStroke> &strokes,
                                const MaskStroke *activeStroke,
                                GeneratorTexture *destination,
                                GeneratorTexture *baseTexture,
                                bool rebuildBase)
    {
        TF3D_PROFILE_SCOPE_DOMAIN("generation/mask/rasterize", PerformanceMonitor::Domain::Generation);
        if (destination == nullptr || destination->GetWidth() <= 0) {
            return false;
        }

        UploadStrokes(strokes, activeStroke);

        int strokeCount = 0;
        for (const auto &stroke : strokes) {
            strokeCount += !stroke.points.empty();
        }

        if (activeStroke != nullptr && !activeStroke->points.empty()) {
            strokeCount += 1;
        }

        const bool hasBase = baseState.runtimeMode >= 0;
        if (hasBase) {
            if (baseTexture == nullptr || baseTexture->GetWidth() != destination->GetWidth()) {
                return false;
            }

            if (rebuildBase && !Dispatch(sourceData, baseGenerator, baseState, baseTexture, nullptr, 0, false)) {
                return false;
            }
            
            return Dispatch(nullptr, baseGenerator, baseState, destination, baseTexture, strokeCount, true);
        }

        return Dispatch(sourceData, baseGenerator, baseState, destination, nullptr, strokeCount, false);
    }

    GeneratorTexture *MaskRasterizer::GetPreviewTexture(GeneratorTexture *sourceTexture,
                                                        GeneratorTexture *visualizationTexture,
                                                        bool invert)
    {
        if (sourceTexture == nullptr || visualizationTexture == nullptr || !invert ||
            !m_CopyShader.has_value() || m_AppState == nullptr)
            return sourceTexture;
        const int visualizationResolution = visualizationTexture->GetWidth();
        if (visualizationResolution <= 0)
            return sourceTexture;

        sourceTexture->Bind(0);
        visualizationTexture->BindForCompute(1);
        m_CopyShader->Bind();
        m_CopyShader->SetUniform1i("u_Resolution", visualizationResolution);
        m_CopyShader->SetUniform1i("u_SourceMask", 0);
        m_CopyShader->SetUniform1i("u_Invert", 1);
        const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
        const auto dispatchSize  = (visualizationResolution + workgroupSize - 1) / workgroupSize;
        m_CopyShader->Dispatch(dispatchSize, dispatchSize, 1);
        m_CopyShader->SetMemoryBarrier();
        return visualizationTexture;
    }

} // namespace tf3d::generators
