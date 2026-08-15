#include "Generators/BaseNoiseGenerator.h"

#include "Data/ApplicationState.h"
#include "Profiler.h"
#include "UI/ImGuiComponents.h"

#include <algorithm>

namespace tf3d::generators
{

    BaseNoiseGenerator::BaseNoiseGenerator(tf3d::data::ApplicationState *appState)
        : m_AppState(appState),
          m_Inspector(std::make_shared<inspector::CustomInspector>()),
          m_UIState(State{true,
                          true,
                          false,
                          m_Inspector->Clone(),
                          MaskLayer::State{BaseMaskGenerator::State{}, MaskTool::State{}}}),
          m_State(m_UIState)
    {
        if (m_AppState == nullptr) {
            return;
        }

        m_MaskLayer = std::make_shared<MaskLayer>(m_AppState, glm::vec3(1.0f, 0.65f, 0.1f),
                                                  "SlopeRamp");
        m_UIState   = CaptureState();
        m_State.Replace(m_UIState);
    }

    bool BaseNoiseGenerator::Initialize()
    {
        if (m_AppState == nullptr || m_AppState->resourceManager == nullptr) {
            TF3D_LOG_ERROR("Failed to initialize base-noise generator: application resources are unavailable.");
            return false;
        }

        if (!m_Inspector->LoadConfig(m_AppState, "BaseNoise")) {
            return false;
        }

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

        PublishState();
        return true;
    }

    bool BaseNoiseGenerator::ShowEnabledControl(bool showRecommendation)
    {
        const bool changed = ImGui::Checkbox("Enable Base Noise##BaseNoiseGenerator", &m_UIState.enabled);
        if (changed) {
            PublishState();
        }
        if (showRecommendation && m_UIState.enabled) {
            ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.25f, 1.0f),
                               "Recommendation: disable Base Noise when using Global Elevation.");
        }
        return RequireUpdation();
    }

    bool BaseNoiseGenerator::ShowSettings()
    {
        ImGui::PushID("BaseNoiseGenerator");
        bool changed = false;
        changed |= ImGui::Checkbox("Enable Base Noise##BaseNoiseGenerator", &m_UIState.enabled);
        changed |= m_Inspector->Render();

        changed |= ImGui::Checkbox("Use mask", &m_UIState.useMask);
        if (m_UIState.useMask && m_MaskLayer != nullptr) {
            if (ImGui::CollapsingHeader("Mask Tool")) {
                changed |= ImGui::Checkbox("Invert mask", &m_UIState.invertMask);
                m_MaskLayer->SetInvertPreview(m_UIState.invertMask);
                changed |= m_MaskLayer->ShowSettings(true);
            }
        } else if (m_MaskLayer != nullptr) {
            m_MaskLayer->SetInvertPreview(false);
            ImGui::TextDisabled("Mask: Global");
        }
        ImGui::PopID();

        if (changed) {
            PublishState();
        }
        return RequireUpdation();
    }

    void BaseNoiseGenerator::Resize(int size)
    {
        if (size <= 0) {
            return;
        }
        if (m_MaskLayer != nullptr) {
            m_MaskLayer->Resize(size);
        }
    }

    BaseNoiseGenerator::State BaseNoiseGenerator::CaptureState() const
    {
        State state  = m_UIState;
        state.values = m_Inspector->Clone();
        if (m_MaskLayer != nullptr) {
            state.mask = m_MaskLayer->GetState().value;
        }
        return state;
    }

    void BaseNoiseGenerator::PublishState()
    {
        m_UIState = CaptureState();
        m_State.Replace(m_UIState);
    }

    void BaseNoiseGenerator::Update(const Snapshot *state,
                                    const GenerationContext *context,
                                    GeneratorData *sourceBuffer,
                                    GeneratorData *targetBuffer)
    {
        if (state == nullptr || context == nullptr || !m_Shader || m_AppState == nullptr ||
            sourceBuffer == nullptr || targetBuffer == nullptr) {
            return;
        }

        if (!state->value.enabled) {
            sourceBuffer->CopyTo(targetBuffer);
            m_State.MarkProcessed(state->revision);
            return;
        }

        TF3D_PROFILE_SCOPE_CHILD_LAZY("base-noise");

        bool useMask = state->value.useMask && m_MaskLayer != nullptr;
        if (useMask) {
            useMask = m_MaskLayer->Apply(state->value.mask, sourceBuffer) &&
                      m_MaskLayer->GetTexture() != nullptr;
        }

        sourceBuffer->Bind(0);
        targetBuffer->Bind(1);

        m_Shader->Bind();
        m_Inspector->ApplyToShader(state->value.values, *m_Shader);
        m_Shader->SetUniform1i("u_Resolution", context->tileResolution);
        const auto inspectorRoot   = state->value.values.Root();
        const auto octaveStrengths = inspectorRoot.Scope("Octaves").Get("OctaveStrengths", std::vector<float>{});
        m_Shader->SetUniform1i("u_NoiseOctaveStrengthsCount",
                               std::clamp(static_cast<int>(octaveStrengths.size()), 0, 16));
        m_Shader->SetUniform1i("u_UseSeedTexture",
                               (context->seedTexture != nullptr &&
                                inspectorRoot.Scope("Blend").Get("AutoUseSeedTexture", false))
                                   ? 1
                                   : 0);
        m_Shader->SetUniform1i("u_UseMask", useMask ? 1 : 0);
        m_Shader->SetUniform1i("u_InvertMask", useMask && state->value.invertMask ? 1 : 0);
        if (context->seedTexture != nullptr) {
            m_Shader->SetUniform1i("u_SeedTexture", context->seedTexture->Bind(1));
        }
        if (useMask) {
            m_Shader->SetUniform1i("u_MaskTexture", m_MaskLayer->GetTexture()->Bind(3));
        }
        const int32_t workgroupSize = context->gpuWorkgroupSize > 0 ? context->gpuWorkgroupSize : 1;
        const auto dispatchSize     = (context->tileResolution + workgroupSize - 1) / workgroupSize;
        TF3D_PROFILE_GPU_SCOPE_CHILD("gpu");
        TF3D_PROFILE_VALUE_DOMAIN("generation/base-noise/dispatch", dispatchSize, dispatchSize, 1,
                                  PerformanceMonitor::Domain::Generation);
        m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
        m_Shader->SetMemoryBarrier();

        m_State.MarkProcessed(state->revision);
    }

    void BaseNoiseGenerator::Load(SerializerNode data)
    {
        if (data == nullptr) {
            return;
        }

        m_UIState.enabled    = data->Get<bool>("Enabled", m_UIState.enabled);
        m_UIState.useMask    = data->Get<bool>("UseMask", m_UIState.useMask);
        m_UIState.invertMask = data->Get<bool>("InvertMask", m_UIState.invertMask);

        if (const auto inspector = data->Get<SerializerNode>("Inspector"); inspector != nullptr) {
            m_Inspector->LoadState(inspector);
        }

        if (m_MaskLayer != nullptr) {
            m_MaskLayer->LoadFrom(data);
            m_MaskLayer->SetInvertPreview(m_UIState.invertMask);
        }
        PublishState();
    }

    SerializerNode BaseNoiseGenerator::Save()
    {
        auto node = CreateSerializerNode();
        node->Set("Enabled", m_UIState.enabled);
        node->Set("UseMask", m_UIState.useMask);
        node->Set("InvertMask", m_UIState.invertMask);
        node->Set("Inspector", m_Inspector->SaveState());
        if (m_MaskLayer != nullptr) {
            m_MaskLayer->SaveTo(node);
        }
        return node;
    }

} // namespace tf3d::generators
