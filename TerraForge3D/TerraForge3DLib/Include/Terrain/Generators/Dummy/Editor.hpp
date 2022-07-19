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
			class Editor : public UI::Editor
			{
			public:
				Editor(ApplicationState* appState);
				~Editor() = default;
				void OnUpdate() override;
				void OnShow() override;
				void OnStart() override;
				void OnEnd() override;

			private:
				ApplicationState* appState = nullptr;

			public:
			};
		}
	}
}
