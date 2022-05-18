#pragma once

#include "Base/Core/Core.hpp"
#include "Job/Job.hpp"
#include "Job/JobSystem.hpp"
#include "Job/Thread.hpp"
#include "UI/Editor.hpp"

namespace TerraForge3D
{

	class ApplicationState;

	class JobManager : public UI::Editor 
	{
	public:
		JobManager(ApplicationState* appState);
		~JobManager();

		virtual void OnUpdate() override;
		virtual void OnShow() override;
		virtual void OnStart() override;
		virtual void OnEnd() override;

	private:
		ApplicationState* appState = nullptr;
	};

}
