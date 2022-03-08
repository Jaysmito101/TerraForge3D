#define _CRT_SECURE_NO_WARNINGS


#include "ExportManager.h"
#include <Utils.h>
#include <fstream>
#include <iostream>
#include <atomic>
#include <thread>
#include <MathUtils.h>
#include <Model.h>

#include <stb/stb_image.h>
#include "stb/stb_image_write.h"
#undef __cplusplus // Temporary
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>


void ExportModelAssimp(Model *model, std::string format, std::string path, std::string extension)
{
	if(path.size() < 3)
	{
		return;
	}

	if (extension == "")
	{
		extension = format;
	}

	if (path.find("." + extension) == std::string::npos)
	{
		path += "." + extension;
	}

	Assimp::Exporter exporter;
	aiScene *scene = new aiScene();
	scene->mRootNode = new aiNode();
	scene->mMaterials = new aiMaterial * [1];
	scene->mMaterials[0] = nullptr;
	scene->mNumMaterials = 1;
	scene->mMaterials[0] = new aiMaterial();
	scene->mMeshes = new aiMesh * [1];
	scene->mMeshes[0] = nullptr;
	scene->mNumMeshes = 1;
	scene->mMeshes[0] = new aiMesh();
	scene->mMeshes[0]->mMaterialIndex = 0;
	scene->mRootNode->mMeshes = new unsigned int[1];
	scene->mRootNode->mMeshes[0] = 0;
	scene->mRootNode->mNumMeshes = 1;
	auto pMesh = scene->mMeshes[0];
	pMesh->mVertices = new aiVector3D[model->mesh->vertexCount];
	pMesh->mNumVertices = model->mesh->vertexCount;
	pMesh->mTextureCoords[0] = new aiVector3D[model->mesh->vertexCount];
	pMesh->mNumUVComponents[0] = model->mesh->vertexCount;

	for (int i = 0; i < model->mesh->vertexCount; i++)
	{
		glm::vec4 &v = model->mesh->vert[i].position;
		glm::vec2 &t = model->mesh->vert[i].texCoord;
		pMesh->mVertices[i] = aiVector3D(v.x, v.y, v.z);
		pMesh->mTextureCoords[0][i] = aiVector3D(t.x, t.y, 0);
	}

	pMesh->mFaces = new aiFace[model->mesh->indexCount / 3];
	pMesh->mNumFaces = model->mesh->indexCount / 3;

	for (int i = 0, j=0; i < model->mesh->indexCount ; i+=3)
	{
		aiFace &face = pMesh->mFaces[j++];
		face.mIndices = new unsigned int[3];
		face.mNumIndices = 3;
		face.mIndices[0] = model->mesh->indices[i + 0];
		face.mIndices[1] = model->mesh->indices[i + 1];
		face.mIndices[2] = model->mesh->indices[i + 2];
	}

	aiReturn r= exporter.Export(scene, format, path);
	Log(exporter.GetErrorString());
}
