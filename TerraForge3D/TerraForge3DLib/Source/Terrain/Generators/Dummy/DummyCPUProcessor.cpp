#include "Terrain/Generators/Dummy/Generator.hpp"
#include "TerraForge3D.hpp"

namespace TerraForge3D
{

	namespace Terrain
	{
		Dummy_CPUProcessor::Dummy_CPUProcessor(Dummy_SharedData* sd)
			:data(sd), Terrain::Processor("Dummy CPU Processor", ProcessorDevice_CPU)
		{}

		Dummy_CPUProcessor::~Dummy_CPUProcessor()
		{}

		bool Dummy_CPUProcessor::Process(Terrain::Data* data)
		{
			uint64_t size = data->mesh->vertices.size();
			auto& clonevertices = data->meshClone->vertices;
			for (uint64_t i = 0; i < size; i++)
			{
				data->mesh->vertices[i].position += sin(data->mesh->vertices[i].position.x * 4.0f) * 4.0f * clonevertices[i].normal;
			}
			return true;
		}


	}

}