#include "API/Operations/GenerationOperations.h"

#include "API/ApiResult.h"
#include "API/OperationRegistry.h"
#include "API/SchemaTemplate.h"
#include "Data/ApplicationState.h"
#include "Generators/GenerationManager.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace tf3d::api
{

    namespace
    {
        constexpr std::size_t MaxTrackedRequests = 128;

        enum class RequestStatus {
            Queued,
            Running,
            Published,
            Superseded,
            Failed
        };

        enum class GenerationState {
            Idle,
            Queued,
            Running,
            Failed
        };

        struct TrackedGenerationRequest {
            std::string id;
            uint64_t inputRevision = 0;
            std::optional<RequestStatus> terminalStatus;
            nlohmann::json terminalOutput  = nullptr;
            nlohmann::json terminalFailure = nullptr;
        };

        struct GenerationApiState {
            std::deque<TrackedGenerationRequest> requests;
        };

        std::string RevisionString(uint64_t revision)
        {
            return std::to_string(revision);
        }

        nlohmann::json OptionalRevision(uint64_t revision)
        {
            return revision == 0 ? nlohmann::json(nullptr) : nlohmann::json(RevisionString(revision));
        }

        nlohmann::json FiniteNumber(float value)
        {
            return std::isfinite(value) ? nlohmann::json(value) : nlohmann::json(nullptr);
        }

        nlohmann::json BuildStatistics(const generators::GenerationLifecycleSnapshot &snapshot)
        {
            const auto &statistics = snapshot.statistics;
            if (!snapshot.canvas.hasOutput || !statistics.valid) {
                return nullptr;
            }

            nlohmann::json histogram = nlohmann::json::array();
            histogram.get_ref<nlohmann::json::array_t &>().reserve(statistics.histogram.size());
            for (const float bin : statistics.histogram) {
                histogram.push_back(FiniteNumber(bin));
            }

            nlohmann::json percentiles = nlohmann::json::array();
            for (const float percentile : statistics.percentiles) {
                percentiles.push_back(FiniteNumber(percentile));
            }

            return {
                {"terrainRevision", RevisionString(snapshot.terrainRevision)},
                {"minimum", FiniteNumber(statistics.minimum)},
                {"maximum", FiniteNumber(statistics.maximum)},
                {"histogramValid", statistics.histogramValid},
                {"histogram", std::move(histogram)},
                {"percentiles", std::move(percentiles)},
                {"requestedPercentile", FiniteNumber(statistics.requestedPercentile)}};
        }

        nlohmann::json BuildOutput(const generators::GenerationLifecycleSnapshot &snapshot)
        {
            if (!snapshot.canvas.hasOutput) {
                return nullptr;
            }

            return {
                {"terrainRevision", RevisionString(snapshot.terrainRevision)},
                {"inputRevision", RevisionString(snapshot.outputInputRevision)},
                {"resolution", snapshot.outputTileResolution},
                {"tileSize", FiniteNumber(snapshot.outputTileSize)},
                {"isCurrent", snapshot.canvas.outputIsCurrent},
                {"stale", !snapshot.canvas.outputIsCurrent}};
        }

        nlohmann::json BuildFailure(uint64_t revision)
        {
            if (revision == 0) {
                return nullptr;
            }
            return {
                {"inputRevision", RevisionString(revision)},
                {"code", "generation_failed"},
                {"message", "Generation completed without publishing output."}};
        }

        TrackedGenerationRequest *FindRequest(GenerationApiState &state, std::string_view requestId)
        {
            const auto request = std::find_if(
                state.requests.begin(), state.requests.end(), [requestId](const auto &candidate) {
                    return candidate.id == requestId;
                });
            return request == state.requests.end() ? nullptr : &*request;
        }

        RequestStatus GetRequestStatus(const TrackedGenerationRequest &request,
                                       const generators::GenerationLifecycleSnapshot &snapshot)
        {
            if (snapshot.canvas.hasOutput && snapshot.outputInputRevision == request.inputRevision) {
                return RequestStatus::Published;
            }
            if (snapshot.lastFailedRevision == request.inputRevision) {
                return RequestStatus::Failed;
            }
            if (snapshot.activeInputRevision == request.inputRevision) {
                return RequestStatus::Running;
            }
            if (snapshot.inputRevision > request.inputRevision) {
                return RequestStatus::Superseded;
            }
            if (snapshot.inputIsDirty) {
                return RequestStatus::Queued;
            }
            return RequestStatus::Failed;
        }

        std::string_view RequestStatusName(RequestStatus status)
        {
            switch (status) {
                case RequestStatus::Queued:
                    return "queued";
                case RequestStatus::Running:
                    return "running";
                case RequestStatus::Published:
                    return "published";
                case RequestStatus::Superseded:
                    return "superseded";
                case RequestStatus::Failed:
                    return "failed";
            }
            return "unknown";
        }

        std::string_view RequestStage(RequestStatus status)
        {
            switch (status) {
                case RequestStatus::Queued:
                    return "queued";
                case RequestStatus::Running:
                    return "generation";
                case RequestStatus::Published:
                case RequestStatus::Superseded:
                case RequestStatus::Failed:
                    return "terminal";
            }
            return "unknown";
        }

        nlohmann::json BuildRequestStatus(TrackedGenerationRequest &request,
                                          const generators::GenerationLifecycleSnapshot &snapshot)
        {
            const RequestStatus outcome = request.terminalStatus.value_or(GetRequestStatus(request, snapshot));
            if (!request.terminalStatus && outcome != RequestStatus::Queued && outcome != RequestStatus::Running) {
                request.terminalStatus  = outcome;
                request.terminalOutput  = outcome == RequestStatus::Published
                                              ? BuildOutput(snapshot)
                                              : nlohmann::json(nullptr);
                request.terminalFailure = outcome == RequestStatus::Failed
                                              ? BuildFailure(request.inputRevision)
                                              : nlohmann::json(nullptr);
            }
            const bool published = outcome == RequestStatus::Published;
            const bool failed    = outcome == RequestStatus::Failed;

            return {
                {"requestId", request.id},
                {"status", std::string(RequestStatusName(outcome))},
                {"inputRevision", RevisionString(request.inputRevision)},
                {"progress", {{"stage", std::string(RequestStage(outcome))},
                               {"percent", published ? nlohmann::json(100) : nlohmann::json(nullptr)}}},
                {"output", published ? request.terminalOutput : nlohmann::json(nullptr)},
                {"failure", failed ? request.terminalFailure : nlohmann::json(nullptr)},
                {"cancellationSupported", false}};
        }

        nlohmann::json BuildGenerationStatus(const generators::GenerationLifecycleSnapshot &snapshot,
                                             GenerationApiState &apiState)
        {
            GenerationState state = GenerationState::Idle;
            if (snapshot.canvas.isGenerating) {
                state = GenerationState::Running;
            } else if (snapshot.inputIsDirty) {
                state = snapshot.lastFailedRevision == snapshot.inputRevision
                            ? GenerationState::Failed
                            : GenerationState::Queued;
            }

            std::string stateName = "idle";
            switch (state) {
                case GenerationState::Idle:
                    stateName = "idle";
                    break;
                case GenerationState::Queued:
                    stateName = "queued";
                    break;
                case GenerationState::Running:
                    stateName = "running";
                    break;
                case GenerationState::Failed:
                    stateName = "failed";
                    break;
            }
            std::string stageName = stateName;
            if (state == GenerationState::Running) {
                stageName = "generation";
            }

            nlohmann::json latestRequest = nullptr;
            if (!apiState.requests.empty()) {
                latestRequest = BuildRequestStatus(apiState.requests.back(), snapshot);
            }

            return {
                {"available", true},
                {"generation", {{"state", stateName}, {"stage", stageName}, {"progressPercent", state == GenerationState::Queued ? nlohmann::json(0) : nlohmann::json(nullptr)}, {"activeInputRevision", OptionalRevision(snapshot.activeInputRevision)}, {"inputRevision", RevisionString(snapshot.inputRevision)}, {"paused", snapshot.updatesPaused}}},
                {"output", BuildOutput(snapshot)},
                {"statistics", BuildStatistics(snapshot)},
                {"lastFailedRevision", OptionalRevision(snapshot.lastFailedRevision)},
                {"lastFailure", BuildFailure(snapshot.lastFailedRevision)},
                {"latestRequest", std::move(latestRequest)},
                {"cancellation", {{"supported", false}, {"supersession", "Newer input revisions prevent stale worker results from being treated as current."}}}};
        }

        ApiResult GetGenerationStatus(tf3d::data::ApplicationState *applicationState,
                                      GenerationApiState &apiState)
        {
            if (applicationState == nullptr || applicationState->generationManager == nullptr) {
                return ApiResult::Failure(ApiErrorType::Shutdown,
                                          "Generation state is not available.");
            }

            return ApiResult::Success(
                BuildGenerationStatus(applicationState->generationManager->GetLifecycleSnapshot(), apiState));
        }

        ApiResult RequestGeneration(tf3d::data::ApplicationState *applicationState,
                                    GenerationApiState &apiState)
        {
            if (applicationState == nullptr || applicationState->generationManager == nullptr) {
                return ApiResult::Success({{"admission", "unavailable"},
                                           {"requestId", nullptr},
                                           {"reason", "generation_unavailable"}});
            }

            auto *manager                     = applicationState->generationManager;
            const auto before                 = manager->GetLifecycleSnapshot();
            const bool retryingFailedRevision = before.inputIsDirty &&
                                                before.lastFailedRevision == before.inputRevision;
            if (before.canvas.isGenerating || (before.inputIsDirty && !retryingFailedRevision) ||
                before.updatesPaused) {
                const std::string reason = before.updatesPaused
                                               ? "generation_updates_paused"
                                               : "generation_busy";
                return ApiResult::Success({{"admission", "busy"},
                                           {"requestId", nullptr},
                                           {"reason", reason}});
            }

            manager->MarkForRegeneration();
            const auto accepted         = manager->GetLifecycleSnapshot();
            const std::string requestId = "generation-" + RevisionString(accepted.inputRevision);
            apiState.requests.push_back({requestId, accepted.inputRevision});
            while (apiState.requests.size() > MaxTrackedRequests) {
                apiState.requests.pop_front();
            }

            return ApiResult::Success({{"admission", "accepted"},
                                       {"requestId", requestId},
                                       {"reason", ""}});
        }

        ApiResult GetGenerationRequest(tf3d::data::ApplicationState *applicationState,
                                       GenerationApiState &apiState,
                                       const nlohmann::json &arguments)
        {
            const std::string requestId       = arguments.at("RequestId").get<std::string>();
            TrackedGenerationRequest *request = FindRequest(apiState, requestId);
            if (request == nullptr) {
                return ApiResult::Failure(ApiErrorType::GenerationRequestNotFound,
                                          "The generation request is unknown or no longer retained.",
                                          {{"requestId", requestId}});
            }
            if (applicationState == nullptr || applicationState->generationManager == nullptr) {
                return ApiResult::Failure(ApiErrorType::Shutdown,
                                          "Generation state is not available.");
            }

            return ApiResult::Success(
                BuildRequestStatus(*request,
                                   applicationState->generationManager->GetLifecycleSnapshot()));
        }
    } // namespace

    void RegisterGenerationOperations(OperationRegistry &operations,
                                      tf3d::data::ApplicationState *applicationState)
    {
        auto apiState = std::make_shared<GenerationApiState>();

        RegisterOperationFromJson(
            operations,
            ApiSchemaTemplate::ComposeDefault("Operations/Generation/Definitions/Status.json"),
            [applicationState, apiState](const nlohmann::json &) {
                return GetGenerationStatus(applicationState, *apiState);
            });

        RegisterOperationFromJson(
            operations,
            ApiSchemaTemplate::ComposeDefault("Operations/Generation/Definitions/Request.json"),
            [applicationState, apiState](const nlohmann::json &) {
                return RequestGeneration(applicationState, *apiState);
            });

        RegisterOperationFromJson(
            operations,
            ApiSchemaTemplate::ComposeDefault("Operations/Generation/Definitions/GetRequest.json"),
            [applicationState, apiState](const nlohmann::json &arguments) {
                return GetGenerationRequest(applicationState, *apiState, arguments);
            });
    }

} // namespace tf3d::api
