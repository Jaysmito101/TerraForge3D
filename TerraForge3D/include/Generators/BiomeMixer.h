#pragma once

#include "Base/Base.h"
#include "Base/RevisionTracker.h"
#include "Generators/BiomeManager.h"
#include "Generators/GenerationContext.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"

#include "Generators/SimpleBiomeMixer.h"

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::generators
{

    enum BiomeMixerMethod {
        BiomeMixerMethod_Simple = 0,
        BiomeMixerMethod_AlphaBlend,
        BiomeMixerMethod_Count
    };

    class BiomeMixer
    {
    public:
        struct State {
            BiomeMixerMethod method = BiomeMixerMethod_Simple;
            SimpleBiomeMixer::State simple;
        };
        using Snapshot = base::GeneratorState<State>::Snapshot;

        BiomeMixer(ApplicationState *appState);
        ~BiomeMixer();

        void Update(const Snapshot *state,
                    const GenerationContext *context,
                    const std::vector<BiomeManager::State> &biomeStates,
                    const std::vector<std::shared_ptr<BiomeManager>> &biomeManagers,
                    GeneratorData *heightmapData,
                    GeneratorData *swapBuffer);
        bool ShowSettings(const std::vector<std::shared_ptr<BiomeManager>> &biomeManagers);

        inline Snapshot GetState() const
        {
            return m_State.Capture();
        }

        inline bool IsUpdationRequired()
        {
            return m_State.RequiresUpdate();
        }

    private:
        State CaptureState() const;
        void PublishState();

        std::shared_ptr<SimpleBiomeMixer> m_SimpleBiomeMixer;
        State m_UIState;
        base::GeneratorState<State> m_State;
    };
} // namespace tf3d::generators
using tf3d::generators::BiomeMixer;
