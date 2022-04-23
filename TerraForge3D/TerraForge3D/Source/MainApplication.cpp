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
#elif defined TF3D_MACOSX
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
			// TF3D_LOG("Application UUID : {0}", GetUUID());

			// Test Vectors:



			exitcb = GetInputEventManager()->RegisterCallback([&](InputEventParams* params) -> bool {
				TF3D_LOG_INFO("Shutting down application.");
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
				pos[0] = posx;
				pos[1] = posy;
				GetWindow()->SetPosition(posx, posy);
			}

			//TF3D_LOG("Mouse Pos {0} , {1}", InputSystem::GetMouseDeltaS().first, InputSystem::GetMouseDeltaS().second)
		}

		virtual void OnImGuiRender() override
		{

			Renderer::Get()->uiManager->clearColor[0] = pos[0]/ 600;
			
			static bool isTestRunning = false;
			static int testWindowCount = 10;

			if (isTestRunning)
			{
				for (int i = 0; i < testWindowCount;i++)
				{
					ImGui::Begin((std::string("Window : ")+std::to_string(i)).c_str());
					ImGui::Text("Test Window");
					ImGui::Button("Button");
					ImGui::ArrowButton("Arrow Button", ImGuiDir_Left);
					ImGui::End();
				}
			}


			ImGui::Begin("Hello World!");

			ImGui::Text("Welcome to TerraForge3D!");
			ImGui::Text("Window Position: %f, %f", pos[0], pos[1]);
			ImGui::Text("Draw Window with middle mouse button to move it!");
			
			ImGui::NewLine();
			
			ImGui::Text("Delta Time: %f", deltaTime);
			ImGui::Text("Frame Rate: %d", static_cast<uint32_t>(1000.0 / deltaTime));
			
			ImGui::NewLine();
			
			ImGui::Checkbox("Stress Test", &isTestRunning);
			ImGui::InputInt("Stress Test Window Count", &testWindowCount, 5);
			
			if (ImGui::Button("Exit"))
				Close();

			ImGui::End();




		}

		virtual void OnEnd() override
		{
			TF3D_LOG("Started Shutdown!");
			GetInputEventManager()->DeregisterCallback(exitcb);
		}

	private:
		uint32_t exitcb;
		float pos[2];
	};
}

TerraForge3D::Application* CreateApplication()
{
	return new TerraForge3D::MainApplication();
}