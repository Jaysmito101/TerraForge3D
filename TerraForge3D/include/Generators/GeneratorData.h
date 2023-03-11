#pragma once

#include <memory>
#include <cstdint>

class GeneratorData
{
public:
	GeneratorData();
	~GeneratorData();
	void Bind(uint32_t slot = 0);
	void Resize(size_t size);
	void SetData(const void* data, size_t offset = 0, size_t size = 0);
	bool CopyTo(const GeneratorData* other);
	float* GetCPUCopy();

private:
	uint32_t m_RendererID = 0;
	uint32_t m_Binding = 0;
	size_t m_Size = 0;
};