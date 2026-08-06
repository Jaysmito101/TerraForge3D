#pragma once

#include "Base/Base.h"

#include <array>
#include <cstdint>
#include <utility>

namespace tf3d::renderer
{
    enum RendererViewportMode {
        RendererViewportMode_Object = 0,
        RendererViewportMode_Wireframe,
        RendererViewportMode_Heightmap,
        RendererViewportMode_TextureSlot,
        RendererViewportMode_COUNT
    };

    class RendererViewport
    {
    public:
        RendererViewport();
        ~RendererViewport();
        void ResizeTo(uint32_t width, uint32_t height);

        std::shared_ptr<FrameBuffer> &GetFrameBuffer();
        const std::shared_ptr<FrameBuffer> &GetFrameBuffer() const;

        Camera &GetCamera();
        const Camera &GetCamera() const;

        RendererViewportMode GetMode() const;
        void SetMode(RendererViewportMode mode);

        std::array<float, 4> &GetPositionOnTerrain();
        const std::array<float, 4> &GetPositionOnTerrain() const;

        float &GetHeightmapOffsetX();
        float &GetHeightmapOffsetY();
        float &GetHeightmapScale();
        
        float &GetTextureSlotOffsetX();
        float &GetTextureSlotOffsetY();
        float &GetTextureSlotScale();
        float GetAspectRatio() const;
        void SetAspectRatio(float aspectRatio);

        std::array<float, 2> &GetMousePosition();
        const std::array<float, 2> &GetMousePosition() const;

        int32_t GetWidth() const;
        int32_t GetHeight() const;

        bool IsHovered() const;
        void SetHovered(bool hovered);

        bool &GetTextureSlotDetailedMode();
        int32_t &GetTextureSlot();
        std::array<std::pair<int32_t, int32_t>, 4> &GetTextureSlotDetailed();
        const std::array<std::pair<int32_t, int32_t>, 4> &GetTextureSlotDetailed() const;

    private:
        struct SharedState {
            std::shared_ptr<FrameBuffer> frameBuffer;
            std::array<float, 2> mousePosition{0.0f, 0.0f};
            int32_t width  = 0;
            int32_t height = 0;
            float aspectRatio = 1.0f;
            bool isHovered = false;
        };

        struct SceneModeState {
            Camera camera;
            std::array<float, 4> positionOnTerrain{0.0f, 0.0f, 0.0f, 0.0f};
        };

        struct HeightmapModeState {
            float offsetX = 0.0f;
            float offsetY = 0.0f;
            float scale   = 1.0f;
        };

        struct TextureSlotModeState {
            float offsetX        = 0.0f;
            float offsetY        = 0.0f;
            float scale          = 1.0f;
            bool detailedMode   = false;
            int32_t textureSlot = 0;
            std::array<std::pair<int32_t, int32_t>, 4> detailed{};
        };

        SharedState m_Shared;
        SceneModeState m_SceneMode;
        HeightmapModeState m_HeightmapMode;
        TextureSlotModeState m_TextureSlotMode;
        RendererViewportMode m_Mode = RendererViewportMode_Object;
    };

} // namespace tf3d::renderer
