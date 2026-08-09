#pragma once

#include "Base/Base.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Inspector/CustomInspector.h"

#include <vector>

namespace tf3d::base
{
    class ComputeShader;
}
using tf3d::base::ComputeShader;
namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::generators
{

    class CalculatedMaskGenerator
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

        explicit CalculatedMaskGenerator(ApplicationState *state,
                                         std::string defaultTypeID = "HeightRange");
        ~CalculatedMaskGenerator();

        CalculatedMaskGenerator(const CalculatedMaskGenerator &)            = delete;
        CalculatedMaskGenerator &operator=(const CalculatedMaskGenerator &) = delete;
        CalculatedMaskGenerator(CalculatedMaskGenerator &&) noexcept = default;
        CalculatedMaskGenerator &operator=(CalculatedMaskGenerator &&) noexcept = default;

        void Resize(int size);
        void Invalidate();
        bool ShowSettings();
        bool Update(GeneratorData *sourceData);
        SerializerNode Save() const;
        void Load(SerializerNode data);

        inline GeneratorTexture *GetTexture() const
        {
            return m_Texture.get();
        }

    private:
        bool LoadMetadata();
        bool BuildShader(const std::string &baseShaderSource);
        int GetSelectedTypeIndex() const;
        int GetShaderModeForType(int typeIndex) const;
        int FindTypeIndexByID(const std::string &id) const;
        int FindTypeIndexByMode(int mode) const;

        ApplicationState *m_AppState = nullptr;
        std::optional<ComputeShader> m_Shader;
        std::shared_ptr<GeneratorTexture> m_Texture;
        std::shared_ptr<CustomInspector> m_Inspector;
        std::vector<AlgorithmDefinition> m_Algorithms;
        std::string m_DefaultTypeID  = "HeightRange";
        int m_Size                   = 256;
        int m_SelectedAlgorithmIndex = -1;
        bool m_Dirty                 = true;
        bool m_MetadataLoaded        = false;
    };

} // namespace tf3d::generators
