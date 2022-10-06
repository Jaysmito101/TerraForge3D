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

    }

    void GeneratorSettings::OnShow()
    {
		static char tempBuffer[32];


        Utils::ImGuiC::PushSubFont(appState->core.fonts["SemiHeader"].handle);
		ImGui::Text("General Settings");
		Utils::ImGuiC::PopSubFont();

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

    }

    void GeneratorSettings::OnEnd()
    {

    }

    bool GeneratorSettings::OnContextMenu()
    {
        return false;
    }
}