#include "Generators/SimpleBiomeMixer.h"
#include "Base/Shader.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"

#include <algorithm>
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
        TF3D_PROFILE_VALUE_DOMAIN("generation/mixer/dispatch", dispatchSize, dispatchSize,
                                  static_cast<uint64_t>(biomes.size()), PerformanceMonitor::Domain::Generation);

        runtime->shader->Bind();
        swapBuffer->Bind(1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        // clear the buffer
        runtime->shader->SetUniform1i("u_Resolution", resolution);
        runtime->shader->SetUniform1i("u_Mode", 0);
        {
            const std::string gpuKey = "generation/mixer/simple/clear/gpu";
            TF3D_PROFILE_GPU_SCOPE(gpuKey);
            runtime->shader->Dispatch(dispatchSize, dispatchSize, 1);
            runtime->shader->SetMemoryBarrier();
        }

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
            {
                const std::string gpuKey = std::string("generation/mixer/simple/biome/") +
                                           biomeRuntime.name + "/gpu";
                TF3D_PROFILE_GPU_SCOPE(gpuKey);
                runtime->shader->Dispatch(dispatchSize, dispatchSize, 1);
                runtime->shader->SetMemoryBarrier();
            }
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

} // namespace tf3d::generators
