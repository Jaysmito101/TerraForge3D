#pragma once
#include "Base/Core/Core.hpp"
#include "Base/Renderer/Renderer.hpp"

#include "Base/Vulkan/Vulkan.hpp"

namespace TerraForge3D
{


	class VulkanRenderer : public Renderer
	{
	public:
		VulkanRenderer();
		~VulkanRenderer();

		virtual void Shutdown() override;

		// ImGui Functions
		virtual void BeginImGui() override;
		virtual void EndImGui() override;

	private:
		Vulkan::Context* vulkanContext = nullptr;
		Vulkan::ImGuiManager* imGuiManager = nullptr;
	};

}
