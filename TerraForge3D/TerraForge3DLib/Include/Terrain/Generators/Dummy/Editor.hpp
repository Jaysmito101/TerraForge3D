#pragma once
#include "Base/Base.hpp"
#include "UI/Editor.hpp"

namespace TerraForge3D
{
	class ApplicationState;

	namespace Terrain
	{
		struct Dummy_SharedData;

		class Dummy_Editor : public UI::Editor
		{
		public:
			Dummy_Editor(ApplicationState* appState, Dummy_SharedData* sharedData);
			~Dummy_Editor() = default;
			void OnUpdate() override;
			void OnShow() override;
			void OnStart() override;
			void OnEnd() override;

		private:
			ApplicationState* appState = nullptr;
			Dummy_SharedData* data;
		public:
		};
	}
}
