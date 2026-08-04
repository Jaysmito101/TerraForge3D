#pragma once

#include "Generators/BiomeFilter.h"
#include "Generators/BiomeFilterCatalog.h"

#include <vector>

class ApplicationState;

class BiomeFilterStack
{
public:
	BiomeFilterStack(ApplicationState* appState);
	~BiomeFilterStack() = default;

	void Resize(size_t dataSize, int resolution);
	void Update(GeneratorData* baseResult);
	bool ShowSettings(int filterIndex);
	int AddFilter(const std::shared_ptr<BiomeFilterDefinition>& definition);
	void Load(SerializerNode data);
	SerializerNode Save() const;

	inline const std::vector<std::shared_ptr<BiomeFilter>>& GetFilters() const { return m_Filters; }
	inline const std::vector<std::shared_ptr<BiomeFilterDefinition>>& GetDefinitions() const { return m_Catalog->GetDefinitions(); }
	inline bool IsUpdationRequired() const { return m_RequireUpdation; }

private:
	void RunFilter(const std::shared_ptr<BiomeFilter>& filter, GeneratorData* input, GeneratorData* output);
	void RunPhase(const std::shared_ptr<BiomeFilter>& filter, const nlohmann::json& pass, GeneratorData* input, GeneratorData* output);
	void RunMergePhase(const std::shared_ptr<BiomeFilter>& filter, const nlohmann::json& merge, GeneratorData* input, GeneratorData* operation, GeneratorData* output);
	void SetPassUniforms(const std::shared_ptr<BiomeFilter>& filter, const std::shared_ptr<ComputeShader>& shader, const nlohmann::json& bindings);
	void EnsureTempBufferCount(size_t count);

	ApplicationState* m_AppState = nullptr;
	std::shared_ptr<BiomeFilterCatalog> m_Catalog;
	std::vector<std::shared_ptr<GeneratorData>> m_TempBuffers;
	std::shared_ptr<GeneratorData> m_ResultA;
	std::shared_ptr<GeneratorData> m_ResultB;
	std::vector<std::shared_ptr<BiomeFilter>> m_Filters;

	size_t m_DataSize = 0;
	int m_Resolution = 1;
	bool m_RequireUpdation = true;
};
