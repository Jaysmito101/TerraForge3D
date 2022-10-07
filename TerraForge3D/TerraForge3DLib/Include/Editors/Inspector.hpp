#pragma once
#include "Base/Base.hpp"
#include "UI/Editor.hpp"
#include "Editors/GeneratorSettings.hpp"

namespace TerraForge3D
{

	class ApplicationState;
	class Viewport;

	class Inspector : public UI::Editor
	{
	public:
		Inspector(ApplicationState* appState);
		virtual ~Inspector();

		virtual void OnStart() override;
		virtual void OnShow() override;
		virtual void OnUpdate() override;
		virtual void OnEnd() override;
		virtual bool OnContextMenu() override;

		bool RenderItems(Viewport* viewport);

		void UpdateTerrainData(TerrainGeneratorState* state);

	private:
		void ShowTerrainSettings();


	private:
		ApplicationState* appState = nullptr;
		std::atomic<bool> isRemeshing = false;

		float itemViewHeight = 100;
		float separatorSliderWidth = 40.0f;

		float prevMousePos = 0.0f;

	public:
		bool isTerrainSelected = false;
		SharedPtr<GeneratorSettings> generatorSettings;
		struct
		{
			SharedPtr<Mesh> mesh;
			SharedPtr<RendererAPI::SharedStorageBuffer> dataBuffer;
			float resolutionData[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		} terrain;

	};



}


