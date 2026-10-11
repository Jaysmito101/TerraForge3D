#include "Generators/SimpleBiomeMixer.h"
#include "Base/Shader.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <utility>

namespace tf3d::generators
{

    SimpleBiomeMixer::SimpleBiomeMixer(data::ApplicationState *appState)
        : m_AppState(appState)
    {
        auto shader = m_AppState->resourceManager->LoadComputeShader("generation/biome_mixer/simple_mixer");
        if (shader.has_value()) {
            m_Shader = std::make_shared<base::ComputeShader>(std::move(*shader));
        }
    }

    SimpleBiomeMixer::~SimpleBiomeMixer()
    {
    }

    bool SimpleBiomeMixer::Execute(const State *state,
                                   const Runtime *runtime,
                                   const GenerationContext *context,
                                   const std::vector<BiomeManager::Snapshot> &biomes,
                                   GeneratorData *heightmapData,
                                   GeneratorData *swapBuffer)
    {
        TF3D_PROFILE_SCOPE_DOMAIN("generation/mixer/simple", PerformanceMonitor::Domain::Generation);
        if (state == nullptr || runtime == nullptr || runtime->shader == nullptr || context == nullptr ||
            heightmapData == nullptr || swapBuffer == nullptr) {
            return false;
        }

        const int workgroupSize = std::max(context->gpuWorkgroupSize, 1);
        const int resolution    = context->tileResolution;
        if (resolution <= 0) {
            return false;
        }
        const auto dispatchSize = (resolution + workgroupSize - 1) / workgroupSize;

        runtime->shader->Bind();
        swapBuffer->Bind(1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        TF3D_PROFILE_GPU_SCOPE("generation/mixer/simple/gpu");

        // clear the buffer
        runtime->shader->SetUniform1i("u_Resolution", resolution);
        runtime->shader->SetUniform1i("u_Mode", 0);
        runtime->shader->Dispatch(dispatchSize, dispatchSize, 1);
        runtime->shader->SetMemoryBarrier();

        // mix the biomes
        runtime->shader->SetUniform1i("u_Mode", 1);
        const size_t biomeCount = biomes.size();
        for (size_t biomeIndex = 0; biomeIndex < biomeCount; ++biomeIndex) {
            const auto &biomeSnapshot = biomes[biomeIndex];
            const auto &biomeRuntime  = biomeSnapshot.runtime;
            const auto &biomeState    = biomeSnapshot.state;
            if (!biomeState.enabled || biomeRuntime.data == nullptr) {
                continue;
            }

            const auto settingsIt    = state->biomeSettings.find(biomeRuntime.id);
            const auto biomeSettings = settingsIt != state->biomeSettings.end()
                                           ? settingsIt->second
                                           : SimpleBiomeMixerSettings{};
            if (!biomeSettings.enabled) {
                continue;
            }

            auto *biomeData = biomeRuntime.data.get();
            biomeData->Bind(0);
            runtime->shader->SetUniform1f("u_Strength", biomeSettings.strength);
            auto *maskTexture       = biomeSettings.useBiomeMask ? biomeRuntime.maskTexture.get() : nullptr;
            const bool useBiomeMask = maskTexture != nullptr;
            runtime->shader->SetUniform1i("u_UseBiomeMask", useBiomeMask ? 1 : 0);
            if (useBiomeMask) {
                maskTexture->Bind(3);
                runtime->shader->SetUniform1i("u_BiomeMask", 3);
            }
            runtime->shader->Dispatch(dispatchSize, dispatchSize, 1);
            runtime->shader->SetMemoryBarrier();
        }

        swapBuffer->CopyTo(heightmapData);
        return true;
    }

    bool SimpleBiomeMixer::ShowSettings(const std::vector<std::shared_ptr<BiomeManager>> &biomeManagers)
    {
        ImGui::Text("Biomes");
        return m_State.Edit([&](State &state) {
            bool changed = false;

            for (const auto &biomeManager : biomeManagers) {
                if (biomeManager == nullptr) {
                    continue;
                }

                const auto &biomeID = biomeManager->GetBiomeID();
                ImGui::PushID(biomeID.c_str());

                if (ImGui::CollapsingHeader(biomeManager->GetBiomeName())) {
                    const auto settings = state.biomeSettings.find(biomeID);
                    auto biomeSettings  = settings != state.biomeSettings.end()
                                              ? settings->second
                                              : SimpleBiomeMixerSettings{};
                    bool biomeChanged   = false;

                    biomeChanged |= ImGui::Checkbox("Enabled", &biomeSettings.enabled);
                    biomeChanged |= ImGui::SliderFloat("Strength", &biomeSettings.strength, 0.0f, 1.0f);
                    biomeChanged |= ImGui::Checkbox("Use Biome Mask", &biomeSettings.useBiomeMask);
                    if (biomeChanged) {
                        state.biomeSettings[biomeID] = biomeSettings;
                        changed                      = true;
                    }
                }

                ImGui::PopID();
            }

            return changed;
        });
    }

    SerializerNode SimpleBiomeMixer::Save(
        const std::vector<std::shared_ptr<BiomeManager>> &biomeManagers) const
    {
        auto settings = CreateSerializerNode();
        std::vector<SerializerNode> biomeSettings;
        biomeSettings.reserve(biomeManagers.size());

        const auto state = m_State.Capture();
        for (const auto &biome : biomeManagers) {
            if (biome == nullptr) {
                continue;
            }
            const BiomeID &biomeId               = biome->GetBiomeID();
            const auto found                     = state.value.biomeSettings.find(biomeId);
            const SimpleBiomeMixerSettings value = found != state.value.biomeSettings.end()
                                                       ? found->second
                                                       : SimpleBiomeMixerSettings{};
            auto entry                           = CreateSerializerNode();
            entry->Set("BiomeID", biomeId.GetValue());
            entry->Set("Enabled", value.enabled);
            entry->Set("Strength", value.strength);
            entry->Set("UseBiomeMask", value.useBiomeMask);
            biomeSettings.push_back(std::move(entry));
        }
        settings->Set("Biomes", biomeSettings);
        return settings;
    }

    bool SimpleBiomeMixer::Load(
        const SerializerNode &settings,
        const std::vector<std::shared_ptr<BiomeManager>> &biomeManagers,
        std::string &error)
    {
        error.clear();
        if (settings == nullptr || !settings->HasKey("Biomes")) {
            error = "Simple mixer settings require a Biomes array.";
            return false;
        }

        std::unordered_set<std::string> knownBiomes;
        for (const auto &biome : biomeManagers) {
            if (biome != nullptr) {
                knownBiomes.insert(biome->GetBiomeID().GetValue());
            }
        }

        State next;
        std::unordered_set<std::string> seen;
        for (const auto &entry : settings->Get<std::vector<SerializerNode>>("Biomes")) {
            if (entry == nullptr) {
                error = "Simple mixer Biomes entries must be objects.";
                return false;
            }
            const std::string biomeId = entry->Get<std::string>("BiomeID", "");
            const float strength      = entry->Get<float>("Strength", 1.0f);
            if (!knownBiomes.contains(biomeId)) {
                error = "Simple mixer settings reference an unknown BiomeID.";
                return false;
            }
            if (!seen.insert(biomeId).second) {
                error = "Simple mixer settings contain a duplicate BiomeID.";
                return false;
            }
            if (!std::isfinite(strength) || strength < 0.0f || strength > 1.0f) {
                error = "Simple mixer Strength must be finite and between 0 and 1.";
                return false;
            }
            next.biomeSettings.emplace(
                BiomeID{biomeId},
                SimpleBiomeMixerSettings{
                    entry->Get<bool>("Enabled", true),
                    strength,
                    entry->Get<bool>("UseBiomeMask", false)});
        }

        m_State.Replace(std::move(next));
        return true;
    }

} // namespace tf3d::generators
