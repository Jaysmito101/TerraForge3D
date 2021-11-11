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

void ShaderStorageBuffer::Bind(int index)
{
	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, rendererId);
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

void ShaderStorageBuffer::SetData(void* data, unsigned int size, bool dynamic)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, rendererId);
	if (dynamic) {
		glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_COPY);
	}
	else {
		glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_STATIC_COPY);
	}
}

void ShaderStorageBuffer::GetData(void* data, int size)
{
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
}
