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
Mesh* meshToExport;
std::string meshExportFileName;

std::atomic<bool> isExportinghMap = false;
Mesh* meshhMapToExport;
std::string meshhMapExportFileName;


static void ExportOBJImpl() {

	std::ofstream outfile;
	std::string fileName = meshExportFileName;
	if (fileName.find(".obj") == std::string::npos)
		fileName += ".obj";

	if (meshToExport && meshToExport->IsValid()) {

		outfile.open(fileName);


		outfile << "# TerraGenV1.0 OBJ" << std::endl << std::endl;;

		for (int i = 0; i < meshToExport->vertexCount; i++)
		{
			outfile << "v " << meshToExport->vert[i].position.x << " " << meshToExport->vert[i].position.y << " " << meshToExport->vert[i].position.z << " " << std::endl;
		}

		outfile << std::endl;
		outfile << std::endl;

		for (int i = 0; i < meshToExport->vertexCount; i++)
		{
			outfile << "vt " << meshToExport->vert[i].texCoord.x << " " << meshToExport->vert[i].texCoord.y << std::endl;
		}

		outfile << std::endl;
		outfile << std::endl;

		for (int i = 0; i < meshToExport->vertexCount; i++)
		{
			outfile << "vn " << meshToExport->vert[i].normal.x << " " << meshToExport->vert[i].normal.y << " " << meshToExport->vert[i].normal.z << " " << std::endl;
		}

		outfile << std::endl;
		outfile << std::endl;

		int a = 0;
		int b = 0;
		int c = 0;

		for (int i = 0; i < meshToExport->indexCount; i += 3)
		{
			a = meshToExport->indices[i + 0] + 1;
			b = meshToExport->indices[i + 1] + 1;
			c = meshToExport->indices[i + 2] + 1;
			outfile << "f " << a << "/" << a << "/" << a << " " << b << "/" << b << "/" << b << " " << c << "/" << c << "/" << c << " " << std::endl;
		}

		outfile.close();

		delete meshToExport;
		Log((std::string("Exported Mesh to ") + fileName));
	}
	isExportingOBJ = false;
}

static void ExportHeightmapPNGImpl() {
	if (meshhMapExportFileName.find(".png") == std::string::npos)
		meshhMapExportFileName += ".png";

	if (meshhMapToExport && meshhMapToExport->IsValid())
	{
		int res = meshhMapToExport->res;
		unsigned char* heightMap = new unsigned char[res * res * 3];
		float t = 0;
		for (int i = 0; i < res; i++) {
			for (int j = 0; j < res; j++) {
				int ind = i + j * res;
				t = Clamp01(meshhMapToExport->vert[ind].position.y * 0.5f + 0.5f);
				heightMap[i * res * 3 + j * 3 + 0] = (unsigned char)(t * 255);
				heightMap[i * res * 3 + j * 3 + 1] = (unsigned char)(t * 255);
				heightMap[i * res * 3 + j * 3 + 2] = (unsigned char)(t * 255);
			}
		}
		stbi_write_png(meshhMapExportFileName.c_str(), res, res, 3, heightMap, res * 3);


		delete meshhMapToExport;
		delete heightMap;
	}
}


static void ExportHeightmapJPGImpl() {
	if (meshhMapExportFileName.find(".jpg") == std::string::npos)
		meshhMapExportFileName += ".jpg";

	if (meshhMapToExport && meshhMapToExport->IsValid())
	{
		int res = meshhMapToExport->res;
		unsigned char* heightMap = new unsigned char[res * res * 3];
		float t = 0;
		for (int i = 0; i < res; i++) {
			for (int j = 0; j < res; j++) {
				int ind = i + j * res;
				t = Clamp01(meshhMapToExport->vert[ind].position.y * 0.5f + 0.5f);
				heightMap[i * res * 3 + j * 3 + 0] = (unsigned char)(t * 255);
				heightMap[i * res * 3 + j * 3 + 1] = (unsigned char)(t * 255);
				heightMap[i * res * 3 + j * 3 + 2] = (unsigned char)(t * 255);
			}
		}
		stbi_write_jpg(meshhMapExportFileName.c_str(), res, res, 3, heightMap, 100);


		delete meshhMapToExport;
		delete heightMap;
	}
}


bool ExportOBJ(Mesh* mesh, std::string fileName)
{
	if (isExportingOBJ || fileName.size() < 4)
		return false;

	isExportingOBJ = false;
	meshToExport = mesh;
	meshExportFileName = fileName;
	std::thread t(ExportOBJImpl);
	t.detach();
	return true;
}

bool ExportHeightmapPNG(Mesh* mesh, std::string filename)
{
	if (isExportinghMap || filename.size()<4)
		return false;

	isExportinghMap = false;
	meshhMapToExport = mesh;
	meshhMapExportFileName = filename;
	std::thread t(ExportHeightmapPNGImpl);
	t.detach();
	return true;
}

bool ExportHeightmapJPG(Mesh* mesh, std::string filename)
{
	if (isExportinghMap || filename.size() < 4)
		return false;

	isExportinghMap = false;
	meshhMapToExport = mesh;
	meshhMapExportFileName = filename;
	std::thread t(ExportHeightmapJPGImpl);
	t.detach();
	return true;
}
