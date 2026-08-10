#include "Generators/BiomeCustomBaseShape.h"

#include "Data/ApplicationState.h"
#include "Profiler.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"

#include <algorithm>

namespace tf3d::generators
{

    BiomeCustomizeBaseShape::BiomeCustomizeBaseShape(tf3d::data::ApplicationState *appState)
        : m_AppState(appState)
    {
        m_WorkingDataBuffer = std::make_shared<GeneratorData>();
        m_SwapBuffer        = std::make_shared<GeneratorData>();
        m_Shader            = m_AppState->resourceManager->LoadComputeShader(
            "generation/customize_base_shape/customize_base_shape");
        m_Masks.push_back(CreateMaskLayer("Mask 1"));
    }

    BiomeCustomizeBaseShape::~BiomeCustomizeBaseShape() = default;

    BiomeCustomizeBaseShape::MaskEntry BiomeCustomizeBaseShape::CreateMaskLayer(const std::string &name) const
    {
        MaskEntry layer;
        layer.name = name;
        layer.mask = std::make_shared<MaskLayer>(m_AppState, glm::vec3(1.0f, 0.45f, 0.05f),
                                                 "None");
        return layer;
    }

    void BiomeCustomizeBaseShape::AddMaskLayer()
    {
        m_Masks.push_back(CreateMaskLayer("Mask " + std::to_string(m_Masks.size() + 1)));
        m_SelectedMask    = static_cast<int>(m_Masks.size()) - 1;
        m_RequireUpdation = true;
    }

