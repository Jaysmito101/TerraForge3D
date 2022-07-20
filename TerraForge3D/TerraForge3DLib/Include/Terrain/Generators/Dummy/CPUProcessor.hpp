#include "Terrain/Processor.hpp"

namespace TerraForge3D
{

	namespace Terrain
	{
		class Data;
		namespace Dummy
		{
			struct SharedData;


			class CPUProcessor : public Terrain::Processor
			{
			public:
				CPUProcessor(SharedData* sharedData);
				~CPUProcessor();

				virtual bool Process(Terrain::Data* data) override;

			private:
				SharedData* data = nullptr;
			};

		}

	}

}