#include "Base/Base.hpp"
#include "EntryPoint.hpp"

namespace TerraForge3D
{
	class MainApplication : public Application
	{
	public:
		virtual void OnPreload()
		{
			SetLogFilePath("TerraForge3D.log");
			std::string applicationName = "TerraForge3D v" + TF3D_VERSION_STRING + " ";
#ifdef TF3D_WINDOWS
			applicationName += "[Windows]";
#elif defined TF3D_LINUX
			applicationName += "[Linux]";
#elif defined TF3D_MACOS
			applicationName += "[MacOS]";
#else
#error "Unknown Platform"
#endif
			applicationName += " ";
#ifdef TF3D_OPENCL
			applicationName += "[OpenCL]";
#elif defined TF3D_VULKAN_COMPUTE
			applicationName += "[VulkanCompute]";
#else
#error "Unknown Compute API"
#endif
			applicationName += " - Jaysmito Mukherjee";
			SetApplicationName(applicationName);
		}

		virtual void OnStart() override
		{
			TF3D_LOG("Started Application!");
			TF3D_LOG("Application UUID : {0}", GetUUID())

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