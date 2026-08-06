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
        m_CalculatedMaskGenerator = std::make_shared<CalculatedMaskGenerator>(m_AppState);
        m_MaskTool                = std::make_shared<MaskTool>(m_AppState, glm::vec3(1.0f, 0.0f, 0.0f));
        m_MaskTool->SetGeneratedMaskTexture(m_CalculatedMaskGenerator->GetTexture(), "Calculated filter mask");
    }

    bool BiomeFilter::ShowSettings()
    {
        bool changed = false;
        if (m_Inspector == nullptr || m_Inspector->GetDescription().empty()) {
            if (!m_Definition->GetDescription().empty())
                ImGui::TextWrapped("%s", m_Definition->GetDescription().c_str());
        }
        changed |= ImGui::Checkbox("Enabled", &m_Enabled);
        if (m_Inspector != nullptr && m_Definition->GetMetadata().contains("Params"))
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
                m_MaskTool->SetInvertPreview(m_InvertMask);
                m_MaskTool->SetGeneratedMaskTexture(m_CalculatedMaskGenerator->GetTexture(), "Calculated filter mask");
                if (m_MaskTool->IsShowingGeneratedMask())
                    changed |= m_CalculatedMaskGenerator->ShowSettings();
                changed |= m_MaskTool->ShowSettings(true);
            }
        } else {
            m_MaskTool->SetInvertPreview(false);
            ImGui::TextDisabled("Mask: Global");
        }

        return changed;
    }

    void BiomeFilter::Resize(int size)
    {
        m_CalculatedMaskGenerator->Resize(size);
        m_MaskTool->Resize(size);
    }

    void BiomeFilter::UpdateGeneratedMask(GeneratorData *sourceData)
    {
        if (!m_UseMask)
            return;
        m_CalculatedMaskGenerator->Invalidate();
        m_CalculatedMaskGenerator->Update(sourceData);
        m_MaskTool->SetGeneratedMaskTexture(m_CalculatedMaskGenerator->GetTexture(), "Calculated filter mask");
    }

    int BiomeFilter::GetIntegerParameter(const std::string &name, int defaultValue) const
    {
        if (m_Inspector == nullptr || !m_Inspector->HasVariable(name))
            return defaultValue;
        return m_Inspector->GetVariable(name).GetInt();
    }

    float BiomeFilter::GetFloatParameter(const std::string &name, float defaultValue) const
    {
        if (m_Inspector == nullptr || !m_Inspector->HasVariable(name))
            return defaultValue;
        return m_Inspector->GetVariable(name).GetFloat();
    }

    void BiomeFilter::Load(SerializerNode data)
    {
        if (data == nullptr)
            return;
        m_ID         = data->GetString("ID", m_ID);
        m_Enabled    = data->GetInteger("Enabled", m_Enabled ? 1 : 0) != 0;
        m_UseMask    = data->GetInteger("UseMask", m_UseMask ? 1 : 0) != 0;
        m_InvertMask = data->GetInteger("InvertMask", m_InvertMask ? 1 : 0) != 0;
        m_Strength   = data->GetFloat("Strength", m_Strength);
        m_MergeMode  = static_cast<BiomeFilterMergeMode>(glm::clamp(data->GetInteger("MergeMode", static_cast<int>(m_MergeMode)), 0, 4));
        if (m_Inspector != nullptr) {
            auto parameters = data->GetChildNode("Parameters");
            if (parameters != nullptr)
                m_Inspector->LoadData(parameters);
        }
        if (m_CalculatedMaskGenerator != nullptr)
            m_CalculatedMaskGenerator->Load(data->GetChildNode("CalculatedMask"));
        if (m_MaskTool != nullptr)
            m_MaskTool->Load(data->GetChildNode("MaskTool"));
    }

    SerializerNode BiomeFilter::Save() const
    {
        auto node = CreateSerializerNode();
        node->SetString("ID", m_ID);
        node->SetString("DefinitionID", GetDefinitionID());
        node->SetInteger("Enabled", m_Enabled ? 1 : 0);
        node->SetInteger("UseMask", m_UseMask ? 1 : 0);
        node->SetInteger("InvertMask", m_InvertMask ? 1 : 0);
        node->SetFloat("Strength", m_Strength);
        node->SetInteger("MergeMode", static_cast<int>(m_MergeMode));
        if (m_Inspector != nullptr)
            node->SetChildNode("Parameters", m_Inspector->SaveData());
        if (m_CalculatedMaskGenerator != nullptr)
            node->SetChildNode("CalculatedMask", m_CalculatedMaskGenerator->Save());
        if (m_MaskTool != nullptr)
            node->SetChildNode("MaskTool", m_MaskTool->Save());
        return node;
    }

} // namespace tf3d::generators
