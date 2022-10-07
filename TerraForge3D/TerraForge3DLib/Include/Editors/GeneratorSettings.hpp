#pragma once
#include "Base/Base.hpp"
#include "UI/Editor.hpp"
#include "Terrain/Generator.hpp"

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
        void BackgroundWorker();
        void OnJobDone();
        void GeneratePreview();

    private:
        std::atomic<bool> isWorking = false;
        std::atomic<bool> isWaiting = false;
        std::atomic<bool> isAlive = false;
        std::atomic<bool> hasThreadQuit = false;
        std::thread backgroundWorker;

    private:
        ApplicationState* appState = nullptr;      

    };
}