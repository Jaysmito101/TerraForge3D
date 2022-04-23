#pragma once
#include "Base/OpenGL/Core.hpp"
#include "Base/Renderer/UIManager.hpp"

namespace TerraForge3D
{
	class Window;

	namespace OpenGL
	{

		class UIManager : public RendererAPI::UIManager
		{
		public:
			UIManager();
			~UIManager();

			virtual void Begin();
			virtual void End();

		private:
			void SetupImGui();

		private:
			Window* window = nullptr;
		};

	}

}
