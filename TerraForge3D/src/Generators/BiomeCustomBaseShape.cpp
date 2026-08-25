#include "Generators/BiomeCustomBaseShape.h"

#include "Data/ApplicationState.h"
#include "Profiler.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"

#include <algorithm>

namespace tf3d::generators
{

    BiomeCustomizeBaseShape::MaskState::MaskState(std::string layerName,
                                                  MaskLayer::State state)
        : name(std::move(layerName)),
          maskState(std::move(state))
    {
    }

    BiomeCustomizeBaseShape::BiomeCustomizeBaseShape(tf3d::data::ApplicationState *appState)
        : m_AppState(appState),
          m_State(m_UIState)
    {
        m_UIState.masks.push_back(CreateMaskState("Mask 1"));
        m_State.Replace(m_UIState);
        m_Shader = m_AppState->resourceManager->LoadComputeShader(
            "generation/customize_base_shape/customize_base_shape");
    }

    BiomeCustomizeBaseShape::~BiomeCustomizeBaseShape() = default;

    BiomeCustomizeBaseShape::Snapshot BiomeCustomizeBaseShape::GetState() const
    {
        auto snapshot  = m_State.Capture();
        auto runtime   = std::make_shared<RuntimeState>();
        runtime->masks = m_RuntimeMasks;

        Snapshot result;
        result.value    = std::move(snapshot.value);
        result.revision = snapshot.revision;
        result.runtime  = std::move(runtime);
        return result;
    }

    std::shared_ptr<MaskLayer> BiomeCustomizeBaseShape::CreateMaskLayer() const
    {
        return std::make_shared<MaskLayer>(m_AppState,
                                           glm::vec3(1.0f, 0.45f, 0.05f),
                                           "None",
                                           true);
    }

    BiomeCustomizeBaseShape::MaskState BiomeCustomizeBaseShape::CreateMaskState(const std::string &name)
    {
        auto maskLayer = CreateMaskLayer();
        auto snapshot  = maskLayer->GetState();
        m_RuntimeMasks.push_back(maskLayer);
        return MaskState{name, std::move(snapshot.value)};
    }

    void BiomeCustomizeBaseShape::AddMaskLayer()
    {
        m_UIState.masks.push_back(CreateMaskState("Mask " + std::to_string(m_UIState.masks.size() + 1)));
        m_SelectedMask = static_cast<int>(m_UIState.masks.size()) - 1;
    }

    bool BiomeCustomizeBaseShape::ShowSettings()
    {
        State &settings = m_UIState;
        bool changed    = false;
        changed |= ImGui::Checkbox("Enabled", &settings.enabled);
        changed |= ImGui::Checkbox("Flatten original base shape", &settings.flattenBaseShape);
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

        if (changed) {
            m_UIState = CaptureState();
            m_State.Replace(m_UIState);
        }
        return RequireUpdation();
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
        const int rowCount         = std::max(1, static_cast<int>(m_UIState.masks.size()));
        const float listHeight     = std::min(rowHeight * 6.0f, rowHeight * static_cast<float>(rowCount)) + style.WindowPadding.y * 2.0f;
        const bool maskListVisible = ImGui::BeginChild("Customize Base Shape Mask List", ImVec2(0.0f, listHeight), true);
        if (maskListVisible) {
            for (int maskIndex = 0; maskIndex < static_cast<int>(m_UIState.masks.size()); ++maskIndex) {
                auto &layer = m_UIState.masks[static_cast<size_t>(maskIndex)];
                ImGui::PushID(maskIndex);
                const std::string label = layer.name + (layer.enabled ? "" : " (disabled)");

                const float removeWidth     = ImGui::CalcTextSize("Remove").x + style.FramePadding.x * 2.0f;
                const float selectableWidth = std::max(
                    0.0f, ImGui::GetContentRegionAvail().x - removeWidth - style.ItemSpacing.x);
                if (ImGui::Selectable(label.c_str(), m_SelectedMask == maskIndex, 0,
                                      ImVec2(selectableWidth, ImGui::GetFrameHeight())))
                    m_SelectedMask = maskIndex;
                ImGui::SameLine();
                ImGui::BeginDisabled(m_UIState.masks.size() <= 1);
                if (ImGui::SmallButton("Remove"))
                    removeIndex = maskIndex;
                ImGui::EndDisabled();
                ImGui::PopID();
                if (removeIndex >= 0)
                    break;
            }
            if (m_UIState.masks.empty())
                ImGui::TextDisabled("No masks. Add one to begin sculpting.");
        }
        ImGui::EndChild();

        if (removeIndex >= 0 && removeIndex < static_cast<int>(m_UIState.masks.size())) {
            m_UIState.masks.erase(m_UIState.masks.begin() + removeIndex);
            if (removeIndex < static_cast<int>(m_RuntimeMasks.size())) {
                m_RuntimeMasks.erase(m_RuntimeMasks.begin() + removeIndex);
            }
            if (m_SelectedMask > removeIndex)
                --m_SelectedMask;
            m_SelectedMask = glm::clamp(m_SelectedMask, 0, static_cast<int>(m_UIState.masks.size()) - 1);
            changed        = true;
        }

        if (m_UIState.masks.empty())
            return changed;

        m_SelectedMask = glm::clamp(m_SelectedMask, 0, static_cast<int>(m_UIState.masks.size()) - 1);
        auto &layer    = m_UIState.masks[static_cast<size_t>(m_SelectedMask)];
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

        if (m_SelectedMask < static_cast<int>(m_RuntimeMasks.size()) &&
            m_RuntimeMasks[static_cast<size_t>(m_SelectedMask)] != nullptr) {
            changed |= m_RuntimeMasks[static_cast<size_t>(m_SelectedMask)]->ShowSettings(true);
        }
        ImGui::PopID();
        return changed;
    }

