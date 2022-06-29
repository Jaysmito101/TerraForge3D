#pragma once
#include "Base/Vulkan/Core.hpp"
#include "Base/Renderer/Shader.hpp"
#include "Base/Vulkan/LogicalDevice.hpp"

#ifdef TF3D_VULKAN_BACKEND
namespace TerraForge3D
{

	namespace Vulkan
	{
	

		class Shader : public RendererAPI::Shader
		{
		public:
			Shader();
			~Shader();

			virtual void Cleanup() override;
			virtual bool Compile() override;
			virtual bool LoadFromBinary() override;

			void LoadPushConstantLocations();

			inline bool PushConstantExists(const std::string& name) { return pushConstantsLocations.find(name) != pushConstantsLocations.end(); };
			inline std::pair<uint32_t, uint32_t>& GetPushConstantLocation(const std::string& name) { return pushConstantsLocations[name]; }

		public:
			LogicalDevice* device = nullptr;

			VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
			VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;

			std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> pushConstantsLocations;
			uint32_t pushConstantsSize = 0;
		};

	}

}
#endif