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

			inline bool PushConstantExists(const std::string& name) { return true; };
			

		public:
			LogicalDevice* device = nullptr;

			VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
			VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;
		};

	}

}
#endif