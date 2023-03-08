#include "Generators/GeneratatorData.h"
#include "Base/Base.h"

GeneratorData::GeneratorData()
{
	glGenBuffers(1, &m_RendererID);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_RendererID);
}

GeneratorData::~GeneratorData()
{
	glDeleteBuffers(1, &m_RendererID);
}

void GeneratorData::Bind(uint32_t slot)
{
	m_Binding = slot;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_RendererID);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, m_RendererID);
}

void GeneratorData::Resize(size_t size) 
{
	if(size == 0) return;
	m_Size = size;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_RendererID);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void GeneratorData::SetData(const void* data, size_t offset, size_t size)
{
	if (m_Size < size) Resize(size);
	if (m_Size == 0 || offset < 0 || offset > m_Size) return;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_RendererID);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

bool GeneratorData::CopyTo(const GeneratorData* other)
{
	if (m_Size != other->m_Size) return false;
	glBindBuffer(GL_COPY_READ_BUFFER, m_RendererID);
	glBindBuffer(GL_COPY_WRITE_BUFFER, other->m_RendererID);
	glBufferData(GL_COPY_WRITE_BUFFER, m_Size, nullptr, GL_DYNAMIC_DRAW);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, m_Size);
}

float* GeneratorData::GetCPUCopy()
{
	float* data = new float[m_Size / sizeof(float)];
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_RendererID);
	float* ptr = (float*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, m_Size, GL_MAP_READ_BIT);
	memcpy(data, ptr, m_Size);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	return data;
}

