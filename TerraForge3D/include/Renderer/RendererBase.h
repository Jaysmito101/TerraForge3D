#pragma once

#include "Base/Base.h"
#include "Renderer/RendererViewport.h"


class ApplicationState;

class RendererBase
{
public:
	RendererBase() = default;
	virtual ~RendererBase() = default;

	virtual void Render(RendererViewport* viewport) = 0;
	virtual void ShowSettings() = 0;

protected:
	virtual void ReloadShaders() = 0;

protected:
	ApplicationState* m_AppState = nullptr;
	std::shared_ptr<Shader> m_Shader = nullptr;
};