#include "Shading/SharedMemoryManager.h"
#include "Base/Shader.h"

#include "glad/glad.h"

static int lastBinding = 1;

SharedMemoryManager::SharedMemoryManager()
{
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SharedMemoryItem), NULL, GL_DYNAMIC_DRAW);
	ssboBinding = lastBinding++;
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssboBinding, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

SharedMemoryManager::~SharedMemoryManager()
{
	glDeleteBuffers(1, &ssbo);
}

void SharedMemoryManager::Clear()
{
	currentID = 0;
	sharedMemoryBlobs.clear();
}

int SharedMemoryManager::AddItem()
{
	SharedMemoryItem blob;
	sharedMemoryBlobs.push_back(blob);
	return currentID++;
}

SharedMemoryItem *SharedMemoryManager::At(int id)
{
	return sharedMemoryBlobs.data() + id;
}

void SharedMemoryManager::UpdateShader(Shader *shader)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SharedMemoryItem) * sharedMemoryBlobs.size(), sharedMemoryBlobs.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssboBinding, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}