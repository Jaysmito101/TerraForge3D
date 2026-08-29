#include "Generators/BiomeMixer.h"
#include "Data/ApplicationState.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"

namespace tf3d::generators
{

    BiomeMixer::BiomeMixer(data::ApplicationState *appState)
        : m_SimpleBiomeMixer(std::make_shared<SimpleBiomeMixer>(appState)),
          m_State(State{})
    {
    }

    BiomeMixer::~BiomeMixer()
    {
    }

    bool BiomeMixer::Execute(const Snapshot *snapshot,
                             const Runtime *runtime,
                             const GenerationContext *context,
                             const std::vector<BiomeManager::Snapshot> &biomes,
                             GeneratorData *heightmapData,
                             GeneratorData *swapBuffer)
    {
        if (snapshot == nullptr || context == nullptr || heightmapData == nullptr || swapBuffer == nullptr ||
            runtime == nullptr || runtime->simple.shader == nullptr) {
            return false;
        }

        bool producedOutput = false;
        switch (snapshot->value.method) {
            case BiomeMixerMethod_Simple:
                producedOutput = SimpleBiomeMixer::Execute(&snapshot->simple.value,
                                                           &runtime->simple,
                                                           context,
                                                           biomes,
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
        return producedOutput;
    }

    bool BiomeMixer::ShowSettings(const std::vector<std::shared_ptr<BiomeManager>> &biomeManagers)
    {
        ImGui::Text("Biome Mixer");

        static const char *s_Methods[] = {
            "Simple",
            "Alpha Blend"};

        const auto stateSnapshot = m_State.Capture();
        int method              = static_cast<int>(stateSnapshot.value.method);

        bool changed = ShowComboBox("Method##BiomeMixerMethod", &method, s_Methods, 2);
        const auto selectedMethod = static_cast<BiomeMixerMethod>(method);

        if (selectedMethod != stateSnapshot.value.method) {
            changed |= m_State.Edit([selectedMethod](State &state) {
                state.method = selectedMethod;
                return true;
            });
        }

        switch (selectedMethod) {
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

        return changed;
    }

} // namespace tf3d::generators
