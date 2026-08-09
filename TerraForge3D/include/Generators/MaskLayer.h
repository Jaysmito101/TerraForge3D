#pragma once

#include "Exporters/Serializer.h"
#include "Generators/CalculatedMaskGenerator.h"
#include "Generators/GeneratorData.h"
#include "Generators/MaskTool.h"

#include <memory>
#include <string>

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::generators
{
    class MaskLayer
    {
    public:
        MaskLayer(ApplicationState *appState, glm::vec3 vizColor,
                  std::string defaultTypeID = "HeightRange",
                  std::string generatedMaskLabel = "Generated mask");
        ~MaskLayer() = default;

        MaskLayer(const MaskLayer &)            = delete;
        MaskLayer &operator=(const MaskLayer &) = delete;
        MaskLayer(MaskLayer &&other) noexcept;
        MaskLayer &operator=(MaskLayer &&other) noexcept;

        void Resize(int size);
        bool ShowSettings(bool showViewportMask = true);
        bool ShowGeneratedSettings();
        bool ShowToolSettings(bool showViewportMask = true);
        bool Update(GeneratorData *sourceData, bool generatedOnly = false);

        void SaveTo(SerializerNode node) const;
        void LoadFrom(SerializerNode node);

        void SetPreviewMode(MaskPreviewMode mode);
        void SetInvertPreview(bool invert);
        void SetVizColor(float r, float g, float b);

        inline bool IsShowingGeneratedMask() const
        {
            return m_MaskTool.IsShowingGeneratedMask();
        }
        inline GeneratorTexture *GetTexture() const
        {
            return m_MaskTool.GetTexture();
        }
        inline GeneratorTexture *GetPreviewTexture() const
        {
            return m_MaskTool.GetPreviewTexture();
        }
        inline GeneratorTexture *GetGeneratedTexture() const
        {
            return m_CalculatedMaskGenerator.GetTexture();
        }

    private:
        void AttachGeneratedMask();

        CalculatedMaskGenerator m_CalculatedMaskGenerator;
        MaskTool m_MaskTool;
        std::string m_GeneratedMaskLabel;
    };

} // namespace tf3d::generators

using tf3d::generators::MaskLayer;
