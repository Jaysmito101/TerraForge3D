#include "Generators/BaseNoiseGenerator.h"

#include "Data/ApplicationState.h"
#include "Profiler.h"
#include "UI/ImGuiComponents.h"

namespace tf3d::generators
{

    BaseNoiseGenerator::BaseNoiseGenerator(ApplicationState *appState)
        : m_AppState(appState), m_Inspector(std::make_shared<CustomInspector>())
    {
        if (m_AppState == nullptr)
            return;

        m_CalculatedMaskGenerator = std::make_shared<CalculatedMaskGenerator>(m_AppState, "SlopeRamp");
        m_MaskTool                = std::make_shared<MaskTool>(m_AppState, glm::vec3(1.0f, 0.65f, 0.1f));
        m_MaskTool->SetGeneratedMaskTexture(m_CalculatedMaskGenerator->GetTexture(), "Calculated base-noise mask");
        m_MaskTool->SetPreviewMode(MaskPreviewMode::Generated);
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
        if (m_UseMask && m_MaskTool != nullptr && m_CalculatedMaskGenerator != nullptr) {
            if (ImGui::CollapsingHeader("Mask Tool")) {
                markChanged(ImGui::Checkbox("Invert mask", &m_InvertMask));
                m_MaskTool->SetInvertPreview(m_InvertMask);
                m_MaskTool->SetGeneratedMaskTexture(m_CalculatedMaskGenerator->GetTexture(), "Calculated base-noise mask");
                if (m_MaskTool->IsShowingGeneratedMask())
                    markChanged(m_CalculatedMaskGenerator->ShowSettings());
                markChanged(m_MaskTool->ShowSettings(true));
            }
        } else if (m_MaskTool != nullptr) {
            m_MaskTool->SetInvertPreview(false);
            ImGui::TextDisabled("Mask: Global");
        }
        ImGui::PopID();

        return m_RequireUpdation;
    }

    void BaseNoiseGenerator::Resize(int size)
    {
        if (size <= 0)
            return;
        if (m_CalculatedMaskGenerator != nullptr)
            m_CalculatedMaskGenerator->Resize(size);
        if (m_MaskTool != nullptr)
            m_MaskTool->Resize(size);
    }

    void BaseNoiseGenerator::Update(GeneratorData *sourceBuffer, GeneratorData *targetBuffer,
                                    GeneratorTexture *seedTexture, std::string_view profilePrefix)
    {
        if (!m_Shader || sourceBuffer == nullptr || targetBuffer == nullptr)
            return;

        const std::string scopePrefix = profilePrefix.empty() ? "generation" : std::string(profilePrefix);
        const std::string scopeKey    = scopePrefix + "/base-noise";
        TF3D_PROFILE_SCOPE_LAZY_DOMAIN(scopeKey, PerformanceMonitor::Domain::Generation);

        const bool useMask = m_UseMask && m_MaskTool != nullptr && m_CalculatedMaskGenerator != nullptr &&
                             m_MaskTool->GetPreviewTexture() != nullptr;
        if (useMask) {
            m_CalculatedMaskGenerator->Invalidate();
            m_CalculatedMaskGenerator->Update(sourceBuffer);
            m_MaskTool->SetGeneratedMaskTexture(m_CalculatedMaskGenerator->GetTexture(), "Calculated base-noise mask");
        }

        sourceBuffer->Bind(0);
        targetBuffer->Bind(1);

        m_Shader->Bind();
        m_Inspector->ApplyToShader(*m_Shader);
        m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
        m_Shader->SetUniform1i("u_UseSeedTexture", (seedTexture != nullptr && m_Inspector->Get("AutoUseSeedTexture", false)) ? 1 : 0);
        m_Shader->SetUniform1i("u_UseMask", useMask ? 1 : 0);
        m_Shader->SetUniform1i("u_InvertMask", useMask && m_InvertMask ? 1 : 0);
        if (seedTexture) {
            m_Shader->SetUniform1i("u_SeedTexture", seedTexture->Bind(1));
        }
        if (useMask) {
            m_Shader->SetUniform1i("u_MaskTexture", m_MaskTool->GetPreviewTexture()->Bind(3));
        }
        const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
        const auto dispatchSize  = (m_AppState->mainMap.tileResolution + workgroupSize - 1) / workgroupSize;
        const std::string gpuKey = scopeKey + "/gpu";
        TF3D_PROFILE_GPU_SCOPE(gpuKey);
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

        if (m_CalculatedMaskGenerator != nullptr) {
            m_CalculatedMaskGenerator->Load(data->Get<SerializerNode>("CalculatedMask"));
            m_CalculatedMaskGenerator->Invalidate();
        }
        if (m_MaskTool != nullptr) {
            m_MaskTool->SetGeneratedMaskTexture(m_CalculatedMaskGenerator != nullptr ? m_CalculatedMaskGenerator->GetTexture() : nullptr,
                                                "Calculated base-noise mask");
            m_MaskTool->Load(data->Get<SerializerNode>("MaskTool"));
            m_MaskTool->SetInvertPreview(m_InvertMask);
        }
        m_RequireUpdation = true;
    }

    SerializerNode BaseNoiseGenerator::Save()
    {
        auto node = CreateSerializerNode();
        node->Set("UseMask", m_UseMask);
        node->Set("InvertMask", m_InvertMask);
        node->Set("Inspector", m_Inspector->SaveState());
        if (m_CalculatedMaskGenerator != nullptr)
            node->Set("CalculatedMask", m_CalculatedMaskGenerator->Save());
        if (m_MaskTool != nullptr)
            node->Set("MaskTool", m_MaskTool->Save());
        return node;
    }

} // namespace tf3d::generators
