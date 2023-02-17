#pragma once

#include <cstdint>

class ShaderStorageBuffer
{

public:
	ShaderStorageBuffer();
	~ShaderStorageBuffer();

	void Bind(int index);
	void Bind();
	void Unbind();
	void SetData(void *data, unsigned int size, bool dynamic = true);
	void SetData(void* data, unsigned int size, unsigned int offset, bool dynamic = true);
	void GetData(void *data, int size);
	void GetData(void* data, unsigned int size, unsigned int offset, bool dynamic = true);

	uint32_t rendererId;
};
