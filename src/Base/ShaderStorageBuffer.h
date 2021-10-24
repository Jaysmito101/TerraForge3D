#pragma once

#include <cstdint>

class ShaderStorageBuffer {

public:
	ShaderStorageBuffer();
	~ShaderStorageBuffer();

	void Bind(int index);
	void Unbind();
	void SetData(void* data, unsigned int size, bool dunamic = true);

	uint32_t rendererId;
};
