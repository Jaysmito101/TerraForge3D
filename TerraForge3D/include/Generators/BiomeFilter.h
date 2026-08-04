#pragma once

#include "Base/Base.h"
#include "Generators/CalculatedMaskGenerator.h"
#include "Generators/GeneratorData.h"
#include "Generators/MaskTool.h"
#include "Generators/BiomeFilterDefinition.h"
#include "Exporters/Serializer.h"

class ApplicationState;

enum class BiomeFilterMergeMode
{
	Override,
	Add,
	Subtract,
	Multiply,
	Blend,
};

class BiomeFilter
{
public:
	BiomeFilter(ApplicationState* appState, std::shared_ptr<BiomeFilterDefinition> definition);
	~BiomeFilter() = default;

	bool ShowSettings();
	void Resize(int size);
	void UpdateGeneratedMask(GeneratorData* sourceData);
	void Load(SerializerNode data);
	SerializerNode Save() const;

	inline const std::string& GetID() const { return m_ID; }
	inline const std::string& GetName() const { return m_Definition->GetName(); }
	inline const std::string& GetCategory() const { return m_Definition->GetCategory(); }
	inline const std::string& GetDescription() const { return m_Definition->GetDescription(); }
	inline const std::string& GetDefinitionID() const { return m_Definition->GetID(); }
	inline const std::string& GetImplementation() const { return m_Definition->GetImplementation(); }
	inline std::shared_ptr<BiomeFilterDefinition> GetDefinition() const { return m_Definition; }
	inline const std::unordered_map<std::string, CustomInspectorValue>& GetParameters() const { return m_Inspector->GetValues(); }
	inline bool IsEnabled() const { return m_Enabled; }
	inline bool UsesMask() const { return m_UseMask; }
	inline bool InvertsMask() const { return m_InvertMask; }
	int GetIntegerParameter(const std::string& name, int defaultValue = 0) const;
	inline float GetStrength() const { return m_Strength; }
	inline BiomeFilterMergeMode GetMergeMode() const { return m_MergeMode; }
	inline GeneratorTexture* GetMaskTexture() const { return m_MaskTool->GetPreviewTexture(); }
	inline std::shared_ptr<ComputeShader> GetPhaseShader(ApplicationState* appState, const std::string& phase) const { return m_Definition->GetPhaseShader(appState, phase); }

private:
	ApplicationState* m_AppState = nullptr;
	std::shared_ptr<BiomeFilterDefinition> m_Definition;
	std::shared_ptr<CustomInspector> m_Inspector;
	std::string m_ID;

	bool m_Enabled = true;
	bool m_UseMask = false;
	bool m_InvertMask = false;
	float m_Strength = 1.0f;
	BiomeFilterMergeMode m_MergeMode = BiomeFilterMergeMode::Blend;

	std::shared_ptr<CalculatedMaskGenerator> m_CalculatedMaskGenerator;
	std::shared_ptr<MaskTool> m_MaskTool;
};
