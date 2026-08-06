#pragma once

#include "Base/Base.h"
#include "Job/Job.h"
#include "Job/JobSystem.h"
#include "Job/Thread.h"

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::job
{
    class JobManager
    {
    public:
        JobManager(ApplicationState *appState);
        virtual ~JobManager();

        virtual void ShowSettings();

        inline bool *IsWindowOpenPtr()
        {
            return &m_IsVisible;
        }
        inline bool IsWindowOpen() const
        {
            return m_IsVisible;
        }

    private:
        ApplicationState *m_AppState = nullptr;
        bool m_IsVisible             = false;
    };

} // namespace tf3d::job
namespace JobSystem = tf3d::job;
