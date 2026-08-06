#include "Renderer/RendererLights.h"
#include "Data/ApplicationState.h"
#include "UI/ImGuiComponents.h"

namespace tf3d::renderer
{

    RendererLights::RendererLights(ApplicationState *appState)
    {
        m_Inspector.LoadConfig(appState, "Lights");
    }

    RendererLights::~RendererLights()
    {
    }

    void RendererLights::ShowSettings()
    {
        m_Inspector.Render();
    }

} // namespace tf3d::renderer
