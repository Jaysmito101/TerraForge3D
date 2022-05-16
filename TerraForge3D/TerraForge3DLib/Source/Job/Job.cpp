#include "Job/Job.hpp"
#include "Job/JobSystem.hpp"


static uint32_t jobIds = 0;

namespace TerraForge3D
{

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

}