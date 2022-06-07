#include "Job/JobManager.hpp"
#include "TerraForge3D.hpp"


namespace TerraForge3D
{
	static void ShowJob(JobSystem::Job* job)
	{
		ImGui::PushID((job->name + std::to_string(job->id)).data());
		if (ImGui::CollapsingHeader(job->name.data()))
		{
			ImGui::Text(job->description.data());
		}
		ImGui::PopID();
	}


	JobManager::JobManager(ApplicationState* appState)
		: UI::Editor("Job Manager")
	{
		this->appState = appState;
	}

	JobManager::~JobManager()
	{
	}

	void JobManager::OnUpdate()
	{
	}

	void JobManager::OnShow()
	{
		ImGui::BeginTabBar(uidStr.data());

		if (ImGui::BeginTabItem("Running"))
		{
			int tmp = 0;
			for (int i = 0; i < TF3D_THREAD_POOL_SIZE; i++)
			{
				if (appState->jobs.manager->threadPool[i].isRunningJob)
				{
					tmp++;
					ShowJob(appState->jobs.manager->threadPool[i].currentJob);
				}
			}
			if (tmp == 0)
			{
				if (tmp == 0)
					ImGui::Text("No Running Jobs");
			}
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Queued"))
		{
			ImGui::PushItemWidth(200);
			std::queue<uint32_t> jobs = appState->jobs.manager->asyncJobsQueue;
			ImGui::Text("Async Jobs");
			ImGui::Separator();
			if (jobs.empty())
				ImGui::Text("No Async Jobs");
			while (!jobs.empty())
			{
				JobSystem::Job* job = appState->jobs.manager->jobs[jobs.front()];
				jobs.pop();
				ShowJob(job);
			}
			jobs = appState->jobs.manager->asyncOnMTJobsQueue;
			ImGui::Separator();
			ImGui::Text("AsyncMT Jobs");
			ImGui::Separator();
			if (jobs.empty())
				ImGui::Text("No AsyncMT Jobs");
			while (!jobs.empty())
			{
				JobSystem::Job* job = appState->jobs.manager->jobs[jobs.front()];
				jobs.pop();
				ShowJob(job);
			}
			ImGui::PopItemWidth();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Completed"))
		{
			ImGui::PushItemWidth(200);
			std::queue<uint32_t> completedJobs = appState->jobs.manager->completedJobs;
			if (completedJobs.empty())
				ImGui::Text("No Jobs");
			while(!completedJobs.empty())
			{
				JobSystem::Job* job = appState->jobs.manager->jobs[completedJobs.front()];
				completedJobs.pop();
				ShowJob(job);
			}
			ImGui::PopItemWidth();
			ImGui::EndTabItem();
		}


		ImGui::EndTabBar();
	}

	void JobManager::OnStart()
	{
		appState->menus.mainMenu->GetManagerPTR()->Register("Windows/Job Manager", [this](UI::MenuItem* context) {
			isVisible = (context->GetToggleState());
			}, UI::MenuItemUse_Toggle)->RegisterTogglePTR(&isVisible);
		isVisible = false;
	}

	void JobManager::OnEnd()
	{
		appState->menus.mainMenu->GetManagerPTR()->Deregister("Windows/Job Manager");
	}

}