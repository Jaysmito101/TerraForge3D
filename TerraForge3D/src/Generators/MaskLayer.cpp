#include "Generators/MaskLayer.h"

#include <utility>

namespace tf3d::generators
{

    MaskLayer::MaskLayer(ApplicationState *appState, glm::vec3 vizColor,
                         std::string defaultTypeID, std::string generatedMaskLabel)
        : m_CalculatedMaskGenerator(appState, std::move(defaultTypeID)),
          m_MaskTool(appState, vizColor),
          m_GeneratedMaskLabel(std::move(generatedMaskLabel))
    {
        if (m_GeneratedMaskLabel.empty())
            m_GeneratedMaskLabel = "Generated mask";

        AttachGeneratedMask();
    }

    MaskLayer::MaskLayer(MaskLayer &&other) noexcept
        : m_CalculatedMaskGenerator(std::move(other.m_CalculatedMaskGenerator)),
          m_MaskTool(std::move(other.m_MaskTool)),
          m_GeneratedMaskLabel(std::move(other.m_GeneratedMaskLabel))
    {
        AttachGeneratedMask();
    }

    MaskLayer &MaskLayer::operator=(MaskLayer &&other) noexcept
    {
        if (this == &other)
            return *this;

        m_CalculatedMaskGenerator = std::move(other.m_CalculatedMaskGenerator);
        m_MaskTool                = std::move(other.m_MaskTool);
        m_GeneratedMaskLabel      = std::move(other.m_GeneratedMaskLabel);
        AttachGeneratedMask();
        return *this;
    }

    void MaskLayer::AttachGeneratedMask()
    {
        m_MaskTool.SetGeneratedMaskTexture(GetGeneratedTexture(), m_GeneratedMaskLabel.c_str());
    }

    void MaskLayer::Resize(int size)
    {
        if (size <= 0)
            return;
        m_CalculatedMaskGenerator.Resize(size);
        m_MaskTool.Resize(size);
    }

    bool MaskLayer::ShowGeneratedSettings()
    {
        AttachGeneratedMask();
        if (!m_MaskTool.IsShowingGeneratedMask())
            return false;
        return m_CalculatedMaskGenerator.ShowSettings();
    }

    bool MaskLayer::ShowToolSettings(bool showViewportMask)
    {
        AttachGeneratedMask();
        return m_MaskTool.ShowSettings(showViewportMask);
    }

    bool MaskLayer::ShowSettings(bool showViewportMask)
    {
        bool changed = ShowGeneratedSettings();
        changed |= ShowToolSettings(showViewportMask);
        return changed;
    }

    bool MaskLayer::Update(GeneratorData *sourceData, bool generatedOnly)
    {
        if (sourceData == nullptr)
            return false;
        if (generatedOnly && !m_MaskTool.IsShowingGeneratedMask())
            return false;

        m_CalculatedMaskGenerator.Invalidate();
        const bool updated = m_CalculatedMaskGenerator.Update(sourceData);
        AttachGeneratedMask();
        return updated;
    }

    void MaskLayer::SaveTo(SerializerNode node) const
    {
        if (node == nullptr)
            return;
        node->Set("CalculatedMask", m_CalculatedMaskGenerator.Save());
        node->Set("MaskTool", m_MaskTool.Save());
    }

    void MaskLayer::LoadFrom(SerializerNode node)
    {
        if (node == nullptr)
            return;

        AttachGeneratedMask();
        m_CalculatedMaskGenerator.Load(node->Get<SerializerNode>("CalculatedMask"));
        m_MaskTool.Load(node->Get<SerializerNode>("MaskTool"));
    }

    void MaskLayer::SetPreviewMode(MaskPreviewMode mode)
    {
        m_MaskTool.SetPreviewMode(mode);
    }

    void MaskLayer::SetInvertPreview(bool invert)
    {
        m_MaskTool.SetInvertPreview(invert);
    }

    void MaskLayer::SetVizColor(float r, float g, float b)
    {
        m_MaskTool.SetVizColor(r, g, b);
    }

} // namespace tf3d::generators
