#include "Shading/SharedMemoryManager.h"
#include "Base/Shader.h"

#include "glad/glad.h"

static int lastBinding = 1;

void SetSharedMemoryItem(SharedMemoryItem* mi, int i, float val)
{
    switch (i)
    {
		case 0: mi->d0 		= val; break;
		case 1: mi->d1 		= val; break;
		case 2: mi->d2 		= val; break;
		case 3: mi->d3 		= val; break;
		case 4: mi->d4 		= val; break;
		case 5: mi->d5 		= val; break;
		case 6: mi->d6 		= val; break;
		case 7: mi->d7 		= val; break;
		case 8: mi->d8 		= val; break;
		case 9: mi->d9 		= val; break;
		case 10: mi->d10 	= val; break;
		case 11: mi->d11 	= val; break;
		case 12: mi->d12 	= val; break;
		case 13: mi->d13 	= val; break;
		case 14: mi->d14 	= val; break;
		case 15: mi->d15 	= val; break;
		case 16: mi->d16 	= val; break;
		case 17: mi->d17 	= val; break;
		case 18: mi->d18 	= val; break;
		case 19: mi->d19 	= val; break;
		case 20: mi->d20 	= val; break;
		case 21: mi->d21 	= val; break;
		case 22: mi->d22 	= val; break;
		case 23: mi->d23 	= val; break;
		case 24: mi->d24 	= val; break;
		case 25: mi->d25 	= val; break;
		case 26: mi->d26 	= val; break;
		case 27: mi->d27 	= val; break;
		case 28: mi->d28 	= val; break;
		case 29: mi->d29 	= val; break;
		case 30: mi->d30 	= val; break;
		case 31: mi->d31 	= val; break;
    }
}

float& SharedMemoryItem::operator[](int i)
{
	switch (i)
	{
		case 0: return d0;
		case 1: return d1;
		case 2: return d2;
		case 3: return d3;
		case 4: return d4;
		case 5: return d5;
		case 6: return d6;
		case 7: return d7;
		case 8: return d8;
		case 9: return d9;
		case 10: return d10;
		case 11: return d11;
		case 12: return d12;
		case 13: return d13;
		case 14: return d14;
		case 15: return d15;
		case 16: return d16;
		case 17: return d17;
		case 18: return d18;
		case 19: return d19;
		case 20: return d20;
		case 21: return d21;
		case 22: return d22;
		case 23: return d23;
		case 24: return d24;
		case 25: return d25;
		case 26: return d26;
		case 27: return d27;
		case 28: return d28;
		case 29: return d29;
		case 30: return d30;
		case 31: return d31;
	}
}

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

