#include "ExportTexture.h"

#include <glad/glad.h>
#include <stb/stb_image_write.h>

void ExportTexture(int fbo, std::string path, int w, int h)
{
	if (path.size() < 3)
		return;

	if (path.find(".png") == std::string::npos)
		path += ".png";

	unsigned char* data = new unsigned char[w * h * 3];

	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);	
	glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	stbi_flip_vertically_on_write(true);

	stbi_write_png(path.c_str(), w, h, 3, data, w * 3);

	delete[] data;
}
