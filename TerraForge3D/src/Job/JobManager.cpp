#include "Job/JobManager.h"
#include "Data/ApplicationState.h"
#include "Job/JobSystem.h"

static void ShowJob(tf3d::job::Job *job)
{
    ImGui::PushID((job->name + std::to_string(job->id)).data());
    if (ImGui::CollapsingHeader(job->name.data())) {
        ImGui::Text("%s", job->description.data());
    }
    ImGui::PopID();
}

namespace tf3d::job
{

    JobManager::JobManager(ApplicationState *appState)
    {
        this->m_AppState = appState;
    }

    JobManager::~JobManager()
    {
    }

    bool *JobManager::IsWindowOpenPtr()
    {
        return m_AppState != nullptr ? &m_AppState->windows.jobManager : nullptr;
    }

    bool JobManager::IsWindowOpen() const
    {
        return m_AppState != nullptr && m_AppState->windows.jobManager;
    }

    void JobManager::ShowSettings()
    {
        if (m_AppState == nullptr || !m_AppState->windows.jobManager)
            return;
        ImGui::Begin("Job Manager", &m_AppState->windows.jobManager);

        ImGui::BeginTabBar("##JOB_MANAGER_TAB_BAR");

        if (ImGui::BeginTabItem("Running")) {
            int tmp = 0;
            for (int i = 0; i < m_AppState->jobSystem->threadPoolSize; i++) {
                if (m_AppState->jobSystem->threadPool[i].isRunningJob) {
                    tmp++;
                    ShowJob(m_AppState->jobSystem->threadPool[i].currentJob);
                }
            }
            if (tmp == 0) {
                ImGui::Text("No Running Jobs");
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Queued")) {
            ImGui::PushItemWidth(200);
            std::queue<uint32_t> jobs = m_AppState->jobSystem->asyncJobsQueue;
            ImGui::Text("Async Jobs");
            ImGui::Separator();
            if (jobs.empty())
                ImGui::Text("No Async Jobs");
            while (!jobs.empty()) {
                auto job = m_AppState->jobSystem->jobs[jobs.front()];
                jobs.pop();
                ShowJob(job.get());
            }
            jobs = m_AppState->jobSystem->asyncOnMTJobsQueue;
            ImGui::Separator();
            ImGui::Text("AsyncMT Jobs");
            ImGui::Separator();
            if (jobs.empty())
                ImGui::Text("No AsyncMT Jobs");
            while (!jobs.empty()) {
                auto job = m_AppState->jobSystem->jobs[jobs.front()];
                jobs.pop();
                ShowJob(job.get());
            }
            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Completed")) {
            ImGui::PushItemWidth(200);
            std::queue<uint32_t> completedJobs = m_AppState->jobSystem->completedJobs;
            if (completedJobs.empty())
                ImGui::Text("No Jobs");
            while (!completedJobs.empty()) {
                auto job = m_AppState->jobSystem->jobs[completedJobs.front()];
                completedJobs.pop();
                ShowJob(job.get());
            }
            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();

        ImGui::End();
    }

} // namespace tf3d::job
