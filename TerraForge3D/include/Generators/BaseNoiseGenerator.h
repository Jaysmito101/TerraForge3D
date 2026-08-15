#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Generators/Masks/MaskLayer.h"
#include "Inspector/CustomInspector.h"

#include <string_view>

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::generators
{

    class BaseNoiseGenerator
    {
    public:
        explicit BaseNoiseGenerator(tf3d::data::ApplicationState *appState);

        bool Initialize();
        bool ShowSettings();
        void Resize(int size);
        void Update(GeneratorData *sourceBuffer, GeneratorData *targetBuffer, GeneratorTexture *seedTexture);

        void Load(SerializerNode data);
        SerializerNode Save();

        inline bool RequireUpdation() const
        {
            return m_RequireUpdation;
        }

    private:
        data::ApplicationState *m_AppState = nullptr;
        bool m_RequireUpdation             = false;
        bool m_Enabled                     = true;
        bool m_UseMask                     = true;
        bool m_InvertMask                  = false;
        std::optional<base::ComputeShader> m_Shader;
        std::shared_ptr<inspector::CustomInspector> m_Inspector;
        std::shared_ptr<MaskLayer> m_MaskLayer;
    };

} // namespace tf3d::generators
