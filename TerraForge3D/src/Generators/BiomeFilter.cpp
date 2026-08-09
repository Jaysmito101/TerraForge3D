#include "Generators/BiomeFilter.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"

namespace tf3d::generators
{

    BiomeFilterMergeMode MergeModeFromString(const std::string &value)
    {
        if (value == "Override")
            return BiomeFilterMergeMode::Override;
        if (value == "Add")
            return BiomeFilterMergeMode::Add;
        if (value == "Subtract")
            return BiomeFilterMergeMode::Subtract;
        if (value == "Multiply")
            return BiomeFilterMergeMode::Multiply;
        return BiomeFilterMergeMode::Blend;
    }

    BiomeFilter::BiomeFilter(ApplicationState *appState, std::shared_ptr<BiomeFilterDefinition> definition)
        : m_AppState(appState), m_Definition(std::move(definition)), m_Inspector(std::make_shared<CustomInspector>()), m_ID(GenerateId(8))
    {
        if (m_Definition == nullptr)
            return;
        m_MergeMode = MergeModeFromString(m_Definition->GetMetadata().value("DefaultMergeMode", "Blend"));
        m_Definition->BuildInspector(*m_Inspector);
        m_MaskLayer = std::make_shared<MaskLayer>(m_AppState, glm::vec3(1.0f, 0.0f, 0.0f),
                                                  "HeightRange", "Calculated filter mask");
    }

    bool BiomeFilter::ShowSettings()
    {
        bool changed = false;
        if (m_Inspector == nullptr || m_Inspector->GetDescription().empty()) {
            if (!m_Definition->GetDescription().empty())
                ImGui::TextWrapped("%s", m_Definition->GetDescription().c_str());
        }
        changed |= ImGui::Checkbox("Enabled", &m_Enabled);
        if (m_Inspector != nullptr &&
            (m_Definition->GetMetadata().contains("Params") ||
             m_Definition->GetMetadata().contains("Sections")))
            changed |= m_Inspector->Render();
        changed |= ImGui::SliderFloat("Strength", &m_Strength, 0.0f, 1.0f);

        static const char *mergeModes[] = {"Override", "Add", "Subtract", "Multiply", "Blend"};
        int mergeMode                   = static_cast<int>(m_MergeMode);
        if (ShowComboBox("Merge mode", &mergeMode, mergeModes, IM_ARRAYSIZE(mergeModes))) {
            m_MergeMode = static_cast<BiomeFilterMergeMode>(mergeMode);
            changed     = true;
        }

        changed |= ImGui::Checkbox("Use mask", &m_UseMask);
        if (m_UseMask) {
            if (ImGui::CollapsingHeader("Mask Tool")) {
                changed |= ImGui::Checkbox("Invert mask", &m_InvertMask);
                m_MaskLayer->SetInvertPreview(m_InvertMask);
                changed |= m_MaskLayer->ShowSettings(true);
            }
        } else {
            m_MaskLayer->SetInvertPreview(false);
            ImGui::TextDisabled("Mask: Global");
        }

        return changed;
    }

    void BiomeFilter::Resize(int size)
    {
        m_MaskLayer->Resize(size);
    }

    void BiomeFilter::UpdateGeneratedMask(GeneratorData *sourceData)
    {
        if (!m_UseMask)
            return;
        m_MaskLayer->Update(sourceData);
    }

    int BiomeFilter::GetIntegerParameter(const std::string &name, int defaultValue) const
    {
        if (m_Inspector == nullptr)
            return defaultValue;
        return m_Inspector->Root().Get(name, defaultValue);
    }

    float BiomeFilter::GetFloatParameter(const std::string &name, float defaultValue) const
    {
        if (m_Inspector == nullptr)
            return defaultValue;
        return m_Inspector->Root().Get(name, defaultValue);
    }

    void BiomeFilter::Load(SerializerNode data)
    {
        if (data == nullptr)
            return;
        m_ID         = data->Get<std::string>("ID", m_ID);
        m_Enabled    = data->Get<bool>("Enabled", m_Enabled);
        m_UseMask    = data->Get<bool>("UseMask", m_UseMask);
        m_InvertMask = data->Get<bool>("InvertMask", m_InvertMask);
        m_Strength   = data->Get<float>("Strength", m_Strength);
        m_MergeMode  = static_cast<BiomeFilterMergeMode>(glm::clamp(data->Get<int>("MergeMode", static_cast<int>(m_MergeMode)), 0, 4));
        if (m_Inspector != nullptr) {
            auto parameters = data->Get<SerializerNode>("Parameters");
            if (parameters != nullptr)
                m_Inspector->LoadState(parameters);
        }
        if (m_MaskLayer != nullptr)
            m_MaskLayer->LoadFrom(data);
    }

    SerializerNode BiomeFilter::Save() const
    {
        auto node = CreateSerializerNode();
        node->Set("ID", m_ID);
        node->Set("DefinitionID", GetDefinitionID());
        node->Set("Enabled", m_Enabled);
        node->Set("UseMask", m_UseMask);
        node->Set("InvertMask", m_InvertMask);
        node->Set("Strength", m_Strength);
        node->Set("MergeMode", static_cast<int>(m_MergeMode));
        if (m_Inspector != nullptr)
            node->Set("Parameters", m_Inspector->SaveState());
        if (m_MaskLayer != nullptr)
            m_MaskLayer->SaveTo(node);
        return node;
    }

} // namespace tf3d::generators
