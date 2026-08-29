#pragma once

#include "Base/Base.h"
#include "Base/RevisionTracker.h"
#include "Generators/BiomeManager.h"
#include "Generators/GenerationContext.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"

#include "Generators/SimpleBiomeMixer.h"

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

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
        };
        using StateSnapshot = base::GeneratorState<State>::Snapshot;

        struct Snapshot {
            State value;
            StateSnapshot::Revision revision = 0;
            SimpleBiomeMixer::Snapshot simple;
        };

        struct Runtime {
            SimpleBiomeMixer::Runtime simple;
        };

        BiomeMixer(data::ApplicationState *appState);
        ~BiomeMixer();

        static bool Execute(const Snapshot *state,
                            const Runtime *runtime,
                            const GenerationContext *context,
                            const std::vector<BiomeManager::Snapshot> &biomes,
                            GeneratorData *heightmapData,
                            GeneratorData *swapBuffer);
        bool ShowSettings(const std::vector<std::shared_ptr<BiomeManager>> &biomeManagers);

        inline Runtime GetRuntime() const
        {
            return Runtime{m_SimpleBiomeMixer != nullptr ? m_SimpleBiomeMixer->GetRuntime()
                                                         : SimpleBiomeMixer::Runtime{}};
        }

        inline Snapshot GetState() const
        {
            const auto state = m_State.Capture();
            return Snapshot{state.value,
                            state.revision,
                            m_SimpleBiomeMixer != nullptr ? m_SimpleBiomeMixer->GetState()
                                                          : SimpleBiomeMixer::Snapshot{}};
        }

    private:
        std::shared_ptr<SimpleBiomeMixer> m_SimpleBiomeMixer;
        base::GeneratorState<State> m_State;
    };
} // namespace tf3d::generators
