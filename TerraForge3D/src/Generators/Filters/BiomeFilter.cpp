#include "Generators/Filters/BiomeFilter.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"

namespace tf3d::generators
{

    BiomeFilter::BiomeFilter(tf3d::data::ApplicationState *appState,
                             std::shared_ptr<BiomeFilterDefinition> definition)
        : m_AppState(appState),
          m_Definition(std::move(definition)),
          m_Inspector(std::make_shared<inspector::CustomInspector>()),
          m_ID(GenerateId(8)),
          m_UIState(true,
                    false,
                    false,
                    1.0f,
                    m_Definition != nullptr ? m_Definition->GetRuntime().defaultMergeMode : BiomeFilterMergeMode::Blend,
                    m_Inspector->Clone(),
                    MaskLayer::State{BaseMaskGenerator::State{}, MaskTool::State{}}),
          m_State(m_UIState)
    {
        if (m_Definition == nullptr) {
            return;
        }

        m_InspectorReady = m_Definition->BuildInspector(*m_Inspector);
        m_MaskLayer      = std::make_shared<MaskLayer>(m_AppState, glm::vec3(1.0f, 0.0f, 0.0f),
                                                       "None");
        PublishState();
    }

    bool BiomeFilter::ShowSettings()
    {
        bool changed = false;
        if (m_Definition != nullptr && (m_Inspector == nullptr || m_Inspector->GetDescription().empty())) {
            if (!m_Definition->GetDescription().empty()) {
                ImGui::TextWrapped("%s", m_Definition->GetDescription().c_str());
            }
        }
        changed |= ImGui::Checkbox("Enabled", &m_UIState.enabled);
        if (m_InspectorReady) {
            changed |= m_Inspector->Render();
        }
        changed |= ImGui::SliderFloat("Strength", &m_UIState.strength, 0.0f, 1.0f);

        static const char *mergeModes[] = {"Override", "Add", "Subtract", "Multiply", "Blend"};
        int mergeMode                   = static_cast<int>(m_UIState.mergeMode);
        if (ShowComboBox("Merge mode", &mergeMode, mergeModes, IM_ARRAYSIZE(mergeModes))) {
            m_UIState.mergeMode = static_cast<BiomeFilterMergeMode>(mergeMode);
            changed             = true;
        }

        changed |= ImGui::Checkbox("Use mask", &m_UIState.useMask);
        if (m_UIState.useMask) {
            if (ImGui::CollapsingHeader("Mask Tool")) {
                changed |= ImGui::Checkbox("Invert mask", &m_UIState.invertMask);
                if (m_MaskLayer != nullptr) {
                    m_MaskLayer->SetInvertPreview(m_UIState.invertMask);
                    changed |= m_MaskLayer->ShowSettings(true);
                }
            }
        } else if (m_MaskLayer != nullptr) {
            m_MaskLayer->SetInvertPreview(false);
            ImGui::TextDisabled("Mask: Global");
        }

        if (changed) {
            PublishState();
        }
        return changed;
    }

    void BiomeFilter::Resize(int size)
    {
        if (m_MaskLayer != nullptr) {
            m_MaskLayer->Resize(size);
        }
    }

    bool BiomeFilter::UpdateGeneratedMask(const State &state, GeneratorData *sourceData)
    {
        if (!state.useMask || m_MaskLayer == nullptr || sourceData == nullptr) {
            return false;
        }
        return m_MaskLayer->Apply(state.mask, sourceData) && m_MaskLayer->GetTexture() != nullptr;
    }

    BiomeFilter::State BiomeFilter::CaptureState() const
    {
        State state  = m_UIState;
        state.values = m_Inspector->Clone();
        if (m_MaskLayer != nullptr) {
            state.mask = m_MaskLayer->GetState().value;
        }
        return state;
    }

    void BiomeFilter::PublishState()
    {
        m_UIState = CaptureState();
        m_State.Replace(m_UIState);
    }

    BiomeFilter::Snapshot BiomeFilter::GetState() const
    {
        return m_State.Capture();
    }

    void BiomeFilter::Load(SerializerNode data)
    {
        if (data == nullptr) {
            return;
        }
        m_ID                 = data->Get<std::string>("ID", m_ID);
        m_UIState.enabled    = data->Get<bool>("Enabled", m_UIState.enabled);
        m_UIState.useMask    = data->Get<bool>("UseMask", m_UIState.useMask);
        m_UIState.invertMask = data->Get<bool>("InvertMask", m_UIState.invertMask);
        m_UIState.strength   = data->Get<float>("Strength", m_UIState.strength);
        m_UIState.mergeMode  = static_cast<BiomeFilterMergeMode>(glm::clamp(data->Get<int>("MergeMode", static_cast<int>(m_UIState.mergeMode)), 0, 4));
        if (m_Inspector != nullptr) {
            auto parameters = data->Get<SerializerNode>("Parameters");
            if (parameters != nullptr) {
                m_Inspector->LoadState(parameters);
            }
        }
        if (m_MaskLayer != nullptr) {
            m_MaskLayer->LoadFrom(data);
        }
        PublishState();
    }

    SerializerNode BiomeFilter::Save() const
    {
        auto node = CreateSerializerNode();
        node->Set("ID", m_ID);
        node->Set("DefinitionID", GetDefinitionID());
        node->Set("Enabled", m_UIState.enabled);
        node->Set("UseMask", m_UIState.useMask);
        node->Set("InvertMask", m_UIState.invertMask);
        node->Set("Strength", m_UIState.strength);
        node->Set("MergeMode", static_cast<int>(m_UIState.mergeMode));
        if (m_Inspector != nullptr) {
            node->Set("Parameters", m_Inspector->SaveState());
        }
        if (m_MaskLayer != nullptr) {
            m_MaskLayer->SaveTo(node);
        }
        return node;
    }

} // namespace tf3d::generators
