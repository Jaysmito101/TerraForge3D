#pragma once

#include "Base/Base.h"
#include "Job/Job.h"
#include "Job/JobSystem.h"
#include "Job/Thread.h"


class ApplicationState;

namespace JobSystem
{
	class JobManager
	{
	public:
		JobManager(ApplicationState* appState);
		~JobManager();

		virtual void ShowSettings();

		inline bool* IsWindowOpenPtr() { return &m_IsVisible; }
		inline bool IsWindowOpen() const { return m_IsVisible; }

	private:
		ApplicationState* m_AppState = nullptr;
		bool m_IsVisible = false;
	};

}