    BiomeCustomizeBaseShape::State BiomeCustomizeBaseShape::CaptureState() const
    {
        State state            = m_UIState;
        const size_t maskCount = std::min(state.masks.size(), m_RuntimeMasks.size());
        for (size_t maskIndex = 0; maskIndex < maskCount; ++maskIndex) {
            if (m_RuntimeMasks[maskIndex] != nullptr) {
                state.masks[maskIndex].maskState = m_RuntimeMasks[maskIndex]->GetState().value;
            }
        }
        return state;
    }

    bool BiomeCustomizeBaseShape::UpdateLayerMask(const MaskState &layer,
                                                  const GenerationContext *context,
                                                  const std::shared_ptr<MaskLayer> &maskLayer,
                                                  GeneratorData *source)
    {
        if (maskLayer == nullptr || source == nullptr) {
            return false;
        }

        return maskLayer->Apply(layer.maskState, context, source) && maskLayer->GetTexture() != nullptr;
    }

    bool BiomeCustomizeBaseShape::ApplyLayer(GeneratorData *data,
                                             const GenerationContext *context,
                                             const MaskState *layer,
                                             const MaskLayer *maskLayer,
                                             bool flattenSource)
    {
        if (!m_Shader || context == nullptr || data == nullptr || context->tileResolution <= 0) {
            return false;
        }

        TF3D_PROFILE_SCOPE_CHILD("customize-base-shape");

        data->Bind(0);
        m_Shader->Bind();
        m_Shader->SetUniform1i("u_Resolution", context->tileResolution);
        m_Shader->SetUniform1i("u_UseMask", layer != nullptr ? 1 : 0);
        m_Shader->SetUniform1i("u_FlattenSource", flattenSource ? 1 : 0);
        m_Shader->SetUniform1i("u_Direction", layer != nullptr && layer->raise ? 1 : -1);
        m_Shader->SetUniform1f("u_Strength", layer != nullptr ? glm::max(layer->strength, 0.0f) : 0.0f);
        m_Shader->SetUniform1f("u_Smoothing", layer != nullptr ? glm::clamp(layer->smoothing, 0.0f, 1.0f) : 0.0f);
        if (layer != nullptr && maskLayer != nullptr) {
            if (auto *maskTexture = maskLayer->GetTexture()) {
                m_Shader->SetUniform1i("u_MaskTexture", maskTexture->Bind(2));
            }
        }

        const auto workgroupSize = std::max(context->gpuWorkgroupSize, 1);
        const auto resolution    = context->tileResolution;
        const auto dispatchSize  = (resolution + workgroupSize - 1) / workgroupSize;
        TF3D_PROFILE_GPU_SCOPE_CHILD("gpu");
        TF3D_PROFILE_VALUE_DOMAIN("generation/customize-base-shape/dispatch", dispatchSize, dispatchSize, 1,
                                  PerformanceMonitor::Domain::Generation);
        m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
        m_Shader->SetMemoryBarrier();
        return true;
    }

