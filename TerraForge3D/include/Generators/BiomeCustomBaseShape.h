#pragma once

#include "Base/Base.h"
#include "Base/RevisionTracker.h"
#include "Exporters/Serializer.h"
#include "Generators/GeneratorData.h"
#include "Generators/GenerationContext.h"
#include "Generators/Masks/MaskLayer.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::generators
{

    class BiomeCustomizeBaseShape
    {
    private:
        struct RuntimeState;

    public:
        struct MaskState {
            std::string name;
            bool enabled    = true;
            bool raise      = true;
            float strength  = 1.0f;
            float smoothing = 0.0f;
            MaskLayer::State maskState;

            MaskState(std::string layerName, MaskLayer::State state);
        };

        struct State {
            bool enabled          = false;
            bool flattenBaseShape = false;
            std::vector<MaskState> masks;
        };

        struct Snapshot {
            using Revision = base::RevisionTracker::Revision;

            State value;
            Revision revision = 0;

        private:
            std::shared_ptr<const RuntimeState> runtime;
            friend class BiomeCustomizeBaseShape;
        };

        explicit BiomeCustomizeBaseShape(tf3d::data::ApplicationState *appState);
        ~BiomeCustomizeBaseShape();

        bool ShowSettings();
        void Update(const Snapshot *state,
                    const GenerationContext *context,
                    GeneratorData *baseShapeBuffer);

        Snapshot GetState() const;
        inline Snapshot::Revision GetStateRevision() const
        {
            return m_State.PublishedRevision();
        }
        inline bool RequireUpdation() const
        {
            return m_State.RequiresUpdate();
        }

        SerializerNode Save() const;
        void Load(SerializerNode node);

        inline bool IsEnabled() const
        {
            return m_UIState.enabled;
        }

        void Resize();

    private:
        std::shared_ptr<MaskLayer> CreateMaskLayer() const;
        MaskState CreateMaskState(const std::string &name);
        void AddMaskLayer();
        bool ShowDrawingSettings();
        State CaptureState() const;
        bool ApplyLayer(GeneratorData *data,
                        const GenerationContext *context,
                        const MaskState *layer,
                        const MaskLayer *maskLayer, bool flattenSource);
        bool UpdateLayerMask(const MaskState &layer,
                             const GenerationContext *context,
                             const std::shared_ptr<MaskLayer> &maskLayer,
                             GeneratorData *source);

        struct RuntimeState {
            std::vector<std::shared_ptr<MaskLayer>> masks;
        };

    private:
        data::ApplicationState *m_AppState = nullptr;
        std::optional<base::ComputeShader> m_Shader;
        State m_UIState;
        base::GeneratorState<State> m_State;
        std::vector<std::shared_ptr<MaskLayer>> m_RuntimeMasks;
        int m_SelectedMask = 0;
    };
} // namespace tf3d::generators
