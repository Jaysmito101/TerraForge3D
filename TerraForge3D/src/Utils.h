#pragma once



#include <Mesh.h>
#include <string>



std::string UChar2Hex(unsigned char c);

struct Hash {

	Hash() {

	}

	Hash(unsigned char* d, int l) {
		length = l;
		data = new unsigned char[length];
		memcpy_s(data, length, d, l);
	}

	~Hash() {
		//delete[] data;
	}

	std::string ToString() {
		std::string hash;
		for (int i = 0; i < length; i++) 
		{
			
			hash += UChar2Hex(data[i]);
		}
		return hash;
	}

	int length = 0;
	unsigned char* data;
};


#define MAX(x, y) (x > y ? x : y)

std::string ShowSaveFileDialog(std::string ext = ".terr3d");

std::string openfilename();

std::string ShowOpenFileDialog(std::string ext = "*.glsl");

std::string ReadShaderSourceFile(std::string path, bool* result);

std::string GetExecutablePath();

std::string GetExecutableDir();

std::string GenerateId(uint32_t length);

std::string FetchURL(std::string baseURL, std::string path);

char* UChar2Char(unsigned char* data, int length);

bool FileExists(std::string path, bool writeAccess = false);

bool PathExist(const std::string& s);

bool IsNetWorkConnected();

char* ReadBinaryFile(std::string path, int* size, uint32_t sizeToLoad = -1);

char* ReadBinaryFile(std::string path, uint32_t sizeToLoad = -1);

Hash MD5File(std::string path);

void DownloadFile(std::string baseURL, std::string urlPath, std::string path, int size = -1);

void SaveToFile(std::string filename, std::string content = "");

void Log(const char* log);

void Log(std::string log);

#ifdef TERR3D_WIN32
#include <windows.h>
void RegSet(HKEY hkeyHive, const char* pszVar, const char* pszValue);
#endif

void AccocFileType();

void MkDir(std::string path);

void CopyFileData(std::string source, std::string destination);