#define _CRT_SECURE_NO_WARNINGS


#include "ExportManager.h"
#include <Utils.h>
#include <fstream>
#include <iostream>
#include <atomic>
#include <thread>
#include <MathUtils.h>
#include <stb/stb_image.h>
#include "stb/stb_image_write.h"

std::atomic<bool> isExportingOBJ = false;
Mesh meshToExport;
std::string meshExportFileName;

std::atomic<bool> isExportinghMap = false;
Mesh meshhMapToExport;
std::string meshhMapExportFileName;


static void ExportOBJImpl() {

	std::ofstream outfile;
	std::string fileName = meshExportFileName;
	if (fileName.find(".obj") == std::string::npos)
		fileName += ".obj";

	if (meshToExport.vert && meshToExport.indices) {

		outfile.open(fileName);


		outfile << "# TerraGenV1.0 OBJ" << std::endl << std::endl;;

		for (int i = 0; i < meshToExport.vertexCount; i++)
		{
			outfile << "v " << meshToExport.vert[i].position.x << " " << meshToExport.vert[i].position.y << " " << meshToExport.vert[i].position.z << " " << std::endl;
		}

		outfile << std::endl;
		outfile << std::endl;

		for (int i = 0; i < meshToExport.indexCount; i += 3)
		{
			outfile << "f " << meshToExport.indices[i] + 1 << " " << meshToExport.indices[i + 1] + 1 << " " << meshToExport.indices[i + 2] + 1 << " " << std::endl;
		}

		outfile.close();

		delete meshToExport.indices;
		delete meshToExport.vert;
		Log((std::string("Exported Mesh to ") + fileName));
	}
	isExportingOBJ = false;
}

static void ExportHeightmapPNGImpl() {
	if (meshhMapToExport.vert && meshhMapToExport.indices)
	{
		unsigned char* heightMap = new unsigned char[meshhMapToExport.res * meshhMapToExport.res * 3];
		float t = 0;
		int res = meshhMapToExport.res;
		for (int i = 0; i < res; i++) {
			for (int j = 0; j < res; j++) {
				int ind = i + j * res;
				t = (meshhMapToExport.vert[ind].position.y * 0.5f + 0.5f);
				heightMap[i * res * 3 + j * 3 + 0] = (unsigned char)(t * 255);
				heightMap[i * res * 3 + j * 3 + 1] = (unsigned char)(t * 255);
				heightMap[i * res * 3 + j * 3 + 2] = (unsigned char)(t * 255);
			}
		}
		stbi_write_png(meshhMapExportFileName.c_str(), meshhMapToExport.res, meshhMapToExport.res, 3, heightMap, meshhMapToExport.res * 3);


		delete meshhMapToExport.vert;
		delete meshhMapToExport.indices;
		delete heightMap;
	}
}

bool ExportOBJ(Mesh mesh, std::string fileName)
{
	if (isExportingOBJ || fileName.size() < 4)
		return false;

	isExportingOBJ = false;
	meshToExport = mesh;
	meshExportFileName = fileName;
	std::thread t(ExportOBJImpl);
	t.detach();
	mesh.deleteOnDestruction = false;
	return true;
}

bool ExportHeightmapPNG(Mesh mesh, std::string filename)
{
	if (isExportinghMap || filename.size()<4)
		return false;

	isExportinghMap = false;
	meshhMapToExport = mesh;
	meshhMapExportFileName = filename;
	std::thread t(ExportHeightmapPNGImpl);
	t.detach();
	mesh.deleteOnDestruction = false;
	return true;
}
