#include "Generators/Masks/MaskLayer.h"

#include "UI/ImGuiComponents.h"

#include <algorithm>
#include <utility>

namespace tf3d::generators
{

    MaskLayer::MaskLayer(tf3d::data::ApplicationState *appState, glm::vec3 vizColor,
                         std::string defaultTypeID, bool allowNegativeValues)
        : m_AppState(appState),
          m_AllowNegativeValues(allowNegativeValues),
          m_BaseMaskGenerator(appState, std::move(defaultTypeID)),
          m_MaskTool(appState, vizColor, allowNegativeValues),
          m_Rasterizer(appState, m_BaseMaskGenerator, allowNegativeValues),
          m_UIState(State{m_BaseMaskGenerator.GetState(), m_MaskTool.GetState()}),
          m_State(m_UIState),
          m_Texture(std::make_shared<GeneratorTexture>(
              256,
              256,
              allowNegativeValues ? GeneratorTextureStorage::R16F : GeneratorTextureStorage::R16))
    {
    }

    MaskLayer::MaskLayer(MaskLayer &&other) noexcept
        : m_AppState(other.m_AppState),
          m_AllowNegativeValues(other.m_AllowNegativeValues),
          m_BaseMaskGenerator(std::move(other.m_BaseMaskGenerator)),
          m_MaskTool(std::move(other.m_MaskTool)),
          m_Rasterizer(std::move(other.m_Rasterizer)),
          m_UIState(other.m_State.Capture().value),
          m_State(m_UIState),
          m_Texture(std::move(other.m_Texture)),
          m_BaseTexture(std::move(other.m_BaseTexture)),
          m_VisualizationTexture(std::move(other.m_VisualizationTexture))
    {
        other.m_AppState = nullptr;
    }

    MaskLayer &MaskLayer::operator=(MaskLayer &&other) noexcept
    {
        if (this == &other)
            return *this;

        m_AppState            = other.m_AppState;
        m_AllowNegativeValues = other.m_AllowNegativeValues;
        m_BaseMaskGenerator   = std::move(other.m_BaseMaskGenerator);
        m_MaskTool            = std::move(other.m_MaskTool);
        m_Rasterizer          = std::move(other.m_Rasterizer);
        m_UIState             = other.m_State.Capture().value;
        m_State.Replace(m_UIState);
        m_Texture              = std::move(other.m_Texture);
        m_BaseTexture          = std::move(other.m_BaseTexture);
        m_VisualizationTexture = std::move(other.m_VisualizationTexture);

        other.m_AppState = nullptr;
        return *this;
    }

    void MaskLayer::Resize(int size)
    {
        if (size <= 0)
            return;
        if (m_Texture == nullptr) {
            m_Texture = std::make_shared<GeneratorTexture>(
                size,
                size,
                m_AllowNegativeValues ? GeneratorTextureStorage::R16F : GeneratorTextureStorage::R16);
        } else {
            m_Texture->Resize(size, size);
        }
        if (m_BaseTexture != nullptr) {
            m_BaseTexture->Resize(size, size);
        }
        if (m_VisualizationTexture != nullptr) {
            const int visualizationSize = std::min(size, kMaxVisualizationResolution);
            m_VisualizationTexture->Resize(visualizationSize, visualizationSize);
        }
        m_MaskTool.OnResolutionChanged();
    }

    void MaskLayer::EnsureVisualizationTexture()
    {
        if (m_Texture == nullptr || m_Texture->GetWidth() <= 0) {
            return;
        }

        const int visualizationSize = std::min(m_Texture->GetWidth(), kMaxVisualizationResolution);
        if (m_VisualizationTexture == nullptr) {
            m_VisualizationTexture = std::make_unique<GeneratorTexture>(visualizationSize,
                                                                        visualizationSize,
                                                                        m_AllowNegativeValues
                                                                            ? GeneratorTextureStorage::RGBA32F
                                                                            : GeneratorTextureStorage::R16);
        } else if (m_VisualizationTexture->GetWidth() != visualizationSize) {
            m_VisualizationTexture->Resize(visualizationSize, visualizationSize);
        }
    }

    bool MaskLayer::ShowBaseSettings()
    {
        bool changed = false;
        ImGui::SeparatorText("Mask base");

        changed |= m_BaseMaskGenerator.ShowSettings();
        if (m_BaseMaskGenerator.IsNone()) {
            ImGui::TextDisabled("Starts from black; strokes are applied on top.");
        } else {
            ImGui::TextDisabled("Calculated from the current source terrain.");
        }
        if (m_AllowNegativeValues) {
            ImGui::TextDisabled("Signed values are enabled: blue is negative, red is positive.");
        }
        return changed;
    }

