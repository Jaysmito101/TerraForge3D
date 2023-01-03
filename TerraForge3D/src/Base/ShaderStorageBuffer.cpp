#include "ShaderStorageBuffer.h"

#include <glad/glad.h>

ShaderStorageBuffer::ShaderStorageBuffer()
{
	glGenBuffers(1, &rendererId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, rendererId);
}

ShaderStorageBuffer::~ShaderStorageBuffer()
{
	glDeleteBuffers(1, &rendererId);
}

void SetData(void* data, unsigned int size, unsigned int offset, bool dynamic = true)
{
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
}

void GetData(void* data, unsigned int size, unsigned int offset, bool dynamic = true)
{
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
}

void ShaderStorageBuffer::Bind(int index)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, rendererId);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, rendererId);
}

void ShaderStorageBuffer::Bind()
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, rendererId);
}

void ShaderStorageBuffer::Unbind()
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::SetData(void *data, unsigned int size, bool dynamic)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, rendererId);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, !dynamic ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
}

void ShaderStorageBuffer::GetData(void *data, int size)
{
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
}
