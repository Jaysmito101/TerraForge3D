#include "TextureCubemap.h"

#include <iostream>
#include <stb/stb_image.h>
#include <glad/glad.h>

#include <BaseMath.h>

TextureCubemap::TextureCubemap()
{
	faces.push_back("");
	faces.push_back("");
	faces.push_back("");
	faces.push_back("");
	faces.push_back("");
	faces.push_back("");
}


TextureCubemap::~TextureCubemap()
{
	for (int i = 0; i < 6; i++)
		if (facesData[i])
			delete[] facesData[i];
	for (int i = 0; i < 6; i++)
		if (textures[i])
			delete textures[i];
	glDeleteTextures(1, &rendereID);
}

void TextureCubemap::SetUpOnGPU()
{
	glGenTextures(1, &rendereID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, rendereID);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

bool TextureCubemap::LoadFaces(std::vector<std::string> paths)
{
	bool res = true;
	int face = 0;
	for (std::string path : paths) {
		res = LoadFace(path, face++);
	}
	return res;
}

bool TextureCubemap::LoadFace(std::string path, int face)
{
	bool res = true;
	int width, height, nrChannels;
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 3);
	facesSizes[face] = IVec2(width, height);
	if (data)
	{
		std::cout << "Loaded : " << path << "\n";
		if(facesData[face])
			stbi_image_free(facesData[face]);
		facesData[face] = data;
		res = true;
	}
	else
	{
		res = false;
		std::cout << "Failed to load : " << path << std::endl;
		stbi_image_free(data);
	}
	faces[face] = path;
	return res;
}

void TextureCubemap::DeleteData()
{
	for (int i = 0; i < 6; i++)
		if (facesData[i])
			delete[] facesData[i];
}

bool TextureCubemap::UploadFaceToGPU(int face)
{
	glBindTexture(GL_TEXTURE_CUBE_MAP, rendereID);
	if (facesData[face]) {
		if (textures[face])
			delete textures[face];
		textures[face] = new Texture2D(facesSizes[face].x, facesSizes[face].y);
		textures[face]->Bind();
		textures[face]->SetData(facesData[face], facesSizes[face].x * facesSizes[face].y * 3);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB, facesSizes[face].x, facesSizes[face].y, 0, GL_RGB, GL_UNSIGNED_BYTE, facesData[face]);
		return true;
	}
	return false;

}

void TextureCubemap::UploadDataToGPU()
{
	for (int i = 0; i < 6; i++) {
		UploadFaceToGPU(i);
	}
}

void TextureCubemap::Bind(int slot)
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_CUBE_MAP, rendereID);
}
