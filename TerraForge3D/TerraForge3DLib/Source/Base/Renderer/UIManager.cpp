#pragma once
#include "Base/Renderer/UIManager.hpp"
#include "Base/Vulkan/UIManager.hpp"

namespace TerraForge3D
{

	namespace RendererAPI
	{
		UIManager* UIManager::mainInstance = nullptr;

		UIManager::UIManager()
		{
		}

		UIManager::~UIManager()
		{
		}

		UIManager* UIManager::Create()
		{
			TF3D_ASSERT(mainInstance == nullptr, "RendererAPI::UIManager already exists");
#if  defined(TF3D_OPENGL_BACKEND)
			mainInstance = new OpenGL::UIManager();
#elif defined(TF3D_VULKAN_BACKEND)
			mainInstance = new Vulkan::UIManager();
#endif 
			return mainInstance;
		}

		UIManager* UIManager::Get()
		{
			TF3D_ASSERT(mainInstance, "RendererAPI::UIManager not yet created");
			return mainInstance;
		}

		UIManager* UIManager::Set(UIManager* manager)
		{
			TF3D_ASSERT(manager, "manager is null");
			mainInstance = manager;
			return mainInstance;
		}

		void UIManager::Destory()
		{
			TF3D_ASSERT(mainInstance, "RendererAPI::UIManager not yet created");
			TF3D_SAFE_DELETE(mainInstance);
		}

	}

}
