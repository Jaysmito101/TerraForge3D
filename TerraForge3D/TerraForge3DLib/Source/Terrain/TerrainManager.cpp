#include "Terrain/Manager.hpp"
#include "TerraForge3D.hpp"

namespace TerraForge3D
{

	namespace Terrain
	{

		Manager::Manager(ApplicationState* as)
		{
			this->appState = as;
		}

		Manager::~Manager()
		{
		}

		void Manager::OnStart()
		{
			this->mesh = new Mesh("MainTerrain");
			this->meshClone = new Mesh("MainTerrain [CLONE]");
			this->mesh->Clear();
			this->Resize(TF3D_DEFAULT_TERRAIN_RESOLUTION, TF3D_DEFAULT_TERRAIN_SCALE);
			this->mesh->UploadToGPU();
			workerThreadRunning = true;
			workerThread = std::thread(std::bind(&Manager::WorkerThread, this));
			workerThread.detach();
			terrainData = new Terrain::Data(appState);
			terrainData->LoadData();
		}

		void Manager::OnEnd()
		{
			while (workerThreadBusy)
			{
				TF3D_LOG("Waiting for worker thread to finish pending jobs.");
				Utils::SleepFor(100);
			}
			workerThreadRunning = false;
			workerThreadReady = true;
			
			if (workerThread.joinable())
				workerThread.join();

			for (auto& gen : generators)
			{
				gen->OnDetach();
			}
		}

		void Manager::Update()
		{
			if (!workerThreadBusy)
			{
				OnJobDone();
				mesh->UploadToGPU();
				workerThreadReady = true;
			}
		}

		void Manager::AddGenerator(SharedPtr<Generator> generator)
		{
			this->generatorsTemp.push_back(generator);
			this->generatorsTemp.back()->OnAttach();
		}

		void Manager::ShowGeneratorSettings()
		{
			static char buffer[128] = {0};
			int i = 0;
			ImVec4 currentChildBgColor = ImGui::GetStyle().Colors[ImGuiCol_ChildBg];
			currentChildBgColor.x = 0.1f;
			currentChildBgColor.y = 0.1f;
			currentChildBgColor.z = 0.1f;

			

			ImGui::PushStyleColor(ImGuiCol_ChildBg, currentChildBgColor);
			for (auto& generator : generators)
			{
				sprintf(buffer, "##GeneratorSettingsHeader%d", i++);
				ImGui::BeginChild(buffer, ImVec2(ImGui::GetWindowWidth(), 100));
				Utils::ImGuiC::PushSubFont(appState->core.fonts["SemiHeader"].handle);
				ImGui::Text(generator->name.data());
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Double click to edit name");
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					appState->modals.manager->InputBox("Enter Name", [&](std::string input, std::string& result)->bool { generator->name = input; return true; });
				Utils::ImGuiC::PopSubFont();
				if (ImGui::Button("Edit"))
				{
					generator->editor->SetVisible(true);
				}
				ImGui::EndChild();
				ImGui::Separator();
			}
			ImGui::PopStyleColor();
		}

		void Manager::Resize(uint32_t res, float sc, float* progress)
		{
			this->isBeingResized = true;
			while (workerThreadBusy)
			{
				TF3D_LOG("Waiting for worker thread to finish pending jobs.");
				Utils::SleepFor(100);
			}
			//if (progress)
			//	*progress = 0.0f;
			this->resolution = res;
			this->scale = sc;
			this->mesh->Plane(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), resolution, scale, progress);
			mesh->RecalculateNormals();
			this->mesh->Clone(this->meshClone.Get());
			this->isBeingResized = false;
		}


		void Manager::WorkerThread()
		{
			TF3D_LOG("Starting up worker thread");
			while (workerThreadRunning)
			{
				workerThreadReady = false;
				workerThreadBusy = false;
				
				while (!workerThreadReady)	Utils::SleepFor(10);
				while (isBeingResized)	Utils::SleepFor(10);

				if (!workerThreadRunning)
					break;
				
				workerThreadBusy = true;
				
				switch (processorDevice)
				{
				case ProcessorDevice_CPU:
					ProcessGeneratorsOnCPU();
					break;
#ifdef TF3D_VULKAN_BACKEND
				case ProcessorDevice_Vulkan:
					ProcessGeneratorsOnVulkan();
					break;
#endif
				case ProcessorDevice_OpenCL:
					ProcessGeneratorsOnOpenCL();
					break;
				default:
					TF3D_ASSERT(false, "Invalid Processor Device");
					break;
				}
			
			}
			TF3D_LOG("Shutting down worker thread");
		}

		void Manager::OnJobDone()
		{
			for (uint32_t i = 0; i < generatorsTemp.size(); i++)
			{
				generators.push_back(generatorsTemp.back());
				generatorsTemp.pop_back();
			}
		}

		void Manager::ProcessGeneratorsOnCPU()
		{
			terrainData->PrepareForGenerators();
			for (int i = 0; i <generators.size();i++)
				generators[i]->cpuProcessor->Process(terrainData.Get());
		}

		void Manager::ProcessGeneratorsOnVulkan()
		{
#ifdef TF3D_VULKAN_BACKEND

#endif
		}

		void Manager::ProcessGeneratorsOnOpenCL()
		{
		}
	}
}