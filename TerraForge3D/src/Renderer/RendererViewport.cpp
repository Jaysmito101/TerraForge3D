#include "Renderer/RendererViewport.h"
#include "Profiler.h"

namespace tf3d::renderer
{
    RendererViewport::RendererViewport()
    {
        m_Shared.width       = 1024;
        m_Shared.height      = 1024;
        m_Shared.frameBuffer = std::make_shared<FrameBuffer>(1024, 1024);
    }

    RendererViewport::~RendererViewport()
    {
    }

    SerializerNode RendererViewport::Save() const
    {
        SerializerNode node = CreateSerializerNode();
        node->Set("Mode", std::string(RendererViewportModeToString(m_Mode)));
        node->Set("Camera", m_SceneMode.camera.Save());
        node->Set(
            "PositionOnTerrain",
            glm::vec4(m_SceneMode.positionOnTerrain[0], m_SceneMode.positionOnTerrain[1],
                      m_SceneMode.positionOnTerrain[2], m_SceneMode.positionOnTerrain[3]));

        SerializerNode heightmap = CreateSerializerNode();
        heightmap->Set("Offset", glm::vec2(m_HeightmapMode.offsetX, m_HeightmapMode.offsetY));
        heightmap->Set("Scale", m_HeightmapMode.scale);
        node->Set("Heightmap", heightmap);

        SerializerNode textureSlot = CreateSerializerNode();
        textureSlot->Set("Offset", glm::vec2(m_TextureSlotMode.offsetX, m_TextureSlotMode.offsetY));
        textureSlot->Set("Scale", m_TextureSlotMode.scale);
        textureSlot->Set("DetailedMode", m_TextureSlotMode.detailedMode);
        textureSlot->Set("TextureSlot", m_TextureSlotMode.textureSlot);
        std::vector<SerializerNode> channels;
        channels.reserve(m_TextureSlotMode.detailed.size());
        for (const auto &[slot, channel] : m_TextureSlotMode.detailed) {
            SerializerNode detail = CreateSerializerNode();
            detail->Set("TextureSlot", slot);
            detail->Set("Channel", channel);
            channels.push_back(detail);
        }
        textureSlot->Set("Channels", channels);
        node->Set("TextureSlot", textureSlot);

        SerializerNode render = CreateSerializerNode();
        render->Set("Width", m_Shared.width);
        render->Set("Height", m_Shared.height);
        render->Set("AspectRatio", m_Shared.aspectRatio);
        node->Set("Render", render);

        SerializerNode interaction = CreateSerializerNode();
        interaction->Set("MousePosition", glm::vec2(m_Shared.mousePosition[0], m_Shared.mousePosition[1]));
        interaction->Set("Hovered", m_Shared.isHovered);
        node->Set("Interaction", interaction);
        return node;
    }

    void RendererViewport::Load(const SerializerNode &data)
    {
        if (!data)
            return;

        if (data->HasKey("Mode")) {
            RendererViewportMode mode;
            if (TryParseRendererViewportMode(data->Get<std::string>("Mode"), mode))
                m_Mode = mode;
        }
        if (const SerializerNode camera = data->Get<SerializerNode>("Camera"))
            m_SceneMode.camera.Load(camera);

        if (data->HasKey("PositionOnTerrain")) {
            const glm::vec4 position      = data->Get<glm::vec4>("PositionOnTerrain");
            m_SceneMode.positionOnTerrain = {position.x, position.y, position.z, position.w};
        }

        if (const SerializerNode heightmap = data->Get<SerializerNode>("Heightmap")) {
            const glm::vec2 offset = heightmap->Get<glm::vec2>(
                "Offset", glm::vec2(m_HeightmapMode.offsetX, m_HeightmapMode.offsetY));
            m_HeightmapMode.offsetX = offset.x;
            m_HeightmapMode.offsetY = offset.y;
            m_HeightmapMode.scale   = std::max(
                heightmap->Get<float>("Scale", m_HeightmapMode.scale), 0.0000001f);
        }

        if (const SerializerNode textureSlot = data->Get<SerializerNode>("TextureSlot")) {
            const glm::vec2 offset = textureSlot->Get<glm::vec2>(
                "Offset", glm::vec2(m_TextureSlotMode.offsetX, m_TextureSlotMode.offsetY));
            m_TextureSlotMode.offsetX = offset.x;
            m_TextureSlotMode.offsetY = offset.y;
            m_TextureSlotMode.scale   = std::max(
                textureSlot->Get<float>("Scale", m_TextureSlotMode.scale), 0.0000001f);
            m_TextureSlotMode.detailedMode = textureSlot->Get<bool>(
                "DetailedMode", m_TextureSlotMode.detailedMode);
            m_TextureSlotMode.textureSlot = std::clamp(
                textureSlot->Get<int>("TextureSlot", m_TextureSlotMode.textureSlot), 0, 5);

            const auto channels = textureSlot->Get<std::vector<SerializerNode>>("Channels");
            if (!channels.empty()) {
                const size_t count = std::min(channels.size(), m_TextureSlotMode.detailed.size());
                for (size_t index = 0; index < count; ++index) {
                    m_TextureSlotMode.detailed[index] = {
                        std::clamp(channels[index]->Get<int>(
                                       "TextureSlot", m_TextureSlotMode.detailed[index].first),
                                   0, 5),
                        std::clamp(channels[index]->Get<int>(
                                       "Channel", m_TextureSlotMode.detailed[index].second),
                                   0, 3)};
                }
            }
        }
    }

