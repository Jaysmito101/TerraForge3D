#include "Base/Base.hpp"
#include "EntryPoint.hpp"

namespace TerraForge3D
{
	class MainApplication : public Application
	{
	public:
		virtual void OnStart() override
		{
			TF3D_LOG("Started Application!")
		}

		virtual void OnUpdate() override
		{
		}

		virtual void OnImGuiRender() override
		{
		}

		virtual void OnEnd() override
		{
			TF3D_LOG("Started Shutdown!")
		}

	private:
	};
}

TerraForge3D::Application* CreateApplication()
{
	return new TerraForge3D::MainApplication();
}