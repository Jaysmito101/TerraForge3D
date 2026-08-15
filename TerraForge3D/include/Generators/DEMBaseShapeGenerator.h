#pragma once

#include "Base/Base.h"
#include "Base/RevisionTracker.h"
#include "Exporters/Serializer.h"
#include "Generators/DEM/DEMTileTypes.h"
#include "Generators/GenerationContext.h"
#include "Generators/GeneratorData.h"
#include "Utils/Utils.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <string>

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)
TF3D_FWD_DEC_CLASS(Renderer, tf3d::generators::dem)
TF3D_FWD_DEC_CLASS(TileSource, tf3d::generators::dem)

namespace tf3d::generators
{

    class DEMBaseShapeGenerator
    {
    public:
        struct State {
            int32_t zoomResolution  = 0;
            float zoomOnMap         = 1.0f;
            float mapStrength       = 1.0f;
            glm::vec2 mapCenter     = glm::vec2(0.0f);
            bool autoZoomResolution = true;
            bool allowTileRequests  = true;
        };
        using Snapshot = base::GeneratorState<State>::Snapshot;

        DEMBaseShapeGenerator(ApplicationState *appState);
        ~DEMBaseShapeGenerator();

        bool ShowSettings();
        void Update(const Snapshot *state, const GenerationContext *context, GeneratorData *buffer);

        inline Snapshot GetState() const
        {
            return m_State.Capture();
        }
        inline Snapshot::Revision GetStateRevision() const
        {
            return m_State.PublishedRevision();
        }

        void Load(SerializerNode data);
        SerializerNode Save();

        inline bool RequireUpdation() const
        {
            return m_State.RequiresUpdate();
        }

    private:
        static constexpr int32_t kMaxVisibleTiles        = 64;
        static constexpr int32_t kViewSettleMilliseconds = 75;
        static constexpr float kMaxMapStrength           = 100.0f;

        struct ViewTileStatus {
            Snapshot::Revision revision = 0;
            int visible                 = 0;
            int ready                   = 0;
            int pending                 = 0;
            int blocked                 = 0;
        };

        void MarkViewInteraction(State &settings);
        ViewTileStatus GetViewTileStatus() const;
        int32_t GetEffectiveZoomResolution(const State &state) const;
        void GetVisibleTileRange(const State &state, int32_t zoomResolution,
                                 int32_t &minTileX, int32_t &maxTileX,
                                 int32_t &minTileY, int32_t &maxTileY) const;
        int32_t GetVisibleTileCount(const State &state, int32_t zoomResolution) const;

    private:
        base::GeneratorState<State> m_State;
        // Main-thread editing state; avoids locking m_State on every UI frame.
        State m_UIState;

        std::unique_ptr<dem::TileSource> m_Tiles;
        std::unique_ptr<dem::Renderer> m_Renderer;

        // Values derived while generating or displayed by the settings panel.
        int32_t m_EffectiveZoomResolution = 0;
        int m_TilesUsingCount             = 0;
        int m_TilesFallbackCount          = 0;
        int m_VisibleTileCount            = 0;
        int m_TilesSkippedCount           = 0;

        mutable std::mutex m_ViewTileStatusMutex;
        ViewTileStatus m_ViewTileStatus;

        std::chrono::steady_clock::time_point m_LastViewInteractionTime = std::chrono::steady_clock::time_point::min();
        bool m_ViewInteractionPending                                   = false;
        char m_APIKeyInput[1024]                                        = {};
    };

} // namespace tf3d::generators
using tf3d::generators::DEMBaseShapeGenerator;
