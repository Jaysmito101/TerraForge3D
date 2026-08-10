#pragma once

#include "Base/Base.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Generators/Masks/MaskTool.h"

#include <memory>
#include <optional>
#include <vector>

TF3D_FWD_DEC_CLASS(ComputeShader, tf3d::base)
TF3D_FWD_DEC_CLASS(ShaderStorageBuffer, tf3d::base)
TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::generators
{

    class BaseMaskGenerator;

    class MaskRasterizer
    {
    public:
        MaskRasterizer(tf3d::data::ApplicationState *appState,
                       const BaseMaskGenerator &baseGenerator);
        ~MaskRasterizer() = default;

        MaskRasterizer(const MaskRasterizer &)                = delete;
        MaskRasterizer &operator=(const MaskRasterizer &)     = delete;
        MaskRasterizer(MaskRasterizer &&) noexcept            = default;
        MaskRasterizer &operator=(MaskRasterizer &&) noexcept = default;

        bool Render(GeneratorData *sourceData,
                    const BaseMaskGenerator &baseGenerator,
                    const std::vector<MaskStroke> &strokes,
                    const MaskStroke *activeStroke,
                    GeneratorTexture *destination,
                    GeneratorTexture *baseTexture,
                    bool rebuildBase);

        GeneratorTexture *GetPreviewTexture(GeneratorTexture *sourceTexture,
                                            GeneratorTexture *visualizationTexture,
                                            bool invert);

    private:
        void UploadStrokes(const std::vector<MaskStroke> &strokes, const MaskStroke *activeStroke);
        bool Dispatch(GeneratorData *sourceData,
                      const BaseMaskGenerator &baseGenerator,
                      GeneratorTexture *destination,
                      GeneratorTexture *baseTexture,
                      int strokeCount,
                      bool useCachedBase);

        tf3d::data::ApplicationState *m_AppState = nullptr;
        std::optional<tf3d::base::ComputeShader> m_Shader;
        std::optional<tf3d::base::ComputeShader> m_CopyShader;
        std::shared_ptr<tf3d::base::ShaderStorageBuffer> m_StrokeSettingsBuffer;
        std::shared_ptr<tf3d::base::ShaderStorageBuffer> m_StrokeRangesBuffer;
        std::shared_ptr<tf3d::base::ShaderStorageBuffer> m_StrokePointsBuffer;
        bool m_StrokeDataValid = false;
    };

} // namespace tf3d::generators
