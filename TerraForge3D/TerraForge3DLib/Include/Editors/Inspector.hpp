#pragma once
#include "Base/Base.hpp"
#include "UI/Editor.hpp"

namespace TerraForge3D
{

	class ApplicationState;

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

	private:
		ApplicationState* appState = nullptr;
		uint32_t itemViewHeight = 100;
		float separatorSliderWidth = 20.0f;

		float prevMousePos = 0.0f;

	public:
		bool isTerrainSelected = false;
	};

}