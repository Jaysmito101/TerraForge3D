#include "Heightmap.h"
#include <string>
#include <cmath>
#include <iostream>

#include <stb/stb_image.h>
#include <glad/glad.h>

Heightmap::Heightmap(const std::string path)
{
    m_Path = path;
	int width, height, channels;
	stbi_set_flip_vertically_on_load(0);
	void* data = stbi_load_16(path.c_str(), &width, &height, &channels, 1);
    if (!data)
    {
        std::cout << "Failed to load heightmap (" << path << "): " << stbi_failure_reason() << std::endl;
        return;
    }
    m_Data = static_cast<uint16_t*>(data);
    m_Width = width;
    m_Height = height;

    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    std::vector<uint8_t> texData;
    for (int i = 0; i < width * height; ++i)
    {
        uint8_t value = (uint8_t)(static_cast<float>(m_Data[i]) / (2 << 15) * 255);
        texData.push_back(value);
        texData.push_back(value);
        texData.push_back(value);
        texData.push_back(255);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
}

Heightmap::~Heightmap()
{
	if (m_Data)
	{
		glDeleteTextures(1, &m_RendererID);
        free(m_Data);
	}
}

uint16_t Heightmap::Sample(float x, float y, bool interpolated) const 
{
    auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
    auto clamp = [](float x, float min, float max) { 
        return x > max ? max : (x < min) ? min : x;
    };

    clamp(x, 0, 1);
    clamp(y, 0, 1);

    if (!interpolated)
        return Get((uint32_t)(x * (m_Width - 1)), (uint32_t)(y * (m_Height - 1)));

    float x1 = std::floor(x * (m_Width - 1));
    float y1 = std::floor(y * (m_Height - 1));
    float x2 = x1 + 1;
    float y2 = y1 + 1;

    float xp = x * m_Width - x1;
    float yp = y * m_Height - y1;

    float h11 = Get(static_cast<uint32_t>(x1), static_cast<uint32_t>(y1));
    float h21 = Get(static_cast<uint32_t>(x2), static_cast<uint32_t>(y1));
    float h12 = Get(static_cast<uint32_t>(x1), static_cast<uint32_t>(y2));
    float h22 = Get(static_cast<uint32_t>(x2), static_cast<uint32_t>(y2));

    auto v1 = lerp(h11, h21, xp);
    auto v2 = lerp(h12, h22, xp);
    auto h = lerp(v1, v2, yp);
    if (h < 0)
        h = 0;
    return (uint16_t) h;

}

uint16_t Heightmap::Get(uint32_t x, uint32_t y) const 
{
    return m_Data[x + m_Width * y];
}