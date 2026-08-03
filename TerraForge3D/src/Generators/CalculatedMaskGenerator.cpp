#include "Generators/CalculatedMaskGenerator.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Utils/Utils.h"

CalculatedMaskGenerator::CalculatedMaskGenerator(ApplicationState* state)
	: m_AppState(state)
{
	m_Texture = std::make_shared<GeneratorTexture>(m_Size, m_Size, GeneratorTextureStorage::R16);
	m_Shader = m_AppState->resourceManager->LoadComputeShader("generation/utils/mask_preview");
}

CalculatedMaskGenerator::~CalculatedMaskGenerator()
{
}

void CalculatedMaskGenerator::Resize(int size)
{
	if (size <= 0) return;
	m_Size = size;
	m_Texture->Resize(size, size);
	m_Dirty = true;
}

void CalculatedMaskGenerator::Invalidate()
{
	m_Dirty = true;
}

bool CalculatedMaskGenerator::ShowSettings()
{
	bool changed = false;
	static const char* types[] = { "Height range", "Slope range" };
	int type = static_cast<int>(m_Settings.type);
	if (ShowComboBox("Source", &type, types, IM_ARRAYSIZE(types)))
	{
		m_Settings.type = static_cast<CalculatedMaskType>(type);
		changed = true;
	}
	if (ImGui::DragFloat2("Range", &m_Settings.minimum, 0.01f, -100.0f, 100.0f))
	{
		if (m_Settings.minimum > m_Settings.maximum) std::swap(m_Settings.minimum, m_Settings.maximum);
		changed = true;
	}
	if (ImGui::SliderFloat("Edge softness", &m_Settings.softness, 0.0f, 2.0f)) changed = true;
	if (changed) Invalidate();
	return changed;
}

bool CalculatedMaskGenerator::Update(GeneratorData* sourceData)
{
	if (sourceData == nullptr || !m_Dirty) return false;

	sourceData->Bind(0);
	m_Texture->BindForCompute(1);
	m_Shader->Bind();
	m_Shader->SetUniform1i("u_Resolution", m_Size);
	m_Shader->SetUniform1i("u_Mode", static_cast<int>(m_Settings.type));
	m_Shader->SetUniform4f(
		"u_Range",
		m_Settings.minimum,
		m_Settings.maximum,
		m_Settings.softness,
		m_AppState->mainMap.tileSize);
	const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
	const auto dispatchSize = (m_Size + workgroupSize - 1) / workgroupSize;
	m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
	m_Shader->SetMemoryBarrier();
	m_Dirty = false;
	return true;
}
