#pragma once

#include "Base/Base.h"
#include "Base/RevisionTracker.h"
#include "Exporters/Serializer.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Generators/GenerationContext.h"
#include "Inspector/CustomInspector.h"
#include "Inspector/CustomInspectorSnapshot.h"

#include <nlohmann/json.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::generators
{

    class BiomeBaseShapeGenerator
    {
    public:
        using Revision = base::RevisionTracker::Revision;

        struct State {
            State(Revision revision, inspector::CustomInspectorSnapshot values)
                : revision(revision), values(std::move(values))
            {
            }

            Revision revision;
            inspector::CustomInspectorSnapshot values;
        };

        static inline std::shared_ptr<BiomeBaseShapeGenerator> Create(ApplicationState *appState,
                                                                      const nlohmann::json &config,
                                                                      const std::string &source,
                                                                      const std::string &shaderPath)
        {
            std::shared_ptr<BiomeBaseShapeGenerator> generator(
                new BiomeBaseShapeGenerator(appState));
            if (!generator->Initialize(config, source, shaderPath)) {
                return nullptr;
            }
            return generator;
        }
        ~BiomeBaseShapeGenerator() = default;

        bool ShowSettings();

        bool Load(SerializerNode data);
        SerializerNode Save() const;

        void Update(const State *state, const GenerationContext *context, GeneratorData *buffer);

        inline State GetState() const
        {
            return State(m_UpdateTracker.PublishedRevision(), m_Inspector.Clone());
        }
        inline Revision GetStateRevision() const
        {
            return m_UpdateTracker.PublishedRevision();
        }

        inline const std::string &GetName() const
        {
            return m_Name;
        }
        inline const std::string &GetID() const
        {
            return m_ID;
        }
        inline const std::string &GetDescription() const
        {
            return m_Description;
        }
        inline bool RequireUpdation() const
        {
            return m_UpdateTracker.RequiresUpdate();
        }

    private:
        explicit BiomeBaseShapeGenerator(ApplicationState *appState)
            : m_AppState(appState) { }

        bool Initialize(const nlohmann::json &config, const std::string &source, const std::string &shaderPath);
        std::string BuildShaderSource(const std::string &templateSource,
                                      const std::string &uniformDeclarations);

    protected:
        std::optional<base::ComputeShader> m_Shader;
        inspector::CustomInspector m_Inspector;
        base::RevisionTracker m_UpdateTracker;

        ApplicationState *m_AppState = nullptr;
        std::string m_Name           = "";
        std::string m_ID             = "";
        std::string m_Description    = "";
        std::string m_Source         = "";
        std::string m_ShaderPath     = "";
    };

} // namespace tf3d::generators
