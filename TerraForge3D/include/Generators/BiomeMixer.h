#pragma once

#include "Base/Base.h"
#include "Generators/BiomeManager.h"
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
        BiomeMixer(ApplicationState *appState);
        ~BiomeMixer();

        void Update(GeneratorData *heightmapData, GeneratorData *m_SwapBuffer);
        bool ShowSettings();

        inline bool IsUpdationRequired()
        {
            return m_RequireUpdation;
        }

    private:
        ApplicationState *m_AppState = nullptr;
        bool m_RequireUpdation       = true;
        BiomeMixerMethod m_Method    = BiomeMixerMethod_Simple;

        // This part could be sorted ot better with some inheritance
        // but thats not really worth it for now
        // maybe someone could create a pull request for this
        std::shared_ptr<SimpleBiomeMixer> m_SimpleBiomeMixer;
    };
} // namespace tf3d::generators
using tf3d::generators::BiomeMixer;
