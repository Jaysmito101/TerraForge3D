#pragma once

#include <algorithm>
#include <string>
#include <glad/glad.h>

#ifdef min 
#undef min
#endif

#ifdef max
#undef max
#endif

class DataTexture
{
public:
	DataTexture(int binding)
	{
		glGenBuffers(1, &m_RendererID);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_RendererID);
		m_Binding = binding;
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_Binding, m_RendererID);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	virtual ~DataTexture()
	{
		glDeleteBuffers(1, &m_RendererID);
		if (pData) delete[] pData;
	}

	inline void Resize(int resolution)
	{
		this->size = resolution;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_RendererID);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * size * size * 4, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	inline void PullBackToCPU()
	{
		if (pData) delete[] pData;
		pData = new float[(uint64_t)size * size * 4];
		memset(pData, 0, sizeof(float) * size * size * 4);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_RendererID);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float) * size * size * 4, pData);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	inline void Bind(int binding = -1)
	{
		if (binding >= 0) m_Binding = binding;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_RendererID);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_Binding, m_RendererID);
	}

	inline void UploadToGPU(size_t offset_, size_t size_)
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_RendererID);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset_, size_, (uint8_t*)pData + offset_);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	inline void UploadToGPU()
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_RendererID);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * size * size * 4, pData, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	inline size_t PixelCoordToDataOffset(int x, int y)
	{
		return (uint64_t)(x + y * size) * 4;
	}

	inline void SetPixelI(int x, int y, float r, float g, float b, float a)
	{
		float* pixel = &pData[PixelCoordToDataOffset(x, y)];
		pixel[0] = r; pixel[1] = g; pixel[2] = b; pixel[3] = a;
	}

	inline void SetPixelF(float tx, float ty, float r, float g, float b, float a)
	{
		int x = (int)(tx * (size - 1)); int y = (int)(ty * (size - 1));
		x = std::clamp(x, 0, size - 1); y = std::clamp(y, 0, size - 1);
		SetPixelI(x, y, r, g, b, a);
	}

	inline void GetPixelI(int x, int y, float* r = nullptr, float* g = nullptr, float* b = nullptr, float* a = nullptr)
	{
		float* pixel = &pData[PixelCoordToDataOffset(x, y)];
		if (r) *r = pixel[0];
		if (g) *g = pixel[1];
		if (b) *b = pixel[2];
		if (a) *a = pixel[3];
	}

	inline void GetPixelF(float tx, float ty, float* r = nullptr, float* g = nullptr, float* b = nullptr, float* a = nullptr)
	{
		int x = (int)(tx * (size - 1)); int y = (int)(ty * (size - 1));
		x = std::clamp(x, 0, size - 1); y = std::clamp(y, 0, size - 1);
		GetPixelI(x, y, r, g, b, a);
	}

	inline size_t GetSize() { return sizeof(float) * 4 * size * size; }

private:

	float* pData = nullptr;
	int size = 0;
	GLuint m_RendererID = 0;
	int m_Binding = 0;
};