    bool BiomeCustomizeBaseShape::ShowSettings()
    {
        bool changed = false;
        changed |= ImGui::Checkbox("Enabled", &m_Enabled);
        changed |= ImGui::Checkbox("Flatten original base shape", &m_FlattenBaseShape);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Start customization from zero instead of the generated base-shape result.");

        ImGui::TextDisabled("The selected base shape runs first; these layers run before base noise and filters.");
        if (ImGui::BeginTabBar("Customize Base Shape Sections")) {
            if (ImGui::BeginTabItem("Drawing")) {
                changed |= ShowDrawingSettings();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        m_RequireUpdation |= changed;
        return changed;
    }

    bool BiomeCustomizeBaseShape::ShowDrawingSettings()
    {
        bool changed = false;
        if (ImGui::Button("Add mask")) {
            AddMaskLayer();
            changed = true;
        }

        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextDisabled("Each mask is evaluated from the terrain before its layer.");
        ImGui::PopTextWrapPos();

        int removeIndex            = -1;
        const ImGuiStyle &style    = ImGui::GetStyle();
        const float rowHeight      = ImGui::GetFrameHeightWithSpacing();
        const int rowCount         = std::max(1, static_cast<int>(m_Masks.size()));
        const float listHeight     = std::min(rowHeight * 6.0f, rowHeight * static_cast<float>(rowCount)) + style.WindowPadding.y * 2.0f;
        const bool maskListVisible = ImGui::BeginChild("Customize Base Shape Mask List", ImVec2(0.0f, listHeight), true);
        if (maskListVisible) {
            for (int maskIndex = 0; maskIndex < static_cast<int>(m_Masks.size()); ++maskIndex) {
                auto &layer = m_Masks[static_cast<size_t>(maskIndex)];
                ImGui::PushID(maskIndex);
                const std::string label = layer.name + (layer.enabled ? "" : " (disabled)");

                const float removeWidth     = ImGui::CalcTextSize("Remove").x + style.FramePadding.x * 2.0f;
                const float selectableWidth = std::max(
                    0.0f, ImGui::GetContentRegionAvail().x - removeWidth - style.ItemSpacing.x);
                if (ImGui::Selectable(label.c_str(), m_SelectedMask == maskIndex, 0,
                                      ImVec2(selectableWidth, ImGui::GetFrameHeight())))
                    m_SelectedMask = maskIndex;
                ImGui::SameLine();
                ImGui::BeginDisabled(m_Masks.size() <= 1);
                if (ImGui::SmallButton("Remove"))
                    removeIndex = maskIndex;
                ImGui::EndDisabled();
                ImGui::PopID();
                if (removeIndex >= 0)
                    break;
            }
            if (m_Masks.empty())
                ImGui::TextDisabled("No masks. Add one to begin sculpting.");
        }
        ImGui::EndChild();

        if (removeIndex >= 0 && removeIndex < static_cast<int>(m_Masks.size())) {
            m_Masks.erase(m_Masks.begin() + removeIndex);
            if (m_SelectedMask > removeIndex)
                --m_SelectedMask;
            m_SelectedMask = glm::clamp(m_SelectedMask, 0, static_cast<int>(m_Masks.size()) - 1);
            changed        = true;
        }

        if (m_Masks.empty())
            return changed;

        m_SelectedMask = glm::clamp(m_SelectedMask, 0, static_cast<int>(m_Masks.size()) - 1);
        auto &layer    = m_Masks[static_cast<size_t>(m_SelectedMask)];
        ImGui::SeparatorText("Selected mask");
        ImGui::PushID("Customize Base Shape Selected Mask");
        changed |= ImGui::Checkbox("Layer enabled", &layer.enabled);

        static const char *directions[] = {"Push up", "Push down"};
        int direction                   = layer.raise ? 0 : 1;
        if (ShowComboBox("Direction", &direction, directions, IM_ARRAYSIZE(directions))) {
            layer.raise = direction == 0;
            changed     = true;
        }
        changed |= ImGui::SliderFloat("Strength", &layer.strength, 0.0f, 4.0f);
        changed |= ImGui::SliderFloat("Smoothing", &layer.smoothing, 0.0f, 1.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Smooths the mask influence before applying the height offset.");

        if (layer.mask != nullptr)
            changed |= layer.mask->ShowSettings(true);
        ImGui::PopID();
        return changed;
    }

    bool BiomeCustomizeBaseShape::UpdateLayerMask(MaskEntry &layer, GeneratorData *source)
    {
        if (layer.mask == nullptr || source == nullptr)
            return false;

        layer.mask->Update(source);
        return layer.mask->GetTexture() != nullptr;
    }

    bool BiomeCustomizeBaseShape::ApplyLayer(GeneratorData *source, GeneratorData *target,
                                             const MaskEntry *layer, bool flattenSource,
                                             std::string_view profilePrefix)
    {
        if (!m_Shader || source == nullptr || target == nullptr)
            return false;

        const std::string scopePrefix = profilePrefix.empty() ? "generation" : std::string(profilePrefix);
        const std::string scopeKey    = scopePrefix + "/customize-base-shape";
        TF3D_PROFILE_SCOPE_DOMAIN(scopeKey, PerformanceMonitor::Domain::Generation);

        source->Bind(0);
        target->Bind(1);
        m_Shader->Bind();
        m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
        m_Shader->SetUniform1i("u_UseMask", layer != nullptr ? 1 : 0);
        m_Shader->SetUniform1i("u_FlattenSource", flattenSource ? 1 : 0);
        m_Shader->SetUniform1i("u_Direction", layer != nullptr && layer->raise ? 1 : -1);
        m_Shader->SetUniform1f("u_Strength", layer != nullptr ? glm::max(layer->strength, 0.0f) : 0.0f);
        m_Shader->SetUniform1f("u_Smoothing", layer != nullptr ? glm::clamp(layer->smoothing, 0.0f, 1.0f) : 0.0f);
        if (layer != nullptr && layer->mask != nullptr) {
            if (auto *maskTexture = layer->mask->GetTexture())
                m_Shader->SetUniform1i("u_MaskTexture", maskTexture->Bind(2));
        }

        const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
        const auto resolution    = m_AppState->mainMap.tileResolution;
        const auto dispatchSize  = (resolution + workgroupSize - 1) / workgroupSize;
        const std::string gpuKey = scopeKey + "/gpu";
        TF3D_PROFILE_GPU_SCOPE(gpuKey);
        TF3D_PROFILE_VALUE_DOMAIN("generation/customize-base-shape/dispatch", dispatchSize, dispatchSize, 1,
                                  PerformanceMonitor::Domain::Generation);
        m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
        m_Shader->SetMemoryBarrier();
        return true;
    }

    void BiomeCustomizeBaseShape::Update(GeneratorData *baseShapeBuffer, GeneratorData *targetBuffer,
                                         std::string_view profilePrefix)
    {
        if (baseShapeBuffer == nullptr || targetBuffer == nullptr)
            return;

        GeneratorData *current = baseShapeBuffer;
        bool hasOutput         = false;

        if (m_FlattenBaseShape) {
            if (!ApplyLayer(baseShapeBuffer, m_WorkingDataBuffer.get(), nullptr, true, profilePrefix)) {
                baseShapeBuffer->CopyTo(targetBuffer);
                return;
            }
            current   = m_WorkingDataBuffer.get();
            hasOutput = true;
        }

        for (auto &layer : m_Masks) {
            if (!layer.enabled)
                continue;
            if (!UpdateLayerMask(layer, current))
                continue;

            GeneratorData *target = current == m_WorkingDataBuffer.get()
                                        ? m_SwapBuffer.get()
                                        : m_WorkingDataBuffer.get();
            if (!ApplyLayer(current, target, &layer, false, profilePrefix)) {
                baseShapeBuffer->CopyTo(targetBuffer);
                return;
            }
            current   = target;
            hasOutput = true;
        }

        if (!hasOutput)
            baseShapeBuffer->CopyTo(targetBuffer);
        else if (current != targetBuffer)
            current->CopyTo(targetBuffer);

        m_RequireUpdation = false;
    }

    SerializerNode BiomeCustomizeBaseShape::Save() const
    {
        auto node = CreateSerializerNode();
        node->Set("Enabled", m_Enabled);
        node->Set("FlattenBaseShape", m_FlattenBaseShape);

        std::vector<SerializerNode> masks;
        masks.reserve(m_Masks.size());
        for (const auto &layer : m_Masks) {
            auto mask = CreateSerializerNode();
            mask->Set("Name", layer.name);
            mask->Set("Enabled", layer.enabled);
            mask->Set("Raise", layer.raise);
            mask->Set("Strength", layer.strength);
            mask->Set("Smoothing", layer.smoothing);
            if (layer.mask != nullptr)
                layer.mask->SaveTo(mask);
            masks.push_back(std::move(mask));
        }
        node->Set("Masks", masks);
        return node;
    }

    void BiomeCustomizeBaseShape::Load(SerializerNode node)
    {
        if (node == nullptr)
            return;

        m_Enabled          = node->Get<bool>("Enabled", m_Enabled);
        m_FlattenBaseShape = node->Get<bool>("FlattenBaseShape", m_FlattenBaseShape);
        if (node->HasKey("Masks")) {
            m_Masks.clear();
            for (const auto &maskNode : node->Get<std::vector<SerializerNode>>("Masks")) {
                if (maskNode == nullptr)
                    continue;
                auto layer      = CreateMaskLayer(maskNode->Get<std::string>("Name", "Mask"));
                layer.enabled   = maskNode->Get<bool>("Enabled", layer.enabled);
                layer.raise     = maskNode->Get<bool>("Raise", layer.raise);
                layer.strength  = maskNode->Get<float>("Strength", layer.strength);
                layer.smoothing = maskNode->Get<float>("Smoothing", layer.smoothing);
                if (layer.mask != nullptr)
                    layer.mask->LoadFrom(maskNode);
                m_Masks.push_back(std::move(layer));
            }
        }
        m_SelectedMask    = glm::clamp(m_SelectedMask, 0, std::max(0, static_cast<int>(m_Masks.size()) - 1));
        m_RequireUpdation = true;
    }

    void BiomeCustomizeBaseShape::Resize()
    {
        const auto size = m_AppState->mainMap.tileResolution * m_AppState->mainMap.tileResolution * sizeof(float);
        m_WorkingDataBuffer->Resize(size);
        m_SwapBuffer->Resize(size);
        for (auto &layer : m_Masks) {
            if (layer.mask != nullptr)
                layer.mask->Resize(m_AppState->mainMap.tileResolution);
        }
        m_RequireUpdation = true;
    }

} // namespace tf3d::generators
