#pragma once

#include <string>
#include <vector>
#include <cstdint>

class Shader;

/** SharedMemoryItem
 *!
 	A data structure to be passed to the GPU. This represents a single logical data blob(may be for a node or any other item).
*/
struct SharedMemoryItem
{
	float d0 = 0;
	float d1 = 0;
	float d2 = 0;
	float d3 = 0;
	float d4 = 0;
	float d5 = 0;
	float d6 = 0;
	float d7 = 0;
	float d8 = 0;
	float d9 = 0;
	float d10 = 0;
	float d11 = 0;
	float d12 = 0;
	float d13 = 0;
	float d14 = 0;
	float d15 = 0;
	float d16 = 0;
	float d17 = 0;
	float d18 = 0;
	float d19 = 0;
	float d20 = 0;
	float d21 = 0;
	float d22 = 0;
	float d23 = 0;
	float d24 = 0;
	float d25 = 0;
	float d26 = 0;
	float d27 = 0;
	float d28 = 0;
	float d29 = 0;
	float d30 = 0;
	float d31 = 0;
};

class SharedMemoryManager
{
public:
	SharedMemoryManager();
	~SharedMemoryManager();

	void Clear();

	int AddItem();

	void UpdateShader(Shader *shader);

	SharedMemoryItem *At(int id);

public:
	uint32_t ssbo;
	uint32_t ssboBinding;
	int currentID;
	std::vector<SharedMemoryItem> sharedMemoryBlobs;
};