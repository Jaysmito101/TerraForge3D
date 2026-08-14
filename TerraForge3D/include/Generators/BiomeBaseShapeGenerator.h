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

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::generators
{

    class BiomeBaseShapeGenerator
    {
    public:
        struct State {
            inspector::CustomInspectorSnapshot values;

            explicit State(inspector::CustomInspectorSnapshot snapshot)
                : values(std::move(snapshot))
            {
            }
        };
        using Snapshot = base::GeneratorState<State>::Snapshot;

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

        void Update(const Snapshot *state, const GenerationContext *context, GeneratorData *buffer);

        inline Snapshot GetState() const
        {
            return m_State.Capture();
        }
        inline Snapshot::Revision GetStateRevision() const
        {
            return m_State.PublishedRevision();
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
            return m_State.RequiresUpdate();
        }

    private:
        explicit BiomeBaseShapeGenerator(ApplicationState *appState)
            : m_AppState(appState),
              m_State(State{m_Inspector.Clone()})
        {
        }

        bool Initialize(const nlohmann::json &config, const std::string &source, const std::string &shaderPath);
        std::string BuildShaderSource(const std::string &templateSource,
                                      const std::string &uniformDeclarations);

    protected:
        ApplicationState *m_AppState = nullptr;
        std::optional<base::ComputeShader> m_Shader;
        inspector::CustomInspector m_Inspector;
        base::GeneratorState<State> m_State;

        std::string m_Name        = "";
        std::string m_ID          = "";
        std::string m_Description = "";
        std::string m_Source      = "";
        std::string m_ShaderPath  = "";
    };

} // namespace tf3d::generators
