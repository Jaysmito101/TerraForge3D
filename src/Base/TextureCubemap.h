#pragma once
#include <string>
#include <vector>
#include <BaseMath.h>
#include <Texture2D.h>

#define TEXTURE_CUBEMAP_PX 0
#define TEXTURE_CUBEMAP_NX 1
#define TEXTURE_CUBEMAP_PY 2
#define TEXTURE_CUBEMAP_NY 3 
#define TEXTURE_CUBEMAP_PZ 4 
#define TEXTURE_CUBEMAP_NZ 5



class TextureCubemap {
public:
	TextureCubemap();
	~TextureCubemap();

	void SetUpOnGPU();
	bool LoadFaces(std::vector<std::string> paths);
	bool LoadFace(std::string path, int face);
	void DeleteData();
	bool UploadFaceToGPU(int face);
	void UploadDataToGPU();
	void Bind(int slot);

	Texture2D* textures[6];
	unsigned char* facesData[6];
	IVec2 facesSizes[6];
	uint32_t rendereID;
	std::vector<std::string> faces;
};