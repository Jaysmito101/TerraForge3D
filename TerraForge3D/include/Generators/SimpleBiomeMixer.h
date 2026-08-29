#pragma once

#include "Base/Base.h"
#include "Base/RevisionTracker.h"
#include "Generators/BiomeManager.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Generators/GenerationContext.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)
TF3D_FWD_DEC_CLASS(ComputeShader, tf3d::base)

namespace tf3d::generators
{

    struct SimpleBiomeMixerSettings {
        bool enabled      = true;
        float strength    = 1.0f;
        bool useBiomeMask = false;
    };

    class SimpleBiomeMixer
    {
    public:
        struct State {
            std::unordered_map<BiomeID, SimpleBiomeMixerSettings, BiomeIDHash> biomeSettings;
        };
        using Snapshot = base::GeneratorState<State>::Snapshot;

        struct Runtime {
            std::shared_ptr<base::ComputeShader> shader;
        };

        SimpleBiomeMixer(data::ApplicationState *state);
        ~SimpleBiomeMixer();

        static bool Execute(const State *state,
                            const Runtime *runtime,
                            const GenerationContext *context,
                            const std::vector<BiomeManager::Snapshot> &biomes,
                            GeneratorData *heightmapData,
                            GeneratorData *swapBuffer);
        bool ShowSettings(const std::vector<std::shared_ptr<BiomeManager>> &biomeManagers);

        inline Snapshot GetState() const
        {
            return m_State.Capture();
        }

        inline Runtime GetRuntime() const
        {
            return Runtime{m_Shader};
        }

    private:
        data::ApplicationState *m_AppState = nullptr;
        std::shared_ptr<base::ComputeShader> m_Shader;
        base::GeneratorState<State> m_State;
    };

} // namespace tf3d::generators
