#pragma once
#include "Base/Core/Core.hpp"


namespace TerraForge3D
{

	class Renderer
	{
	protected:
		Renderer();
		~Renderer();

	public:
		virtual void Shutdown() = 0;

		// ImGui Functions
		virtual void BeginImGui() = 0;
		virtual void EndImGui() = 0;

		static Renderer* Create();
		static Renderer* Get();
		static Renderer* Set(Renderer* context);
		static void Destroy();

	private:

		static Renderer* mainInstance;
	};
}
