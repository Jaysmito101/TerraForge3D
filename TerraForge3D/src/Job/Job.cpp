#include "Job/Job.h"

static uint32_t jobIds = 0;

namespace tf3d::job
{

    Job::Job(std::string name)
    {
        this->name = name;
        this->id   = jobIds++;
    }

    Job::~Job()
    {
    }

} // namespace tf3d::job
