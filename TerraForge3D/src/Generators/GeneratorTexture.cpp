#include "Generators/GeneratorTexture.h"

GeneratorTexture::GeneratorTexture(int32_t width, int32_t height)
{
    m_Width = width; m_Height = height;
    m_Format = GL_RGBA;
    m_InternalFormat = GL_RGBA32F;
    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_Format, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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
	glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_Format, GL_FLOAT, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);
}

float* GeneratorTexture::MakeCPUCopy()
{
    m_Data = new float[m_Width * m_Height * 4];
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
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, m_Format, GL_FLOAT, m_Data);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GeneratorTexture::ZeroCPUCopy()
{
    if (m_Data != nullptr) memset(m_Data, 0, m_Width * m_Height * 4 * sizeof(float));
}

int32_t GeneratorTexture::Bind(int32_t slot)
{
    glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, m_RendererID);
    return slot;
}

void GeneratorTexture::SetPixel(float x, float y, float r, float g, float b, float a)
{
    int32_t pixelX = std::clamp((int32_t)(x * m_Width), 0, m_Width - 1);
    int32_t pixelY = std::clamp((int32_t)(y * m_Height), 0, m_Height - 1);
    if (m_Data == nullptr)
    {
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        float data[] = {r, g, b, a};
        glTexSubImage2D(GL_TEXTURE_2D, 0, pixelX, pixelY, 1, 1, m_Format, GL_FLOAT, data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    else
    {
        m_Data[(pixelY * m_Width + pixelX) * 4 + 0] = r;
        m_Data[(pixelY * m_Width + pixelX) * 4 + 1] = g;
        m_Data[(pixelY * m_Width + pixelX) * 4 + 2] = b;
        m_Data[(pixelY * m_Width + pixelX) * 4 + 3] = a;
    }
}

