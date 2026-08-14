#pragma once

#include "Base/Base.h"
#include "Base/Shader.h"
#include "Generators/DEM/DEMTileTypes.h"
#include "Generators/GeneratorTexture.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>

TF3D_FWD_DEC_CLASS(GeneratorData, tf3d::generators)
TF3D_FWD_DEC_CLASS(Texture2D, tf3d::base)
TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::generators::dem
{

    struct RenderSettings {
        int32_t tileResolution      = 0;
        int32_t workgroupSize       = 1;
        float zoomOnMap             = 1.0f;
        float mapStrength           = 1.0f;
        glm::vec2 mapCenter         = glm::vec2(0.0f);
        bool adaptiveBaseMultiplier = true;
    };

    struct RenderTile {
        TileKey key;
        std::shared_ptr<base::Texture2D> texture;
        bool fallback = false;
    };

    struct RenderStats {
        int tilesUsing    = 0;
        int tilesFallback = 0;
    };

    class Renderer
    {
    public:
        explicit Renderer(tf3d::data::ApplicationState *appState);

        RenderStats Render(const RenderSettings &settings,
                           GeneratorData *buffer,
                           std::span<const RenderTile> tiles);

        inline const GeneratorTexture &Texture() const
        {
            return m_Texture;
        }

    private:
        static constexpr int32_t kPreviewWidth  = 512;
        static constexpr int32_t kPreviewHeight = 512;

        GeneratorTexture m_Texture;
        std::optional<base::ComputeShader> m_Shader;
    };

} // namespace tf3d::generators::dem
