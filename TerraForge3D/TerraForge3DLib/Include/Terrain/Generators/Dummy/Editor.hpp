#pragma once
#include "Base/Base.hpp"
#include "UI/Editor.hpp"

namespace TerraForge3D
{
	class ApplicationState;

	namespace Terrain
	{
		namespace Dummy
		{
			struct SharedData;

			class Editor : public UI::Editor
			{
			public:
				Editor(ApplicationState* appState, SharedData* sharedData);
				~Editor() = default;
				void OnUpdate() override;
				void OnShow() override;
				void OnStart() override;
				void OnEnd() override;

			private:
				ApplicationState* appState = nullptr;
				SharedData* data;
			public:
			};
		}
	}
}
