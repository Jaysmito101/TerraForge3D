#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Generators/Masks/BaseMaskGenerator.h"
#include "Generators/Masks/MaskRasterizer.h"
#include "Generators/Masks/MaskTool.h"

#include <memory>
#include <string>

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::generators
{

    class MaskLayer
    {
    public:
        MaskLayer(tf3d::data::ApplicationState *appState, glm::vec3 vizColor,
                  std::string defaultTypeID = "None");
        ~MaskLayer() = default;

        MaskLayer(const MaskLayer &)            = delete;
        MaskLayer &operator=(const MaskLayer &) = delete;
        MaskLayer(MaskLayer &&other) noexcept;
        MaskLayer &operator=(MaskLayer &&other) noexcept;

        void Resize(int size);
        bool ShowSettings(bool showViewportMask = true);
        bool Update(GeneratorData *sourceData);

        void SaveTo(SerializerNode node) const;
        void LoadFrom(SerializerNode node);

        void SetInvertPreview(bool invert);
        void SetVizColor(float r, float g, float b);

        inline GeneratorTexture *GetTexture() const
        {
            return m_Texture.get();
        }

    private:
        static constexpr int32_t kMaxVisualizationResolution = 512;

        bool ShowBaseSettings();
        bool Render(bool rebuildBase);
        void EnsureVisualizationTexture();

        tf3d::data::ApplicationState *m_AppState = nullptr;
        BaseMaskGenerator m_BaseMaskGenerator;
        MaskTool m_MaskTool;
        MaskRasterizer m_Rasterizer;
        std::shared_ptr<GeneratorTexture> m_Texture;
        std::unique_ptr<GeneratorTexture> m_BaseTexture;
        std::unique_ptr<GeneratorTexture> m_VisualizationTexture;
        GeneratorData *m_LastSourceData = nullptr;
        bool m_BaseNeedsUpdate          = true;
    };

} // namespace tf3d::generators
