#pragma once
#include "Base/Base.hpp"
#include "UI/Editor.hpp"

namespace TerraForge3D
{
    class ApplicationState;
    
    class GeneratorSettings : public UI::Editor
    {
    public:
        GeneratorSettings(ApplicationState* appState);
        virtual ~GeneratorSettings();
    
        virtual void OnStart() override;
		virtual void OnShow() override;
		virtual void OnUpdate() override;
		virtual void OnEnd() override;
		virtual bool OnContextMenu() override;

    private:
        ApplicationState* appState = nullptr;
        

    };
}