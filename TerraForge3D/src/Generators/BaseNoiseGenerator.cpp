#include "Generators/BaseNoiseGenerator.h"

#include "Data/ApplicationState.h"
#include "Profiler.h"
#include "UI/ImGuiComponents.h"

#include <algorithm>

namespace tf3d::generators
{

    BaseNoiseGenerator::BaseNoiseGenerator(tf3d::data::ApplicationState *appState)
        : m_AppState(appState), m_Inspector(std::make_shared<inspector::CustomInspector>())
    {
        if (m_AppState == nullptr)
            return;

        m_MaskLayer = std::make_shared<MaskLayer>(m_AppState, glm::vec3(1.0f, 0.65f, 0.1f),
                                                  "SlopeRamp");
    }

    bool BaseNoiseGenerator::Initialize()
    {
        if (m_AppState == nullptr || m_AppState->resourceManager == nullptr) {
            TF3D_LOG_ERROR("Failed to initialize base-noise generator: application resources are unavailable.");
            return false;
        }

        if (!m_Inspector->LoadConfig(m_AppState, "BaseNoise"))
            return false;

        bool shaderLoaded              = false;
        const std::string shaderSource = m_AppState->resourceManager->LoadShaderSource(
            "generation/base_noise/noise_gen", false, &shaderLoaded);
        if (!shaderLoaded) {
            TF3D_LOG_ERROR("Failed to load base-noise shader source.");
            return false;
        }

        m_Shader = m_AppState->resourceManager->GetComputeShader("BaseNoiseGen", shaderSource);
        if (!m_Shader.has_value()) {
            TF3D_LOG_ERROR("Failed to compile base-noise shader.");
            return false;
        }

        m_RequireUpdation = true;
        return true;
    }

    bool BaseNoiseGenerator::ShowSettings()
    {
        const auto markChanged = [this](bool changed) {
            m_RequireUpdation = changed || m_RequireUpdation;
        };

        ImGui::PushID("BaseNoiseGenerator");
        markChanged(m_Inspector->Render());

        markChanged(ImGui::Checkbox("Use mask", &m_UseMask));
        if (m_UseMask && m_MaskLayer != nullptr) {
            if (ImGui::CollapsingHeader("Mask Tool")) {
                markChanged(ImGui::Checkbox("Invert mask", &m_InvertMask));
                m_MaskLayer->SetInvertPreview(m_InvertMask);
                markChanged(m_MaskLayer->ShowSettings(true));
            }
        } else if (m_MaskLayer != nullptr) {
            m_MaskLayer->SetInvertPreview(false);
            ImGui::TextDisabled("Mask: Global");
        }
        ImGui::PopID();

        return m_RequireUpdation;
    }

    void BaseNoiseGenerator::Resize(int size)
    {
        if (size <= 0)
            return;
        if (m_MaskLayer != nullptr)
            m_MaskLayer->Resize(size);
    }

    void BaseNoiseGenerator::Update(GeneratorData *sourceBuffer, GeneratorData *targetBuffer,
                                    GeneratorTexture *seedTexture)
    {
        if (!m_Shader || sourceBuffer == nullptr || targetBuffer == nullptr)
            return;

        TF3D_PROFILE_SCOPE_CHILD_LAZY("base-noise");

        const bool useMask = m_UseMask && m_MaskLayer != nullptr &&
                             m_MaskLayer->GetTexture() != nullptr;
        if (useMask) {
            m_MaskLayer->Update(sourceBuffer);
        }

        sourceBuffer->Bind(0);
        targetBuffer->Bind(1);

        m_Shader->Bind();
        m_Inspector->ApplyToShader(*m_Shader);
        m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
        const auto inspectorRoot   = m_Inspector->Root();
        const auto octaveStrengths = inspectorRoot.Scope("Octaves").Get("OctaveStrengths", std::vector<float>{});
        m_Shader->SetUniform1i("u_NoiseOctaveStrengthsCount",
                               std::clamp(static_cast<int>(octaveStrengths.size()), 0, 16));
        m_Shader->SetUniform1i("u_UseSeedTexture",
                               (seedTexture != nullptr && inspectorRoot.Scope("Blend").Get("AutoUseSeedTexture", false)) ? 1 : 0);
        m_Shader->SetUniform1i("u_UseMask", useMask ? 1 : 0);
        m_Shader->SetUniform1i("u_InvertMask", useMask && m_InvertMask ? 1 : 0);
        if (seedTexture) {
            m_Shader->SetUniform1i("u_SeedTexture", seedTexture->Bind(1));
        }
        if (useMask) {
            m_Shader->SetUniform1i("u_MaskTexture", m_MaskLayer->GetTexture()->Bind(3));
        }
        const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
        const auto dispatchSize  = (m_AppState->mainMap.tileResolution + workgroupSize - 1) / workgroupSize;
        TF3D_PROFILE_GPU_SCOPE_CHILD("gpu");
        TF3D_PROFILE_VALUE_DOMAIN("generation/base-noise/dispatch", dispatchSize, dispatchSize, 1,
                                  PerformanceMonitor::Domain::Generation);
        m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
        m_Shader->SetMemoryBarrier();

        m_RequireUpdation = false;
    }

    void BaseNoiseGenerator::Load(SerializerNode data)
    {
        if (data == nullptr)
            return;

        m_UseMask    = data->Get<bool>("UseMask", m_UseMask);
        m_InvertMask = data->Get<bool>("InvertMask", m_InvertMask);

        if (const auto inspector = data->Get<SerializerNode>("Inspector"); inspector != nullptr)
            m_Inspector->LoadState(inspector);

        if (m_MaskLayer != nullptr) {
            m_MaskLayer->LoadFrom(data);
            m_MaskLayer->SetInvertPreview(m_InvertMask);
        }
        m_RequireUpdation = true;
    }

    SerializerNode BaseNoiseGenerator::Save()
    {
        auto node = CreateSerializerNode();
        node->Set("UseMask", m_UseMask);
        node->Set("InvertMask", m_InvertMask);
        node->Set("Inspector", m_Inspector->SaveState());
        if (m_MaskLayer != nullptr)
            m_MaskLayer->SaveTo(node);
        return node;
    }

} // namespace tf3d::generators
