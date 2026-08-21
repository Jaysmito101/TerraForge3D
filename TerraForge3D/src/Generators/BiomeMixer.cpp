#include "Generators/BiomeMixer.h"
#include "Data/ApplicationState.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"

namespace tf3d::generators
{

    BiomeMixer::BiomeMixer(ApplicationState *appState)
        : m_SimpleBiomeMixer(std::make_shared<SimpleBiomeMixer>(appState)),
          m_State(m_UIState)
    {
        PublishState();
    }

    BiomeMixer::~BiomeMixer()
    {
    }

    void BiomeMixer::Update(const Snapshot *state,
                            const GenerationContext *context,
                            const std::vector<BiomeManager::State> &biomeStates,
                            const std::vector<std::shared_ptr<BiomeManager>> &biomeManagers,
                            GeneratorData *heightmapData,
                            GeneratorData *swapBuffer)
    {
        if (state == nullptr || context == nullptr || heightmapData == nullptr || swapBuffer == nullptr ||
            m_SimpleBiomeMixer == nullptr) {
            return;
        }

        switch (state->value.method) {
            case BiomeMixerMethod_Simple:
                m_SimpleBiomeMixer->Update(&state->value.simple,
                                           context,
                                           biomeStates,
                                           biomeManagers,
                                           heightmapData,
                                           swapBuffer);
                break;
            case BiomeMixerMethod_AlphaBlend:
                break;
            case BiomeMixerMethod_Count:
                break;
            default:
                break;
        }
        m_State.MarkProcessed(state->revision);
    }

    BiomeMixer::State BiomeMixer::CaptureState() const
    {
        State state = m_UIState;
        if (m_SimpleBiomeMixer != nullptr) {
            state.simple = m_SimpleBiomeMixer->GetState().value;
        }
        return state;
    }

    void BiomeMixer::PublishState()
    {
        m_UIState = CaptureState();
        m_State.Replace(m_UIState);
    }

    bool BiomeMixer::ShowSettings(const std::vector<std::shared_ptr<BiomeManager>> &biomeManagers)
    {
        ImGui::Text("Biome Mixer");

        static const char *s_Methods[] = {
            "Simple",
            "Alpha Blend"};

        int method = static_cast<int>(m_UIState.method);

        bool changed = ShowComboBox("Method##BiomeMixerMethod", &method, s_Methods, 2);

        m_UIState.method = static_cast<BiomeMixerMethod>(method);

        switch (m_UIState.method) {
            case BiomeMixerMethod_Simple:
                if (m_SimpleBiomeMixer != nullptr) {
                    changed |= m_SimpleBiomeMixer->ShowSettings(biomeManagers);
                }
                break;
            case BiomeMixerMethod_AlphaBlend:
                ImGui::Text("TODO");
                break;
            default:
                break;
        }

        if (changed) {
            PublishState();
        }
        return changed;
    }

} // namespace tf3d::generators
