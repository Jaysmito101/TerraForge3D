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

        bool *IsWindowOpenPtr();
        bool IsWindowOpen() const;

    private:
        ApplicationState *m_AppState = nullptr;
    };

} // namespace tf3d::job
namespace JobSystem = tf3d::job;
