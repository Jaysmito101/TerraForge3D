
#include "Texture2D.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

Texture2D::Texture2D(uint32_t width, uint32_t height)
		: m_Width(width), m_Height(height)
{
	m_InternalFormat = GL_RGB8;
	m_DataFormat = GL_RGB;

	glGenTextures(1, &m_RendererID);
	glBindTexture(GL_TEXTURE_2D, m_RendererID);

	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

Texture2D::Texture2D(const std::string path)
		: m_Path(path)
{
	int width, height, channels;
	stbi_set_flip_vertically_on_load(0);
	unsigned char* data = nullptr;

	
	data = stbi_load(path.c_str(), &width, &height, &channels, 3);
	
		
	if (data)
	{
		m_Data = data;
		m_IsLoaded = true;

		m_Width = width;
		m_Height = height;

		glGenTextures(1, &m_RendererID);
		glBindTexture(GL_TEXTURE_2D, m_RendererID);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	}
}

Texture2D::~Texture2D()
{
	if (m_Data)
		stbi_image_free(m_Data);
		glDeleteTextures(1, &m_RendererID);
}

void Texture2D::SetData(void* data, uint32_t size)
{
	uint32_t bpp = 3;
	glBindTexture(GL_TEXTURE_2D, m_RendererID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, m_Width, m_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
}

void Texture2D::Bind(uint32_t slot) const
{
	glBindTextureUnit(slot, m_RendererID);
}

