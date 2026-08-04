#include "Generators/GeneratorDataStatistics.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Generators/GeneratorData.h"
#include "Base/ShaderStorageBuffer.h"

#include <algorithm>
#include <cstring>
#include <vector>

	constexpr int ResultWordCount = 2 + GeneratorDataStatistics::HistogramBinCount;

	float OrderedUintToFloat(uint32_t value)
	{
		const uint32_t bits = (value & 0x80000000u) != 0u
			? value ^ 0x80000000u
			: ~value;
		float result = 0.0f;
		std::memcpy(&result, &bits, sizeof(result));
		return result;
	}

GeneratorDataStatistics::GeneratorDataStatistics(ApplicationState* appState)
	: m_AppState(appState)
{
	m_Shader = m_AppState->resourceManager->LoadComputeShader("generation/utils/generator_data_statistics");
	m_ResultBuffer = std::make_shared<ShaderStorageBuffer>();
	std::vector<uint32_t> initial(ResultWordCount, 0u);
	initial[0] = 0xffffffffu;
	m_ResultBuffer->SetData(initial.data(), static_cast<unsigned int>(initial.size() * sizeof(uint32_t)));
}

void GeneratorDataStatistics::Compute(GeneratorData* data, int resolution, int sampleStride)
{
	if (data == nullptr || m_Shader == nullptr || m_ResultBuffer == nullptr || resolution <= 0) return;

	sampleStride = std::max(sampleStride, 1);
	std::vector<uint32_t> initial(ResultWordCount, 0u);
	initial[0] = 0xffffffffu;
	m_ResultBuffer->SetData(initial.data(), static_cast<unsigned int>(initial.size() * sizeof(uint32_t)));

	data->Bind(0);
	m_ResultBuffer->Bind(1);
	m_Shader->Bind();
	m_Shader->SetUniform1i("u_Resolution", resolution);
	m_Shader->SetUniform1i("u_SampleStride", sampleStride);

	const int workgroupSize = m_AppState->constants.gpuWorkgroupSize;
	const int sampleResolution = (resolution + sampleStride - 1) / sampleStride;
	const int dispatchSize = (sampleResolution + workgroupSize - 1) / workgroupSize;

	m_Shader->SetUniform1i("u_Mode", 0);
	m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
	m_Shader->SetMemoryBarrier();

	m_Shader->SetUniform1i("u_Mode", 1);
	m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
	m_Shader->SetMemoryBarrier();
}

GeneratorDataStatistics::Result GeneratorDataStatistics::Read() const
{
	Result statistics;
	if (m_ResultBuffer == nullptr) return statistics;

	std::array<uint32_t, ResultWordCount> result{};
	m_ResultBuffer->Bind();
	m_ResultBuffer->GetData(result.data(), static_cast<int>(result.size() * sizeof(uint32_t)));

	if (result[0] == 0xffffffffu || result[1] == 0u)
	{
		return statistics;
	}

	statistics.minimum = OrderedUintToFloat(result[0]);
	statistics.maximum = OrderedUintToFloat(result[1]);
	uint32_t maximumCount = 0u;
	for (int index = 0; index < HistogramBinCount; ++index)
		maximumCount = std::max(maximumCount, result[2 + index]);

	for (int index = 0; index < HistogramBinCount; ++index)
		statistics.histogram[index] = maximumCount == 0u
			? 0.0f
			: static_cast<float>(result[2 + index]) / static_cast<float>(maximumCount);
	statistics.valid = maximumCount != 0u;
	return statistics;
}
