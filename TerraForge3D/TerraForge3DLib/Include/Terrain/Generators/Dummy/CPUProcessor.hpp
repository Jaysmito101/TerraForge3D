#include "Terrain/Processor.hpp"

namespace TerraForge3D
{

	namespace Terrain
	{
		class Data;

		struct Dummy_SharedData;


		class Dummy_CPUProcessor : public Processor
		{
		public:
			Dummy_CPUProcessor(Dummy_SharedData* sharedData);
			~Dummy_CPUProcessor();

			virtual bool Process(Terrain::Data* data) override;

		private:
			Dummy_SharedData* data = nullptr;
		};



	}

}