    MaskLayer::State MaskLayer::CaptureState() const
    {
        return State{m_BaseMaskGenerator.GetState(), m_MaskTool.GetState()};
    }

    bool MaskLayer::Render(const State &state,
                           const GenerationContext *context,
                           GeneratorData *sourceData)
    {
        if (m_Texture == nullptr || context == nullptr) {
            return false;
        }

        if (state.base.runtimeMode < 0) {
            m_BaseTexture.reset();
        } else if (m_BaseTexture == nullptr) {
            m_BaseTexture = std::make_unique<GeneratorTexture>(m_Texture->GetWidth(),
                                                               m_Texture->GetHeight(),
                                                               m_AllowNegativeValues
                                                                   ? GeneratorTextureStorage::R16F
                                                                   : GeneratorTextureStorage::R16);
        }

        const MaskStroke *activeStroke = state.tool.activeStroke.has_value()
                                             ? &*state.tool.activeStroke
                                             : nullptr;
        const bool rendered            = m_Rasterizer.Render(sourceData,
                                                             context,
                                                             m_BaseMaskGenerator,
                                                             state.base,
                                                             state.tool.strokes,
                                                             activeStroke,
                                                             m_Texture.get(),
                                                             m_BaseTexture.get(),
                                                             true);
        return rendered;
    }

    bool MaskLayer::ShowSettings(bool showViewportMask)
    {
        bool changed = ShowBaseSettings();

        GeneratorTexture *previewTexture = m_Texture.get();
        if (m_AllowNegativeValues || m_MaskTool.GetInvertPreview()) {
            EnsureVisualizationTexture();
            previewTexture = m_Rasterizer.GetPreviewTexture(m_Texture.get(),
                                                            m_VisualizationTexture.get(),
                                                            m_MaskTool.GetInvertPreview());
        }
        changed |= m_MaskTool.ShowSettings(m_Texture.get(), previewTexture, showViewportMask);

        if (changed) {
            m_UIState = CaptureState();
            m_State.Replace(m_UIState);
        }
        return changed;
    }

    bool MaskLayer::Apply(const State &state,
                          const GenerationContext *context,
                          GeneratorData *sourceData)
    {
        if (sourceData != nullptr && sourceData->GetResolution() > 0 &&
            (m_Texture == nullptr || m_Texture->GetWidth() != sourceData->GetResolution())) {
            Resize(sourceData->GetResolution());
        }
        return Render(state, context, sourceData);
    }

    bool MaskLayer::Update(const Snapshot *state,
                           const GenerationContext *context,
                           GeneratorData *sourceData)
    {
        if (state == nullptr) {
            return false;
        }

        const bool rendered = Apply(state->value, context, sourceData);
        if (rendered) {
            m_State.MarkProcessed(state->revision);
        }
        return rendered;
    }

    void MaskLayer::SaveTo(SerializerNode node) const
    {
        if (node == nullptr)
            return;
        node->Set("BaseMask", m_BaseMaskGenerator.Save());
        node->Set("MaskTool", m_MaskTool.Save());
    }

    void MaskLayer::LoadFrom(SerializerNode node)
    {
        if (node == nullptr)
            return;

        const auto toolNode    = node->Get<SerializerNode>("MaskTool");
        const auto baseNode    = node->Get<SerializerNode>("BaseMask");
        const auto oldBaseNode = node->Get<SerializerNode>("CalculatedMask");

        m_BaseMaskGenerator.Load(baseNode != nullptr ? baseNode : oldBaseNode);

        if (node->HasKey("BaseType")) {
            const int oldBaseType = glm::clamp(node->Get<int>("BaseType", 0), 0, 1);
            if (oldBaseType == 0) {
                m_BaseMaskGenerator.SetTypeID("None");
            } else if (baseNode == nullptr && oldBaseNode == nullptr) {
                m_BaseMaskGenerator.SetTypeID("HeightRange");
            }
        } else if (toolNode != nullptr && toolNode->HasKey("MaskSource")) {
            const int oldSource = toolNode->Get<int>("MaskSource", 0);
            if (oldSource == 0) {
                m_BaseMaskGenerator.SetTypeID("None");
            } else if (baseNode == nullptr && oldBaseNode == nullptr) {
                m_BaseMaskGenerator.SetTypeID("HeightRange");
            }
        }

        m_MaskTool.Load(toolNode);
        m_UIState = CaptureState();
        m_State.Replace(m_UIState);
    }

    void MaskLayer::SetInvertPreview(bool invert)
    {
        m_MaskTool.SetInvertPreview(invert);
        if (!invert)
            m_VisualizationTexture.reset();
    }

    void MaskLayer::SetVizColor(float r, float g, float b)
    {
        m_MaskTool.SetVizColor(r, g, b);
    }

} // namespace tf3d::generators
