#pragma once

#include "Base/Base.h"
#include "Utils/Utils.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Exporters/Serializer.h"
#include "Renderer/ObjectRenderer.h"

class ApplicationState;

class MaskEditor
{
public:

	MaskEditor(ApplicationState* state, glm::vec3 vizColor);
	~MaskEditor();

	void Resize(int size);

	bool ShowSettings();

	bool ApplyDrawingShaders();

	inline void SetVizColor( float r, float g, float b ) { m_VizColor = glm::vec3(r, g, b); }

	inline GeneratorTexture* GetTexture() const { return m_Texture.get(); }

private:
	ApplicationState* m_AppState = nullptr;

	std::shared_ptr<ComputeShader> m_Shader;
	std::shared_ptr<GeneratorTexture> m_Texture;

	glm::vec3 m_VizColor = glm::vec3(0.2f, 0.2f, 0.2f);

	DrawBrushSettings m_DrawSettings;

	int m_Size = 256;
	bool m_RequireUpdation = true;
	bool m_IsEditing = false;

	static MaskEditor* s_CurrentlyEditingMaskEditor;
};