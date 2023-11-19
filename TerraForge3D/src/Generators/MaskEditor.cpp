#include "Generators/MaskEditor.h"
#include "Data/ApplicationState.h"

MaskEditor::MaskEditor(ApplicationState* state)
{
	m_AppState = state;
	m_GeneratorTexture = std::make_shared<GeneratorTexture>(m_Size, m_Size);
}

MaskEditor::~MaskEditor()
{
}

void MaskEditor::Resize(int size)
{
	
}

bool MaskEditor::ShowSettings()
{
	ImGui::Separator();
	ImGui::Text("Mask Editor");

	ImGui::Image(m_GeneratorTexture->GetTextureID(), ImVec2(256, 256));

	ImGui::Separator();
	return false;
}
