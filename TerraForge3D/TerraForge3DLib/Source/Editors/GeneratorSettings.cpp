#include "Editors/GeneratorSettings.hpp"

#include "TerraForge3D.hpp"

namespace TerraForge3D
{
    GeneratorSettings::GeneratorSettings(ApplicationState* as)
    :UI::Editor("Generator Settings")
    {
        this->appState = as;
    }

    GeneratorSettings::~GeneratorSettings()
    {

    }

    void GeneratorSettings::OnStart()
    {
        this->backgroundWorker = std::thread([this]()->void{this->BackgroundWorker();});
        this->backgroundWorker.detach();
    }

    void GeneratorSettings::OnShow()
    {
		static char tempBuffer[32];


        Utils::ImGuiC::PushSubFont(appState->core.fonts["SemiHeader"].handle);
		ImGui::Text("General Settings");
		Utils::ImGuiC::PopSubFont();

        static const char* deviceNames[] = {
            "CPU",
            "OpenCL"
#ifdef TF3D_VULKAN_BACKEND
            ,
            "Vulkan Compute"
#endif
        };

        if(ImGui::BeginCombo("Generator Device", deviceNames[appState->terrain.generatorState.device]))
        {
            for(int i = 0 ; i < TF3D_STATIC_ARRAY_SIZE(deviceNames); i++)
            {
                bool isSelected = (appState->terrain.generatorState.device == i);
                if(ImGui::Selectable(deviceNames[i], isSelected))
                    appState->terrain.generatorState.device = (TerrainGeneratorDevice)i;
                if(isSelected)
                    ImGui::SetItemDefaultFocus();
            }
			ImGui::EndCombo();
        }

        ImGui::DragFloat4("Scale", appState->terrain.globalScale, 0.01f);
        ImGui::DragFloat4("Offset", appState->terrain.globalOffset, 0.01f);

        sprintf(tempBuffer, "%d", appState->terrain.previewResolution);
		if(ImGui::BeginCombo("Preview Resolution", tempBuffer))
		{
			for (int n = 64; n <= 4096; n *= 2)
    		{
				sprintf(tempBuffer, "%d", n);
	        	bool isSelected = (appState->terrain.previewResolution == n);
    	    	if (ImGui::Selectable(tempBuffer, isSelected))
        		    appState->terrain.previewResolution = n;
    	    	if (isSelected)
	            	ImGui::SetItemDefaultFocus(); 
		    }
			ImGui::EndCombo();
		}
        Utils::ImGuiC::PushSubFont(appState->core.fonts["SemiHeader"].handle);
		ImGui::Text("Generators");
		Utils::ImGuiC::PopSubFont();
    }

    void GeneratorSettings::OnUpdate()
    {
        if(!this->isWorking)
        {
            OnJobDone();
            // assign new Job
            this->isWaiting = false;
            while(!this->isWorking) Utils::SleepFor(10);
        }
    }

    void GeneratorSettings::OnEnd()
    {
        this->isAlive = false;
        while(!this->hasThreadQuit) Utils::SleepFor(100);
        if(backgroundWorker.joinable())
            backgroundWorker.join();
        if(appState->terrain.generatorState.data)
            delete[] appState->terrain.generatorState.data;
    }

    bool GeneratorSettings::OnContextMenu()
    {
        return false;
    }

    void GeneratorSettings::OnJobDone()
    {
        appState->editors.inspector->UpdateTerrainData(&appState->terrain.generatorState);
    }

    void GeneratorSettings::GeneratePreview()
    {
        switch(appState->terrain.generatorState.device)
        {
            case TerrainGeneratorDevice::TerrainGeneratorDevice_CPU:
            {
                if(appState->terrain.generatorState.outputResolution[0] != appState->terrain.previewResolution)
                {
                    appState->terrain.generatorState.outputResolution[0] = appState->terrain.previewResolution;
                    if(appState->terrain.generatorState.data)
                        delete[] appState->terrain.generatorState.data;
                    appState->terrain.generatorState.data = new TerrainPointData[appState->terrain.previewResolution * appState->terrain.previewResolution];
                }
                memset(appState->terrain.generatorState.data, 0, appState->terrain.generatorState.outputResolution[0] * appState->terrain.generatorState.outputResolution[0] * sizeof(TerrainPointData));
                break;
            }
            case TerrainGeneratorDevice::TerrainGeneratorDevice_OpenCL:
            {
                break;
            }
#ifdef TF3D_VULKAN_BACKEND
            case TerrainGeneratorDevice::TerrainGeneratorDevice_VulkanCompute:
            {
                break;
            }
#endif
            default:
                TF3D_LOG_WARN("Invalid Generator Device Selected");
        }
    }

    void GeneratorSettings::BackgroundWorker()
    {
        TF3D_LOG("Started Backgournd Worker");
        this->hasThreadQuit = false;
        this->isAlive = true;
        while(this->isAlive)
        {
            while(this->isWaiting && this->isAlive) Utils::SleepFor(100);
            if(!this->isAlive) break;
            this->isWorking = true;
            this->GeneratePreview();
            this->isWaiting = true;
            this->isWorking = false;
        }
        this->hasThreadQuit = true;
        TF3D_LOG("Shutdown Backgournd Worker");
    }
}