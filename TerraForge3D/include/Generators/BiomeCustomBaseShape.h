#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Inspector/CustomInspector.h"
#include "Renderer/ObjectRenderer.h"
#include "Utils/Utils.h"

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::generators
{

    enum BiomeCustomBaseShapeEditMode {
        BiomeCustomBaseShapeEditMode_Draw,
        BiomeCustomBaseShapeEditMode_Count
    };

    class BiomeCustomBaseShape
    {
    public:
        BiomeCustomBaseShape(ApplicationState *appState);
        ~BiomeCustomBaseShape();

        bool ShowShettings();
        void Update(GeneratorData *sourceBuffer, GeneratorData *targetBuffer, GeneratorData *swapBuffer);

        SerializerNode Save();
        void Load(SerializerNode node);

        inline bool RequireUpdation() const
        {
            return m_RequireUpdation;
        }
        inline bool IsEnabled() const
        {
            return m_Enabled;
        }
        inline bool RequiresBaseShapeUpdate() const
        {
            return m_RequireBaseShapeUpdate;
        }

        void Resize();

    private:
        bool ApplyDrawingShaders();
        bool ShowDrawEditor();

    private:
        data::ApplicationState *m_AppState = nullptr;
        bool m_RequireUpdation             = true;
        bool m_Enabled                     = false;
        bool m_RequireBaseShapeUpdate      = false;
        std::optional<base::ComputeShader> m_Shader;
        std::shared_ptr<GeneratorData> m_WorkingDataBuffer, m_SwapBuffer;
        std::shared_ptr<GeneratorTexture> m_PreviewTexture;
        renderer::DrawBrushSettings m_DrawSettings;
    };

} // namespace tf3d::generators
using tf3d::generators::BiomeCustomBaseShape;
