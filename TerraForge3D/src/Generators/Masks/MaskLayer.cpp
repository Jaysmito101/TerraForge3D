#include "Generators/Masks/MaskLayer.h"

#include "UI/ImGuiComponents.h"

#include <algorithm>
#include <utility>

namespace tf3d::generators
{

    MaskLayer::MaskLayer(tf3d::data::ApplicationState *appState, glm::vec3 vizColor,
                         std::string defaultTypeID)
        : m_AppState(appState),
          m_BaseMaskGenerator(appState, std::move(defaultTypeID)),
          m_MaskTool(appState, vizColor),
          m_Rasterizer(appState, m_BaseMaskGenerator),
          m_Texture(std::make_shared<GeneratorTexture>(256, 256, GeneratorTextureStorage::R16))
    {
    }

    MaskLayer::MaskLayer(MaskLayer &&other) noexcept
        : m_AppState(other.m_AppState),
          m_BaseMaskGenerator(std::move(other.m_BaseMaskGenerator)),
          m_MaskTool(std::move(other.m_MaskTool)),
          m_Rasterizer(std::move(other.m_Rasterizer)),
          m_Texture(std::move(other.m_Texture)),
          m_BaseTexture(std::move(other.m_BaseTexture)),
          m_VisualizationTexture(std::move(other.m_VisualizationTexture)),
          m_LastSourceData(other.m_LastSourceData),
          m_BaseNeedsUpdate(other.m_BaseNeedsUpdate)
    {
        other.m_AppState       = nullptr;
        other.m_LastSourceData = nullptr;
    }

    MaskLayer &MaskLayer::operator=(MaskLayer &&other) noexcept
    {
        if (this == &other)
            return *this;

        m_AppState             = other.m_AppState;
        m_BaseMaskGenerator    = std::move(other.m_BaseMaskGenerator);
        m_MaskTool             = std::move(other.m_MaskTool);
        m_Rasterizer           = std::move(other.m_Rasterizer);
        m_Texture              = std::move(other.m_Texture);
        m_BaseTexture          = std::move(other.m_BaseTexture);
        m_VisualizationTexture = std::move(other.m_VisualizationTexture);
        m_LastSourceData       = other.m_LastSourceData;
        m_BaseNeedsUpdate      = other.m_BaseNeedsUpdate;

        other.m_AppState       = nullptr;
        other.m_LastSourceData = nullptr;
        return *this;
    }

    void MaskLayer::Resize(int size)
    {
        if (size <= 0)
            return;
        if (m_Texture == nullptr) {
            m_Texture = std::make_shared<GeneratorTexture>(size, size, GeneratorTextureStorage::R16);
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
        m_BaseNeedsUpdate = true;
    }

    void MaskLayer::EnsureVisualizationTexture()
    {
        if (m_Texture == nullptr || m_Texture->GetWidth() <= 0)
            return;

        const int visualizationSize = std::min(m_Texture->GetWidth(), kMaxVisualizationResolution);
        if (m_VisualizationTexture == nullptr) {
            m_VisualizationTexture = std::make_unique<GeneratorTexture>(visualizationSize,
                                                                        visualizationSize,
                                                                        GeneratorTextureStorage::R16);
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
        if (changed) {
            m_BaseNeedsUpdate = true;
        }
        return changed;
    }

    bool MaskLayer::Render(bool rebuildBase)
    {
        if (m_Texture == nullptr)
            return false;

        if (m_BaseMaskGenerator.IsNone()) {
            m_BaseTexture.reset();
        } else if (m_BaseTexture == nullptr) {
            m_BaseTexture = std::make_unique<GeneratorTexture>(m_Texture->GetWidth(),
                                                               m_Texture->GetHeight(),
                                                               GeneratorTextureStorage::R16);
            rebuildBase   = true;
        }

        const bool rendered = m_Rasterizer.Render(m_LastSourceData,
                                                  m_BaseMaskGenerator,
                                                  m_MaskTool.GetStrokes(),
                                                  m_MaskTool.GetActiveStroke(),
                                                  m_Texture.get(),
                                                  m_BaseTexture.get(),
                                                  rebuildBase);
        if (rendered) {
            m_BaseNeedsUpdate = false;
        }
        return rendered;
    }

    bool MaskLayer::ShowSettings(bool showViewportMask)
    {
        bool changed = ShowBaseSettings();

        GeneratorTexture *previewTexture = m_Texture.get();
        if (m_MaskTool.GetInvertPreview()) {
            EnsureVisualizationTexture();
            previewTexture = m_Rasterizer.GetPreviewTexture(m_Texture.get(),
                                                            m_VisualizationTexture.get(),
                                                            true);
        }
        changed |= m_MaskTool.ShowSettings(m_Texture.get(), previewTexture, showViewportMask);

        if (changed) {
            Render(m_BaseNeedsUpdate);
        }
        return changed;
    }

    bool MaskLayer::Update(GeneratorData *sourceData)
    {
        if (sourceData != nullptr && sourceData->GetResolution() > 0 &&
            (m_Texture == nullptr || m_Texture->GetWidth() != sourceData->GetResolution())) {
            Resize(sourceData->GetResolution());
        }
        m_LastSourceData = sourceData;
        return Render(true);
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
        m_BaseNeedsUpdate = true;
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
