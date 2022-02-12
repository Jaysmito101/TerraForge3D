#pragma once

#include <cstdint>

class ShaderStorageBuffer {

public:
	ShaderStorageBuffer();
	~ShaderStorageBuffer();

	void Bind(int index);
	void Bind();
	void Unbind();
	void SetData(void* data, unsigned int size, bool dynamic = true);
	void GetData(void* data, int size);

	uint32_t rendererId;
};
