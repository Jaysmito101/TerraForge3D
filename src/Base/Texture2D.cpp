
#include "Texture2D.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include <avir/avir.h>

Texture2D::Texture2D(uint32_t width, uint32_t height)
		: m_Width(width), m_Height(height)
{
	m_InternalFormat = GL_RGB8;
	m_DataFormat = GL_RGB;
	m_Data = new unsigned char[width * height * 3];

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
	if (!data)
		return;
	glBindTexture(GL_TEXTURE_2D, m_RendererID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, m_Width, m_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
}

void Texture2D::Bind(uint32_t slot) const
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, m_RendererID);
}

void Texture2D::Resize(int width, int height, bool resetOpenGL) {
	if (m_Width == width && m_Height == height)
		return;
	if (!m_Data)
		return;
	unsigned char* data = (unsigned char*)malloc(width * height * 3 * sizeof(unsigned char));
	memset(data, 0, width * height * 3 * sizeof(unsigned char));
	avir::CImageResizer<> ImageResizer(8);
	ImageResizer.resizeImage(m_Data, m_Width, m_Height, 0, data, width, height, 3, 0);
	if (data) {
		if (m_Data)
			delete m_Data;
		m_Data = data;
		m_Width = width;
		m_Height = height;
		if (resetOpenGL) {
			glBindTexture(GL_TEXTURE_2D, m_RendererID);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, m_Width, m_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
	}
}

unsigned char* Texture2D::GetData() {
	if (m_Data)
		return m_Data;
	else
		return nullptr;
}
