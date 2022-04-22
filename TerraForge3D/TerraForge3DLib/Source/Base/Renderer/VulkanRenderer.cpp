#include "Base/Renderer/VulkanRenderer.hpp"

namespace TerraForge3D
{

	VulkanRenderer::VulkanRenderer()
	{
		vulkanContext = Vulkan::Context::Create();
		imGuiManager = new Vulkan::ImGuiManager();
	}

	VulkanRenderer::~VulkanRenderer()
	{
		TF3D_SAFE_DELETE(imGuiManager);
		Vulkan::Context::Destroy();
	}

	void VulkanRenderer::Shutdown()
	{
		TF3D_VK_CALL(vkDeviceWaitIdle(vulkanContext->graphicsDevice->handle));
	}

	void VulkanRenderer::BeginImGui()
	{
		imGuiManager->Begin();
	}

	void VulkanRenderer::EndImGui()
	{
		imGuiManager->End();
	}

}
