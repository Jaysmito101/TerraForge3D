#include "Job/Job.h"

static uint32_t jobIds = 0;

namespace JobSystem
{

	Job::Job(std::string name)
	{
		this->name = name;
		this->id = jobIds++;
	}

	Job::~Job()
	{
	}

}

