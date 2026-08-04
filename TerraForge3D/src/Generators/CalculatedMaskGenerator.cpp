#include "Generators/CalculatedMaskGenerator.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Utils/Utils.h"

namespace
{
	void SetMaskTypeDefaults(CalculatedMaskSettings& settings, CalculatedMaskType type)
	{
		settings.minimum = 0.25f;
		settings.maximum = 0.75f;
		settings.softness = 0.05f;
		settings.angle = 0.0f;
		settings.angleWidth = 45.0f;
		settings.sampleRadius = 3.0f;

		switch (type)
		{
		case CalculatedMaskType::SlopeRange:
			settings.minimum = 5.0f;
			settings.maximum = 35.0f;
			settings.softness = 3.0f;
			break;
		case CalculatedMaskType::Aspect:
			settings.softness = 10.0f;
			break;
		case CalculatedMaskType::Curvature:
		case CalculatedMaskType::Roughness:
		case CalculatedMaskType::RidgeValley:
		case CalculatedMaskType::FlowWetness:
		case CalculatedMaskType::AmbientOcclusion:
		case CalculatedMaskType::Exposure:
			settings.minimum = 0.35f;
			settings.maximum = 0.65f;
			settings.softness = 0.08f;
			break;
		case CalculatedMaskType::Flatness:
			settings.minimum = 0.65f;
			settings.maximum = 1.0f;
			settings.softness = 0.05f;
			break;
		case CalculatedMaskType::Coastline:
			settings.minimum = -0.05f;
			settings.maximum = 0.05f;
			settings.softness = 0.02f;
			break;
		case CalculatedMaskType::DistanceFromCoast:
			settings.minimum = 0.0f;
			settings.maximum = 0.25f;
			settings.softness = 0.03f;
			break;
		case CalculatedMaskType::DistanceFromBorder:
			settings.minimum = 0.0f;
			settings.maximum = 0.35f;
			settings.softness = 0.03f;
			break;
		case CalculatedMaskType::DistanceFromPointPath:
		case CalculatedMaskType::RadialGradient:
			settings.minimum = 0.0f;
			settings.maximum = 0.2f;
			settings.softness = 0.02f;
			break;
		case CalculatedMaskType::ProceduralNoise:
			settings.minimum = 0.35f;
			settings.maximum = 0.65f;
			settings.softness = 0.05f;
			break;
		case CalculatedMaskType::HeightContour:
			settings.minimum = 0.45f;
			settings.maximum = 0.55f;
			settings.softness = 0.02f;
			break;
		case CalculatedMaskType::HeightRange:
		case CalculatedMaskType::Count:
		default:
			break;
		}
	}
}

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
	static const char* types[] = {
		"Height range", "Slope range", "Aspect / direction", "Curvature", "Roughness / local variance",
		"Flatness", "Ridge / valley", "Coastline / sea-level band", "Distance from coast",
		"Flow accumulation / wetness", "Ambient occlusion / cavity", "Exposure", "Distance from border",
		"Distance from point / path", "Height contour band", "Procedural noise", "Radial / gradient"
	};
	static_assert(IM_ARRAYSIZE(types) == static_cast<int>(CalculatedMaskType::Count));
	int type = static_cast<int>(m_Settings.type);
	if (ShowComboBox("Source", &type, types, IM_ARRAYSIZE(types)))
	{
		m_Settings.type = static_cast<CalculatedMaskType>(type);
		SetMaskTypeDefaults(m_Settings, m_Settings.type);
		changed = true;
	}
	if (ImGui::Button("Reset recommended"))
	{
		SetMaskTypeDefaults(m_Settings, m_Settings.type);
		changed = true;
	}
	const auto maskType = m_Settings.type;
	const bool usesAngle = maskType == CalculatedMaskType::Aspect || maskType == CalculatedMaskType::Exposure;
	const bool usesRange = maskType != CalculatedMaskType::Aspect;
	const bool usesSlope = maskType == CalculatedMaskType::SlopeRange || maskType == CalculatedMaskType::Aspect ||
		maskType == CalculatedMaskType::Flatness || maskType == CalculatedMaskType::Exposure ||
		maskType == CalculatedMaskType::FlowWetness;
	const bool usesDistance = maskType == CalculatedMaskType::DistanceFromPointPath || maskType == CalculatedMaskType::RadialGradient;
	const bool usesNoise = maskType == CalculatedMaskType::ProceduralNoise;
	const bool usesSeaLevel = maskType == CalculatedMaskType::Coastline || maskType == CalculatedMaskType::DistanceFromCoast;
	const bool usesPointPath = maskType == CalculatedMaskType::DistanceFromPointPath;
	const bool usesRidgeValley = maskType == CalculatedMaskType::RidgeValley;

	if (usesAngle)
	{
		if (ImGui::DragFloat("Direction", &m_Settings.angle, 0.5f, -360.0f, 360.0f)) changed = true;
		if (ImGui::SliderFloat("Direction width", &m_Settings.angleWidth, 0.0f, 180.0f)) changed = true;
	}
	if (usesSlope)
	{
		if (ImGui::SliderFloat("Slope sample radius", &m_Settings.sampleRadius, 1.0f, 8.0f)) changed = true;
	}
	else if (usesPointPath || usesDistance)
	{
		if (ImGui::DragFloat2("Center", &m_Settings.center.x, 0.005f, -1.0f, 2.0f)) changed = true;
	}
	if (usesPointPath)
	{
		if (ImGui::DragFloat2("Path endpoint", &m_Settings.pathEnd.x, 0.005f, -1.0f, 2.0f)) changed = true;
		if (ImGui::Checkbox("Use path", &m_Settings.usePath)) changed = true;
	}
	if (usesNoise)
	{
		if (ImGui::DragFloat("Noise scale", &m_Settings.scale, 0.01f, 0.01f, 64.0f)) changed = true;
		if (ImGui::InputFloat("Noise seed", &m_Settings.seed)) changed = true;
	}
	if (usesSeaLevel)
	{
		if (ImGui::DragFloat("Sea level", &m_Settings.seaLevel, 0.01f, -100.0f, 100.0f)) changed = true;
	}
	if (usesRidgeValley)
	{
		static const char* ridgeValleyTypes[] = { "Ridge", "Valley" };
		int ridgeValley = m_Settings.selectValleys ? 1 : 0;
		if (ShowComboBox("Feature", &ridgeValley, ridgeValleyTypes, IM_ARRAYSIZE(ridgeValleyTypes)))
		{
			m_Settings.selectValleys = ridgeValley == 1;
			changed = true;
		}
	}

	const bool normalizedRange = maskType == CalculatedMaskType::Curvature || maskType == CalculatedMaskType::Roughness ||
		maskType == CalculatedMaskType::RidgeValley || maskType == CalculatedMaskType::FlowWetness ||
		maskType == CalculatedMaskType::AmbientOcclusion || maskType == CalculatedMaskType::Exposure ||
		maskType == CalculatedMaskType::Flatness || usesNoise;
	const char* rangeLabel = maskType == CalculatedMaskType::SlopeRange ? "Slope degrees" :
		usesDistance ? "Distance range" :
		usesNoise ? "Noise range" :
		normalizedRange ? "Normalized range" :
		"Range";
	const bool boundedRange = maskType == CalculatedMaskType::SlopeRange || normalizedRange || usesDistance ||
		maskType == CalculatedMaskType::DistanceFromBorder;
	if (usesRange)
	{
		const float rangeMinimum = boundedRange ? 0.0f : -100.0f;
		const float rangeMaximum = maskType == CalculatedMaskType::SlopeRange ? 90.0f : (boundedRange ? 1.0f : 100.0f);
		if (ImGui::DragFloat2(rangeLabel, &m_Settings.minimum, maskType == CalculatedMaskType::SlopeRange ? 0.25f : 0.01f,
			rangeMinimum, rangeMaximum))
		{
			if (m_Settings.minimum > m_Settings.maximum) std::swap(m_Settings.minimum, m_Settings.maximum);
			changed = true;
		}
	}
	const float softnessMax = usesAngle ? 90.0f : maskType == CalculatedMaskType::SlopeRange ? 15.0f : boundedRange ? 0.5f : 2.0f;
	if (ImGui::SliderFloat("Edge softness", &m_Settings.softness, 0.0f, softnessMax)) changed = true;
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
	m_Shader->SetUniform4f("u_Settings0", m_Settings.angle, m_Settings.angleWidth, m_Settings.scale, m_Settings.seed);
	m_Shader->SetUniform4f("u_Settings1", m_Settings.center.x, m_Settings.center.y, m_Settings.pathEnd.x, m_Settings.pathEnd.y);
	m_Shader->SetUniform4f("u_Settings2", m_Settings.seaLevel, m_Settings.selectValleys ? 1.0f : 0.0f, m_Settings.usePath ? 1.0f : 0.0f, m_Settings.sampleRadius);
	const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
	const auto dispatchSize = (m_Size + workgroupSize - 1) / workgroupSize;
	m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
	m_Shader->SetMemoryBarrier();
	m_Dirty = false;
	return true;
}
