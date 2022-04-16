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
			applicationName += " [OpenCL + VulkanCompute]";
			applicationName += " - Jaysmito Mukherjee";
			SetApplicationName(applicationName);
		}

		virtual void OnStart() override
		{
			TF3D_LOG("Started Application!");
			TF3D_LOG("Application UUID : {0}", GetUUID());

			// Test Vectors:

			Vector2 a2(58, 6);
			Vector2 b2 (4, 3.6f);

			TF3D_LOG_TRACE("Vector 2 Tests");
			TF3D_LOG_TRACE("Size {}", sizeof(Vector2));
			TF3D_LOG_TRACE("Initial Vector {} and {}", a2, b2);
			TF3D_LOG_TRACE("Vector Length {} and {}", a2.Length(), b2.Length());
			TF3D_LOG_TRACE("Vector Add {}", a2 + b2);
			TF3D_LOG_TRACE("Vector Dot {}", Vector2::Dot(a2, b2));
			TF3D_LOG_TRACE("Vector Cross {}", Vector2::Cross(a2, b2));
			TF3D_LOG_TRACE("Normalized Vector {}", a2.Normalize());
			TF3D_LOG_TRACE("Vector Length {}", a2.Length());
			TF3D_LOG_TRACE("Vector .YX {}", a2.YX());


			exitcb = GetInputEventManager()->RegisterCallback([&](InputEventParams* params) -> bool {
				TF3D_LOG_INFO("Shutting down applicatyin");
				Close();
				return true;
				}, { InputEventType_WindowClose });

			GetInputEventManager()->RegisterCallback([&](InputEventParams* params) -> bool {
				TF3D_LOG("Drag and Drop Recieved!");
				for (auto& item : params->paths)
				{
					TF3D_LOG("\t{}", item);
				}
				return true;
				}, { InputEventType_DragNDrop });

		}

		virtual void OnUpdate() override
		{
			if (InputSystem::IsKeyPressedS(KeyCode_Escape))
			{
				Close();
			}

			if (InputSystem::IsMouseButtonPressedS(MouseButton_Middle))
			{
				auto [posx, posy] = GetWindow()->GetPosition();
				auto [mdx, mdy] = InputSystem::GetMouseDeltaS();
				posx += TF3D_CLAMP(mdx, -10, 10);
				posy += TF3D_CLAMP(mdy, -10, 10);
				GetWindow()->SetPosition(posx, posy);
			}

			//TF3D_LOG("Mouse Pos {0} , {1}", InputSystem::GetMouseDeltaS().first, InputSystem::GetMouseDeltaS().second)
		}

		virtual void OnImGuiRender() override
		{
		}

		virtual void OnEnd() override
		{
			TF3D_LOG("Started Shutdown!");
			GetInputEventManager()->DeregisterCallback(exitcb);
		}

	private:
		uint32_t exitcb;
	};
}

TerraForge3D::Application* CreateApplication()
{
	return new TerraForge3D::MainApplication();
}