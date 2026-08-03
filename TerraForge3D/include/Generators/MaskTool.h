#pragma once

#include "Base/Base.h"
#include "Generators/GeneratorTexture.h"
#include "Renderer/BrushSettings.h"

#include <vector>

class ApplicationState;
class ComputeShader;

enum class MaskPreviewMode
{
	Painted,
	Generated,
};

class MaskTool
{
public:
	MaskTool(ApplicationState* state, glm::vec3 vizColor);
	~MaskTool();

	void Resize(int size);

	bool ShowSettings();
	bool ApplyDrawingShaders();

	void SetGeneratedMaskTexture(GeneratorTexture* texture, const char* label = "Filter mask");
	void ClearGeneratedMaskTexture();
	bool CopyGeneratedMaskToPainted();

	inline void SetVizColor(float r, float g, float b) { m_VizColor = glm::vec3(r, g, b); }
	inline GeneratorTexture* GetTexture() const { return m_PaintedTexture.get(); }
	inline GeneratorTexture* GetPreviewTexture() const;
	inline bool IsShowingGeneratedMask() const { return m_PreviewMode == MaskPreviewMode::Generated; }

private:
	struct MaskStroke
	{
		std::vector<glm::vec2> points;
		float strength = 0.0f;
		float size = 0.0f;
		float falloff = 0.0f;
		int mode = 0;
	};

	void SetPreviewMode(MaskPreviewMode mode);
	void UpdateViewportOverlay(bool showBrush);
	bool ShowPaintedSettings();
	bool UndoLastStroke();
	void RasterizeStrokes();
	void FinishActiveStroke();
	void StartActiveStroke(const glm::vec2& position);
	void AppendActiveStrokePoint(const glm::vec2& position);

	ApplicationState* m_AppState = nullptr;
	std::shared_ptr<ComputeShader> m_RasterizeShader;
	std::shared_ptr<ComputeShader> m_CopyShader;
	std::shared_ptr<ShaderStorageBuffer> m_StrokeSettingsBuffer;
	std::shared_ptr<ShaderStorageBuffer> m_StrokeRangesBuffer;
	std::shared_ptr<ShaderStorageBuffer> m_StrokePointsBuffer;
	std::shared_ptr<GeneratorTexture> m_BaseTexture;
	std::shared_ptr<GeneratorTexture> m_PaintedTexture;
	GeneratorTexture* m_ExternalGeneratedTexture = nullptr;
	std::vector<MaskStroke> m_Strokes;
	MaskStroke m_ActiveStroke;
	bool m_HasActiveStroke = false;

	glm::vec3 m_VizColor = glm::vec3(0.2f, 0.2f, 0.2f);
	DrawBrushSettings m_DrawSettings;
	MaskPreviewMode m_PreviewMode = MaskPreviewMode::Painted;
	std::string m_GeneratedMaskLabel = "Generated mask";

	int m_Size = 256;
	bool m_RequireUpdation = true;
	bool m_IsEditing = false;
	int m_PreviousBrushMode = 0;

	static MaskTool* s_CurrentlyEditingMaskTool;
};

inline GeneratorTexture* MaskTool::GetPreviewTexture() const
{
	if (m_PreviewMode == MaskPreviewMode::Generated)
	{
		return m_ExternalGeneratedTexture;
	}
	return m_PaintedTexture.get();
}
