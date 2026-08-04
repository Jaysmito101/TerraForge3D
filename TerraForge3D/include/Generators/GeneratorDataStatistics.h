#pragma once

#include <array>
#include <cstdint>
#include <memory>

class ApplicationState;
class GeneratorData;
class ComputeShader;
class ShaderStorageBuffer;

struct GeneratorDataStatisticsResult
{
	static constexpr int HistogramBinCount = 256;

	std::array<float, HistogramBinCount> histogram{};
	float minimum = 0.0f;
	float maximum = 0.0f;
	bool valid = false;
};

class GeneratorDataStatistics
{
public:
	using Result = GeneratorDataStatisticsResult;
	static constexpr int HistogramBinCount = Result::HistogramBinCount;

	explicit GeneratorDataStatistics(ApplicationState* appState);
	~GeneratorDataStatistics() = default;

	void Compute(GeneratorData* data, int resolution, int sampleStride = 4);
	Result Read() const;

private:
	ApplicationState* m_AppState = nullptr;
	std::shared_ptr<ComputeShader> m_Shader;
	std::shared_ptr<ShaderStorageBuffer> m_ResultBuffer;
};
