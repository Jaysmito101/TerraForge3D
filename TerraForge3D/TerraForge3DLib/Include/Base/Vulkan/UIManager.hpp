#pragma once

#include "Base/Vulkan/Context.hpp"
#include "Base/Renderer/UIManager.hpp"

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"

namespace TerraForge3D
{
	namespace Vulkan
	{

		class UIManager : public RendererAPI::UIManager
		{
		public:
			UIManager();
			~UIManager();

		private:
			void SetupWindow();
			void SetupImGui();
			void UploadFonts();

			void RenderFrame(ImDrawData* draw_data);
			void PresentFrame();
		public:

			virtual void Begin() override;
			virtual void End() override;

		private:
			Vulkan::Context* vulkanContext = nullptr;
			Vulkan::GraphicsDevice* graphicsDevice = nullptr;
			ImGui_ImplVulkanH_Window imguiWindow;
			VkPipelineCache pipelineCache = VK_NULL_HANDLE;
			bool requireSwapChainRebuild = false;
			GLFWwindow* windowHandle = nullptr;

			const ImVec4 clear_color = ImVec4(0.3f, 0.3f, 0.3f, 1.00f);
			const uint32_t minImageCount = 3;
		};

	}
}