#include "Data/ApplicationState.hpp"

namespace TerraForge3D
{

	ApplicationState* ApplicationState::appState = nullptr;


	ApplicationState* ApplicationState::Create()
	{
		TF3D_ASSERT(appState == nullptr, "Applicaiton State already created");
		appState = new ApplicationState();
		return appState;
	}

	void ApplicationState::Destory()
	{
		TF3D_ASSERT(appState, "Applicaiton Sate not yet created");
		TF3D_SAFE_DELETE(appState);
	}

	void ApplicationState::Set(ApplicationState* as)
	{
		appState = as;
	}
}