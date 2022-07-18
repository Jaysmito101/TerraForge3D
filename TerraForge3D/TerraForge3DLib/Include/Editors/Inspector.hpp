#pragma once
#include "Base/Base.hpp"
#include "UI/Editor.hpp"

namespace TerraForge3D
{

	class ApplicationState;
	class Viewport;

	namespace Terrain
	{
		class Manager;
	}

	class Inspector : public UI::Editor
	{
	public:
		Inspector(ApplicationState* appState);
		~Inspector();

		virtual void OnStart() override;
		virtual void OnShow() override;
		virtual void OnUpdate() override;
		virtual void OnEnd() override;
		virtual bool OnContextMenu() override;

		bool RenderItems(Viewport* viewport);

	private:
		void ShowTerrainSettings();


	private:
		ApplicationState* appState = nullptr;

		float itemViewHeight = 100;
		float separatorSliderWidth = 20.0f;

		float prevMousePos = 0.0f;

	public:
		bool isTerrainSelected = false;

		struct
		{
			uint32_t resolutionIndex = 0;
			uint32_t resolution = 128;
			float scale = 1.0f;
			SharedPtr<Terrain::Manager> manager;
		} terrain;

		struct
		{

		} generators;
	};



}


