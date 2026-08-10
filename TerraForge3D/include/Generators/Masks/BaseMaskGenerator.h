#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Inspector/CustomInspector.h"

#include <string>
#include <string_view>
#include <vector>

TF3D_FWD_DEC_CLASS(ComputeShader, tf3d::base)
TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::generators
{

    class BaseMaskGenerator
    {
    public:
        struct AlgorithmDefinition {
            std::string id;
            std::string label;
            std::string description;
            std::string shaderPath;
            std::string shaderSource;
            int selectionValue = -1;
            int runtimeMode    = -1;
        };

        explicit BaseMaskGenerator(tf3d::data::ApplicationState *state,
                                   std::string defaultTypeID = "None");
        ~BaseMaskGenerator();

        BaseMaskGenerator(const BaseMaskGenerator &)                = delete;
        BaseMaskGenerator &operator=(const BaseMaskGenerator &)     = delete;
        BaseMaskGenerator(BaseMaskGenerator &&) noexcept            = default;
        BaseMaskGenerator &operator=(BaseMaskGenerator &&) noexcept = default;

        bool ShowSettings();
        SerializerNode Save() const;
        void Load(SerializerNode data);
        bool SetTypeID(std::string_view typeID);
        bool IsNone() const;

        void ApplyToShader(tf3d::base::ComputeShader &shader) const;

        inline bool IsAvailable() const
        {
            return m_MetadataLoaded && m_Inspector != nullptr && !m_ShaderSource.empty();
        }

        inline const std::string &GetShaderSource() const
        {
            return m_ShaderSource;
        }

    private:
        bool LoadMetadata();
        bool BuildShaderSource(const std::string &baseShaderSource);
        int GetSelectedTypeIndex() const;
        int GetShaderModeForType(int typeIndex) const;
        int FindTypeIndexByID(const std::string &id) const;
        int FindTypeIndexByMode(int mode) const;

        tf3d::data::ApplicationState *m_AppState = nullptr;
        std::shared_ptr<inspector::CustomInspector> m_Inspector;
        std::vector<AlgorithmDefinition> m_Algorithms;
        std::string m_DefaultTypeID = "None";
        std::string m_ShaderSource;
        int m_SelectedAlgorithmIndex = -1;
        bool m_MetadataLoaded        = false;
    };

} // namespace tf3d::generators
