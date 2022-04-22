#include "Base/Vulkan/ImGuiManager.hpp"
#include "Base/Core/Application.hpp"

#include "GLFW/glfw3.h"
#include "imgui/backends/imgui_impl_vulkan.cpp"


namespace TerraForge3D
{
	namespace Vulkan
	{
		ImGuiManager::ImGuiManager()
		{
			vulkanContext = Context::Get();
			graphicsDevice = GraphicsDevice::Get();
			windowHandle = Window::Get()->GetHandle();
			SetupWindow();
			SetupImGui();
			UploadFonts();
		}

		ImGuiManager::~ImGuiManager()
		{
			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();

			ImGui_ImplVulkanH_DestroyWindow(vulkanContext->instance, vulkanContext->graphicsDevice->handle, &imguiWindow, nullptr);
		}

		void ImGuiManager::SetupWindow()
		{
			imguiWindow.Surface = vulkanContext->swapChain->GetSurface();

			// Check WSI support
			if (!vulkanContext->graphicsDevice->physicalDevice.IsSurfaceSupported(imguiWindow.Surface, vulkanContext->graphicsDevice->presentQueueProperties.index))
			{
				TF3D_LOG_ERROR("Error no WSI Support on Physical Device");
				exit(-1);
			}

			// Select Surface Format
			const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
			const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
			imguiWindow.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(vulkanContext->graphicsDevice->physicalDevice.handle, imguiWindow.Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

			VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
			imguiWindow.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(vulkanContext->graphicsDevice->physicalDevice.handle, imguiWindow.Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
			TF3D_LOG("Selected Present Mode = {}", imguiWindow.PresentMode);

			// Create SwapChain, RenderPass, Framebuffer, etc.
			int w, h;
			glfwGetFramebufferSize(Window::Get()->GetHandle(), &w, &h);
			ImGui_ImplVulkanH_CreateOrResizeWindow(vulkanContext->instance, vulkanContext->graphicsDevice->physicalDevice.handle, vulkanContext->graphicsDevice->handle, &imguiWindow, vulkanContext->graphicsDevice->graphicsQueueProperties.index, nullptr, w, h, minImageCount);
		}

		void ImGuiManager::SetupImGui()
		{
			// Setup Dear ImGui context
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
			// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
			
			io.IniFilename = Application::Get()->windowConfigPath.c_str();
			

			// Setup Dear ImGui style
			ImGui::StyleColorsDark();

			// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
			ImGuiStyle& style = ImGui::GetStyle();
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				style.WindowRounding = 0.0f;
				style.Colors[ImGuiCol_WindowBg].w = 1.0f;
			}

			// Setup Platform/Renderer backends
			ImGui_ImplGlfw_InitForVulkan(Window::Get()->GetHandle(), true);
			ImGui_ImplVulkan_InitInfo init_info = {};
			init_info.Instance = vulkanContext->instance;
			init_info.PhysicalDevice = vulkanContext->graphicsDevice->physicalDevice.handle;
			init_info.Device = vulkanContext->graphicsDevice->handle;
			init_info.QueueFamily = vulkanContext->graphicsDevice->graphicsQueueProperties.index;
			init_info.Queue = vulkanContext->graphicsDevice->graphicsQueue;
			init_info.PipelineCache = pipelineCache;
			init_info.DescriptorPool = vulkanContext->graphicsDevice->descriptorPool;
			init_info.Subpass = 0;
			init_info.MinImageCount = minImageCount;
			init_info.ImageCount = imguiWindow.ImageCount;
			init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
			init_info.Allocator = nullptr;
			// init_info.CheckVkResultFn = check_vk_result;
			ImGui_ImplVulkan_Init(&init_info, imguiWindow.RenderPass);
		}

		void ImGuiManager::UploadFonts()
		{
			std::vector<ApplicationFont>& appFonts = Application::Get()->GetFonts();
			ImGuiIO& io = ImGui::GetIO();
			io.Fonts->AddFontDefault();
			for (auto& appFont : appFonts)
			{
				switch (appFont.type)
				{
				case FontType_TTF:
					appFont.handle = io.Fonts->AddFontFromFileTTF(appFont.path.data(), appFont.size);
					break;
				default:
					break;
				}
			}

			// Upload fonts to GPU
			// Use any command queue
			VkCommandPool command_pool = imguiWindow.Frames[imguiWindow.FrameIndex].CommandPool;
			VkCommandBuffer command_buffer = imguiWindow.Frames[imguiWindow.FrameIndex].CommandBuffer;

			VkResult err = vkResetCommandPool(vulkanContext->graphicsDevice->handle, command_pool, 0);
			check_vk_result(err);
			VkCommandBufferBeginInfo begin_info = {};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			err = vkBeginCommandBuffer(command_buffer, &begin_info);
			check_vk_result(err);

			ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

			VkSubmitInfo end_info = {};
			end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			end_info.commandBufferCount = 1;
			end_info.pCommandBuffers = &command_buffer;
			err = vkEndCommandBuffer(command_buffer);
			check_vk_result(err);
			err = vkQueueSubmit(vulkanContext->graphicsDevice->graphicsQueue, 1, &end_info, VK_NULL_HANDLE);
			check_vk_result(err);

			err = vkDeviceWaitIdle(vulkanContext->graphicsDevice->handle);
			check_vk_result(err);
			ImGui_ImplVulkan_DestroyFontUploadObjects();
			
		}

		void ImGuiManager::RenderFrame(ImDrawData* draw_data)
		{
			VkResult err;

			VkSemaphore image_acquired_semaphore = imguiWindow.FrameSemaphores[imguiWindow.SemaphoreIndex].ImageAcquiredSemaphore;
			VkSemaphore render_complete_semaphore = imguiWindow.FrameSemaphores[imguiWindow.SemaphoreIndex].RenderCompleteSemaphore;
			err = vkAcquireNextImageKHR(vulkanContext->graphicsDevice->handle, imguiWindow.Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &imguiWindow.FrameIndex);
			if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
			{
				requireSwapChainRebuild = true;
				return;
			}
			check_vk_result(err);

			ImGui_ImplVulkanH_Frame* fd = &imguiWindow.Frames[imguiWindow.FrameIndex];
			{
				err = vkWaitForFences(vulkanContext->graphicsDevice->handle, 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
				check_vk_result(err);

				err = vkResetFences(vulkanContext->graphicsDevice->handle, 1, &fd->Fence);
				check_vk_result(err);
			}
			{
				err = vkResetCommandPool(vulkanContext->graphicsDevice->handle, fd->CommandPool, 0);
				check_vk_result(err);
				VkCommandBufferBeginInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
				check_vk_result(err);
			}
			{
				VkRenderPassBeginInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				info.renderPass = imguiWindow.RenderPass;
				info.framebuffer = fd->Framebuffer;
				info.renderArea.extent.width = imguiWindow.Width;
				info.renderArea.extent.height = imguiWindow.Height;
				info.clearValueCount = 1;
				info.pClearValues = &imguiWindow.ClearValue;
				vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
			}

			// Record dear imgui primitives into command buffer
			ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

			// Submit command buffer
			vkCmdEndRenderPass(fd->CommandBuffer);
			{
				VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				VkSubmitInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				info.waitSemaphoreCount = 1;
				info.pWaitSemaphores = &image_acquired_semaphore;
				info.pWaitDstStageMask = &wait_stage;
				info.commandBufferCount = 1;
				info.pCommandBuffers = &fd->CommandBuffer;
				info.signalSemaphoreCount = 1;
				info.pSignalSemaphores = &render_complete_semaphore;

				err = vkEndCommandBuffer(fd->CommandBuffer);
				check_vk_result(err);
				err = vkQueueSubmit(vulkanContext->graphicsDevice->graphicsQueue, 1, &info, fd->Fence);
				check_vk_result(err);
			}
		}

		void ImGuiManager::PresentFrame()
		{
			if (requireSwapChainRebuild)
				return;
			VkSemaphore render_complete_semaphore = imguiWindow.FrameSemaphores[imguiWindow.SemaphoreIndex].RenderCompleteSemaphore;
			VkPresentInfoKHR info = {};
			info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			info.waitSemaphoreCount = 1;
			info.pWaitSemaphores = &render_complete_semaphore;
			info.swapchainCount = 1;
			info.pSwapchains = &imguiWindow.Swapchain;
			info.pImageIndices = &imguiWindow.FrameIndex;
			VkResult err = vkQueuePresentKHR(vulkanContext->graphicsDevice->graphicsQueue, &info);
			if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
			{
				requireSwapChainRebuild = true;
				return;
			}
			check_vk_result(err);
			imguiWindow.SemaphoreIndex = (imguiWindow.SemaphoreIndex + 1) % imguiWindow.ImageCount; // Now we can use the next set of semaphoress
		}

		void ImGuiManager::Begin()
		{
			if (requireSwapChainRebuild)
			{
				int width, height;
				glfwGetFramebufferSize(windowHandle, &width, &height);
				if (width > 0 && height > 0)
				{
					ImGui_ImplVulkan_SetMinImageCount(minImageCount);
					ImGui_ImplVulkanH_CreateOrResizeWindow(vulkanContext->instance, vulkanContext->graphicsDevice->physicalDevice.handle, vulkanContext->graphicsDevice->handle, &imguiWindow, vulkanContext->graphicsDevice->graphicsQueueProperties.index, nullptr, width, height, minImageCount);
					imguiWindow.FrameIndex = 0;
					requireSwapChainRebuild = false;
				}
			}

			// Start the Dear ImGui frame
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}

		void ImGuiManager::End()
		{
			// Rendering
			ImGui::Render();
			ImDrawData* main_draw_data = ImGui::GetDrawData();
			const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
			imguiWindow.ClearValue.color.float32[0] = clear_color.x * clear_color.w;
			imguiWindow.ClearValue.color.float32[1] = clear_color.y * clear_color.w;
			imguiWindow.ClearValue.color.float32[2] = clear_color.z * clear_color.w;
			imguiWindow.ClearValue.color.float32[3] = clear_color.w;
			if (!main_is_minimized)
				RenderFrame(main_draw_data);

			// Update and Render additional Platform Windows
			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}

			// Present Main Platform Window
			if (!main_is_minimized)
				PresentFrame();
		}
	}

}