    void BiomeCustomizeBaseShape::Update(const Snapshot *state,
                                         const GenerationContext *context,
                                         GeneratorData *baseShapeBuffer)
    {
        if (state == nullptr || state->runtime == nullptr || context == nullptr ||
            baseShapeBuffer == nullptr || !m_Shader) {
            return;
        }

        if (!state->value.enabled) {
            m_State.MarkProcessed(state->revision);
            return;
        }

        if (state->value.flattenBaseShape) {
            if (!ApplyLayer(baseShapeBuffer, context, nullptr, nullptr, true)) {
                return;
            }
        }

        for (size_t maskIndex = 0; maskIndex < state->value.masks.size(); ++maskIndex) {
            const auto &layer = state->value.masks[maskIndex];
            if (!layer.enabled) {
                continue;
            }
            if (maskIndex >= state->runtime->masks.size()) {
                continue;
            }
            const auto &maskLayer = state->runtime->masks[maskIndex];
            if (!UpdateLayerMask(layer, context, maskLayer, baseShapeBuffer)) {
                continue;
            }
            if (!ApplyLayer(baseShapeBuffer, context, &layer, maskLayer.get(), false)) {
                return;
            }
        }

        m_State.MarkProcessed(state->revision);
    }

    SerializerNode BiomeCustomizeBaseShape::Save() const
    {
        auto node = CreateSerializerNode();
        node->Set("Enabled", m_UIState.enabled);
        node->Set("FlattenBaseShape", m_UIState.flattenBaseShape);

        std::vector<SerializerNode> masks;
        masks.reserve(m_UIState.masks.size());
        for (size_t maskIndex = 0; maskIndex < m_UIState.masks.size(); ++maskIndex) {
            const auto &layer = m_UIState.masks[maskIndex];
            auto mask         = CreateSerializerNode();
            mask->Set("Name", layer.name);
            mask->Set("Enabled", layer.enabled);
            mask->Set("Raise", layer.raise);
            mask->Set("Strength", layer.strength);
            mask->Set("Smoothing", layer.smoothing);
            if (maskIndex < m_RuntimeMasks.size() && m_RuntimeMasks[maskIndex] != nullptr) {
                m_RuntimeMasks[maskIndex]->SaveTo(mask);
            }
            masks.push_back(std::move(mask));
        }
        node->Set("Masks", masks);
        return node;
    }

    void BiomeCustomizeBaseShape::Load(SerializerNode node)
    {
        if (node == nullptr)
            return;

        m_UIState.enabled          = node->Get<bool>("Enabled", m_UIState.enabled);
        m_UIState.flattenBaseShape = node->Get<bool>("FlattenBaseShape", m_UIState.flattenBaseShape);
        if (node->HasKey("Masks")) {
            m_UIState.masks.clear();
            m_RuntimeMasks.clear();
            for (const auto &maskNode : node->Get<std::vector<SerializerNode>>("Masks")) {
                if (maskNode == nullptr) {
                    continue;
                }
                auto layer             = CreateMaskState(maskNode->Get<std::string>("Name", "Mask"));
                layer.enabled          = maskNode->Get<bool>("Enabled", layer.enabled);
                layer.raise            = maskNode->Get<bool>("Raise", layer.raise);
                layer.strength         = maskNode->Get<float>("Strength", layer.strength);
                layer.smoothing        = maskNode->Get<float>("Smoothing", layer.smoothing);
                const size_t maskIndex = m_RuntimeMasks.size() - 1;
                if (maskIndex < m_RuntimeMasks.size() && m_RuntimeMasks[maskIndex] != nullptr) {
                    m_RuntimeMasks[maskIndex]->LoadFrom(maskNode);
                }
                m_UIState.masks.push_back(std::move(layer));
            }
        }
        m_SelectedMask = glm::clamp(m_SelectedMask,
                                    0,
                                    std::max(0, static_cast<int>(m_UIState.masks.size()) - 1));
        m_UIState      = CaptureState();
        m_State.Replace(m_UIState);
    }

    void BiomeCustomizeBaseShape::Resize()
    {
        for (size_t maskIndex = 0; maskIndex < m_RuntimeMasks.size(); ++maskIndex) {
            if (m_RuntimeMasks[maskIndex] != nullptr) {
                m_RuntimeMasks[maskIndex]->Resize(m_AppState->mainMap.tileResolution);
            }
        }
        m_UIState = CaptureState();
        m_State.Replace(m_UIState);
    }

} // namespace tf3d::generators
