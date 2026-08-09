#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Generators/GeneratorData.h"
#include "Generators/MaskLayer.h"

#include <string>
#include <string_view>
#include <vector>

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::generators
{

    class BiomeCustomizeBaseShape
    {
    public:
        explicit BiomeCustomizeBaseShape(ApplicationState *appState);
        ~BiomeCustomizeBaseShape();

        bool ShowSettings();
        void Update(GeneratorData *baseShapeBuffer, GeneratorData *targetBuffer,
                    std::string_view profilePrefix = {});

        SerializerNode Save() const;
        void Load(SerializerNode node);

        inline bool RequireUpdation() const
        {
            return m_RequireUpdation;
        }
        inline bool IsEnabled() const
        {
            return m_Enabled;
        }

        void Resize();

    private:
        struct MaskEntry {
            std::string name;
            bool enabled    = true;
            bool raise      = true;
            float strength  = 1.0f;
            float smoothing = 0.0f;
            std::shared_ptr<MaskLayer> mask;
        };

        MaskEntry CreateMaskLayer(const std::string &name) const;
        void AddMaskLayer();
        bool ShowDrawingSettings();
        bool ApplyLayer(GeneratorData *source, GeneratorData *target,
                        const MaskEntry *layer, bool flattenSource,
                        std::string_view profilePrefix);
        bool UpdateLayerMask(MaskEntry &layer, GeneratorData *source);

    private:
        data::ApplicationState *m_AppState = nullptr;
        bool m_RequireUpdation             = true;
        bool m_Enabled                     = false;
        bool m_FlattenBaseShape            = false;
        std::optional<base::ComputeShader> m_Shader;
        std::shared_ptr<GeneratorData> m_WorkingDataBuffer, m_SwapBuffer;
        std::vector<MaskEntry> m_Masks;
        int m_SelectedMask = 0;
    };
} // namespace tf3d::generators