    void RendererViewport::ResizeTo(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
            return;
        if (m_Shared.frameBuffer && m_Shared.width == static_cast<int32_t>(width) &&
            m_Shared.height == static_cast<int32_t>(height))
            return;

        TF3D_PROFILE_SCOPE_DOMAIN("renderer/viewport/resize", PerformanceMonitor::Domain::Resource);
        TF3D_PROFILE_VALUE_DOMAIN("renderer/viewport/size", width, height, 0,
                                  PerformanceMonitor::Domain::Resource);
        m_Shared.width       = static_cast<int32_t>(width);
        m_Shared.height      = static_cast<int32_t>(height);
        m_Shared.aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        m_Shared.frameBuffer = std::make_shared<FrameBuffer>(width, height);
    }

    std::shared_ptr<FrameBuffer> &RendererViewport::GetFrameBuffer()
    {
        return m_Shared.frameBuffer;
    }

    const std::shared_ptr<FrameBuffer> &RendererViewport::GetFrameBuffer() const
    {
        return m_Shared.frameBuffer;
    }

    Camera &RendererViewport::GetCamera()
    {
        return m_SceneMode.camera;
    }

    const Camera &RendererViewport::GetCamera() const
    {
        return m_SceneMode.camera;
    }

    RendererViewportMode RendererViewport::GetMode() const
    {
        return m_Mode;
    }

    void RendererViewport::SetMode(RendererViewportMode mode)
    {
        m_Mode = mode;
    }

    std::array<float, 4> &RendererViewport::GetPositionOnTerrain()
    {
        return m_SceneMode.positionOnTerrain;
    }

    const std::array<float, 4> &RendererViewport::GetPositionOnTerrain() const
    {
        return m_SceneMode.positionOnTerrain;
    }

    float &RendererViewport::GetHeightmapOffsetX()
    {
        return m_HeightmapMode.offsetX;
    }

    float &RendererViewport::GetHeightmapOffsetY()
    {
        return m_HeightmapMode.offsetY;
    }

    float &RendererViewport::GetHeightmapScale()
    {
        return m_HeightmapMode.scale;
    }

    float &RendererViewport::GetTextureSlotOffsetX()
    {
        return m_TextureSlotMode.offsetX;
    }

    float &RendererViewport::GetTextureSlotOffsetY()
    {
        return m_TextureSlotMode.offsetY;
    }

    float &RendererViewport::GetTextureSlotScale()
    {
        return m_TextureSlotMode.scale;
    }

    float RendererViewport::GetAspectRatio() const
    {
        return m_Shared.aspectRatio;
    }

    void RendererViewport::SetAspectRatio(float aspectRatio)
    {
        m_Shared.aspectRatio = aspectRatio;
    }

    std::array<float, 2> &RendererViewport::GetMousePosition()
    {
        return m_Shared.mousePosition;
    }

    const std::array<float, 2> &RendererViewport::GetMousePosition() const
    {
        return m_Shared.mousePosition;
    }

    int32_t RendererViewport::GetWidth() const
    {
        return m_Shared.width;
    }

    int32_t RendererViewport::GetHeight() const
    {
        return m_Shared.height;
    }

    bool RendererViewport::IsHovered() const
    {
        return m_Shared.isHovered;
    }

    void RendererViewport::SetHovered(bool hovered)
    {
        m_Shared.isHovered = hovered;
    }

    bool &RendererViewport::GetTextureSlotDetailedMode()
    {
        return m_TextureSlotMode.detailedMode;
    }

    int32_t &RendererViewport::GetTextureSlot()
    {
        return m_TextureSlotMode.textureSlot;
    }

    std::array<std::pair<int32_t, int32_t>, 4> &RendererViewport::GetTextureSlotDetailed()
    {
        return m_TextureSlotMode.detailed;
    }

    const std::array<std::pair<int32_t, int32_t>, 4> &RendererViewport::GetTextureSlotDetailed() const
    {
        return m_TextureSlotMode.detailed;
    }

} // namespace tf3d::renderer
