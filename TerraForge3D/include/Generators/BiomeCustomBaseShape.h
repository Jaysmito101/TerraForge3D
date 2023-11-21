#pragma once

#include "Base/Base.h"
#include "Utils/Utils.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Exporters/Serializer.h"
#include "Misc/CustomInspector.h"
#include "Renderer/ObjectRenderer.h"

class ApplicationState;

enum BiomeCustomBaseShapeEditMode
{
	BiomeCustomBaseShapeEditMode_Draw,
	BiomeCustomBaseShapeEditMode_Count
};

class BiomeCustomBaseShape
{
public:
	BiomeCustomBaseShape(ApplicationState* appState);
	~BiomeCustomBaseShape();

	bool ShowShettings();
	void Update(GeneratorData* sourceBuffer, GeneratorData* targetBuffer, GeneratorData* swapBuffer);

	SerializerNode Save();
	void Load(SerializerNode node);

	inline bool RequireUpdation() const { return m_RequireUpdation; }
	inline bool IsEnabled() const { return m_Enabled; }
	inline bool RequiresBaseShapeUpdate() const { return m_RequireBaseShapeUpdate; }

	void Resize();

private:
	bool ApplyDrawingShaders();
	bool ShowDrawEditor();

private:
	ApplicationState* m_AppState = nullptr;
	bool m_RequireUpdation = true;
	bool m_Enabled = false;
	bool m_RequireBaseShapeUpdate = false;
	std::shared_ptr<ComputeShader> m_Shader;
	std::shared_ptr<GeneratorData> m_WorkingDataBuffer, m_SwapBuffer;
	std::shared_ptr<GeneratorTexture> m_PreviewTexture;
	BiomeCustomBaseShapeEditMode m_EditMode = BiomeCustomBaseShapeEditMode_Draw;
	float m_CalculationTime = 0.0f;
	DrawBrushSettings m_DrawSettings;
};