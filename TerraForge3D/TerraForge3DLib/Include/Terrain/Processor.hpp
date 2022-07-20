#pragma once

#include "Base/Base.hpp"
#include "Terrain/Data.hpp"

namespace TerraForge3D
{

	namespace Terrain
	{

		enum ProcessorDevice
		{
			ProcessorDevice_CPU = 0,
#ifdef TF3D_VULKAN_BACKEND
			ProcessorDevice_Vulkan,
#endif
			ProcessorDevice_OpenCL
		};

		static const char* ProcessorDeviceStr[] = {
			"CPU",
#ifdef TF3D_VULKAN_BACKEND
			"Vulkan",
#endif
			"OpenCL"
		};

		class Processor
		{
		public:
			Processor(std::string name, ProcessorDevice device)
			: name(name), device(device) {}
			virtual ~Processor() = default;

			virtual bool Process(Terrain::Data* data) = 0;

		protected:
			ProcessorDevice device = ProcessorDevice_CPU;
		public:
			std::string name = "Processor";
		};
	}

}
