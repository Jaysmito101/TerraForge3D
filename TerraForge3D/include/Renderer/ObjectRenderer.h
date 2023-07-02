#pragma once

#include "Renderer/RendererBase.h"

#define OBJECT_RENDERER_MAX_LIGHTS 16

class ObjectRenderer : public RendererBase 
{
public:
	ObjectRenderer(ApplicationState* appState);
	virtual ~ObjectRenderer();

	virtual void Render(RendererViewport* viewport) override;
	virtual void ShowSettings() override;

private:
	virtual void ReloadShaders() override;

private:
	bool m_InvertNormals = false;
	std::shared_ptr<ShaderStorageBuffer> m_SharedMemoryBuffer;
};