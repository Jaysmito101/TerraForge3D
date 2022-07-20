#include "Terrain/Generators/Dummy/Generator.hpp"
#include "TerraForge3D.hpp"

namespace TerraForge3D
{

	namespace Terrain
	{
		namespace Dummy
		{

			CPUProcessor::CPUProcessor(SharedData* sd)
				:data(sd), Terrain::Processor("Dummy CPU Processor", ProcessorDevice_CPU)
			{}

			CPUProcessor::~CPUProcessor()
			{}

			bool CPUProcessor::Process(Terrain::Data* data)
			{
				uint32_t size = data->mesh->vertices.size();
				auto& clonevertices = data->meshClone->vertices;
				for (uint32_t i = 0 ; i < size ; i++)
				{
					data->mesh->vertices[i].position = sin(data->mesh->vertices[i].position.x * 4.0f) * 4.0f * clonevertices[i].normal;
				}
				return true;
			}

		}
	}

}