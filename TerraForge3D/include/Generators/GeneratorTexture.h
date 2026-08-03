#pragma once
#include "Base/Base.h"

enum class GeneratorTextureStorage
{
	RGBA32F,
	R8,
	R16,
};

class GeneratorTexture
{
public:
	GeneratorTexture(int32_t width, int32_t height, GeneratorTextureStorage storage = GeneratorTextureStorage::RGBA32F);
	~GeneratorTexture();

	void Resize(int32_t width, int32_t height);
	int32_t Bind(int32_t slot);
	int32_t BindForCompute(int32_t binding);
	void SetPixel(float x, float y, float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);
	float* MakeCPUCopy();
	void FreeCPUCopy();
	void UploadCPUCopy();
	void ZeroCPUCopy();

	inline float* GetCPUCopy() const { return m_Data; }
	inline bool HasCPUCopy() const { return m_Data != nullptr; }
	inline const int32_t GetWidth() const { return m_Width; }
	inline const int32_t GetHeight() const { return m_Height; }
	inline const uint32_t GetRendererID() const { return m_RendererID; }
	inline const ImTextureID GetTextureID() const { return (ImTextureID)(intptr_t)m_RendererID; }
	inline const bool IsSingleChannel() const { return m_Storage == GeneratorTextureStorage::R8 || m_Storage == GeneratorTextureStorage::R16; }


private:
	int32_t m_Width = 0;
	int32_t m_Height = 0;
	uint32_t m_RendererID = 0;
	GLenum m_InternalFormat = 0;
	GLenum m_Format = 0;
	GLenum m_PixelType = 0;
	GeneratorTextureStorage m_Storage = GeneratorTextureStorage::RGBA32F;
	float* m_Data = nullptr;
};
