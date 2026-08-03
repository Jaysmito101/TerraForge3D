#include "Generators/GeneratorTexture.h"

GeneratorTexture::GeneratorTexture(int32_t width, int32_t height, GeneratorTextureStorage storage)
{
    m_Width = width; m_Height = height;
    m_Storage = storage;
    if (m_Storage == GeneratorTextureStorage::R8)
    {
        m_Format = GL_RED;
        m_InternalFormat = GL_R8;
        m_PixelType = GL_UNSIGNED_BYTE;
    }
    else if (m_Storage == GeneratorTextureStorage::R16)
    {
        m_Format = GL_RED;
        m_InternalFormat = GL_R16;
        m_PixelType = GL_UNSIGNED_SHORT;
    }
    else
    {
        m_Format = GL_RGBA;
        m_InternalFormat = GL_RGBA32F;
        m_PixelType = GL_FLOAT;
    }
    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_Format, m_PixelType, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	if (IsSingleChannel())
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
	}
	if (IsSingleChannel())
	{
		if (m_Storage == GeneratorTextureStorage::R16)
		{
			const uint16_t zero = 0;
			glClearTexImage(m_RendererID, 0, m_Format, m_PixelType, &zero);
		}
		else
		{
			const uint8_t zero = 0;
			glClearTexImage(m_RendererID, 0, m_Format, m_PixelType, &zero);
		}
	}
	else
	{
		const float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearTexImage(m_RendererID, 0, m_Format, m_PixelType, zero);
	}
    glBindTexture(GL_TEXTURE_2D, 0);
}

GeneratorTexture::~GeneratorTexture()
{
    glDeleteTextures(1, &m_RendererID);
}

void GeneratorTexture::Resize(int32_t width, int32_t height)
{
    if (width == m_Width && height == m_Height) return;
    if (width == 0 || height == 0) return;
    m_Width = width; m_Height = height;
	glBindTexture(GL_TEXTURE_2D, m_RendererID);
	glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_Format, m_PixelType, nullptr);
	if (IsSingleChannel())
	{
		if (m_Storage == GeneratorTextureStorage::R16)
		{
			const uint16_t zero = 0;
			glClearTexImage(m_RendererID, 0, m_Format, m_PixelType, &zero);
		}
		else
		{
			const uint8_t zero = 0;
			glClearTexImage(m_RendererID, 0, m_Format, m_PixelType, &zero);
		}
	}
	else
	{
		const float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearTexImage(m_RendererID, 0, m_Format, m_PixelType, zero);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}

float* GeneratorTexture::MakeCPUCopy()
{
	const int channels = IsSingleChannel() ? 1 : 4;
	m_Data = new float[m_Width * m_Height * channels];
	glBindTexture(GL_TEXTURE_2D, m_RendererID);
	glGetTexImage(GL_TEXTURE_2D, 0, m_Format, GL_FLOAT, m_Data);
	glBindTexture(GL_TEXTURE_2D, 0);
    return m_Data;
}

void GeneratorTexture::FreeCPUCopy()
{
    if (m_Data != nullptr)
    {
		delete[] m_Data;
		m_Data = nullptr;
	}
}

void GeneratorTexture::UploadCPUCopy()
{
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	if (IsSingleChannel())
	{
		if (m_Storage == GeneratorTextureStorage::R16)
		{
			std::vector<uint16_t> values(m_Width * m_Height);
			for (size_t i = 0; i < values.size(); i++) values[i] = static_cast<uint16_t>(glm::clamp(m_Data[i], 0.0f, 1.0f) * 65535.0f + 0.5f);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, m_Format, m_PixelType, values.data());
		}
		else
		{
			std::vector<uint8_t> values(m_Width * m_Height);
			for (size_t i = 0; i < values.size(); i++) values[i] = static_cast<uint8_t>(glm::clamp(m_Data[i], 0.0f, 1.0f) * 255.0f + 0.5f);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, m_Format, m_PixelType, values.data());
		}
	}
	else
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, m_Format, m_PixelType, m_Data);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GeneratorTexture::ZeroCPUCopy()
{
    if (m_Data != nullptr)
	{
		const int channels = IsSingleChannel() ? 1 : 4;
		memset(m_Data, 0, m_Width * m_Height * channels * sizeof(float));
	}
}

int32_t GeneratorTexture::Bind(int32_t slot)
{
    glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, m_RendererID);
    return slot;
}

int32_t GeneratorTexture::BindForCompute(int32_t binding)
{
    // glBindTexture(GL_TEXTURE_2D, m_RendererID);
    glBindImageTexture(binding, m_RendererID, 0, GL_FALSE, 0, GL_READ_WRITE, m_InternalFormat);
    return binding;
}

void GeneratorTexture::SetPixel(float x, float y, float r, float g, float b, float a)
{
    int32_t pixelX = std::clamp((int32_t)(x * m_Width), 0, m_Width - 1);
    int32_t pixelY = std::clamp((int32_t)(y * m_Height), 0, m_Height - 1);
    if (m_Data == nullptr)
    {
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		if (IsSingleChannel())
		{
			if (m_Storage == GeneratorTextureStorage::R16)
			{
				const uint16_t data = static_cast<uint16_t>(glm::clamp(r, 0.0f, 1.0f) * 65535.0f + 0.5f);
				glTexSubImage2D(GL_TEXTURE_2D, 0, pixelX, pixelY, 1, 1, m_Format, m_PixelType, &data);
			}
			else
			{
				const uint8_t data = static_cast<uint8_t>(glm::clamp(r, 0.0f, 1.0f) * 255.0f + 0.5f);
				glTexSubImage2D(GL_TEXTURE_2D, 0, pixelX, pixelY, 1, 1, m_Format, m_PixelType, &data);
			}
		}
		else
		{
			float data[] = {r, g, b, a};
			glTexSubImage2D(GL_TEXTURE_2D, 0, pixelX, pixelY, 1, 1, m_Format, m_PixelType, data);
		}
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    else
    {
		if (IsSingleChannel())
		{
			m_Data[pixelY * m_Width + pixelX] = r;
		}
		else
		{
			m_Data[(pixelY * m_Width + pixelX) * 4 + 0] = r;
			m_Data[(pixelY * m_Width + pixelX) * 4 + 1] = g;
			m_Data[(pixelY * m_Width + pixelX) * 4 + 2] = b;
			m_Data[(pixelY * m_Width + pixelX) * 4 + 3] = a;
		}
    }
}

