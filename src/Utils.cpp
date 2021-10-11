#define _CRT_SECURE_NO_WARNINGS

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib/httplib.h>

#include <Utils.h>
#include <fstream>
#include <iostream>
#include  <io.h>
#include  <stdio.h>
#include  <stdlib.h>


static std::string getExecutablePath() {
	char rawPathName[MAX_PATH];
	GetModuleFileNameA(NULL, rawPathName, MAX_PATH);
	return std::string(rawPathName);
}

static std::string getExecutableDir() {
	std::string executablePath = getExecutablePath();
	std::string directory = executablePath.substr(0, executablePath.find_last_of("\\/"));
	return directory;
}



std::string ShowSaveFileDialog(std::string ext , HWND owner ) {
	OPENFILENAME ofn;
	WCHAR fileName[MAX_PATH];
	ZeroMemory(fileName, MAX_PATH);
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = owner;
	ofn.lpstrFilter = L"*.terr3d\0";
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = (LPWSTR)"";

	std::string fileNameStr;

	if (GetSaveFileName(&ofn)) {
		std::wstring ws(ofn.lpstrFile);
		// your new String
		std::string str(ws.begin(), ws.end());
		return str;
	}
	return std::string("");
}

std::string openfilename(HWND owner) {
	OPENFILENAME ofn;
	WCHAR fileName[MAX_PATH];
	ZeroMemory(fileName, MAX_PATH);
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = owner;
	ofn.lpstrFilter = L"*.obj\0";
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = (LPWSTR)"";

	std::string fileNameStr;

	if (GetSaveFileName(&ofn)) {
		std::wstring ws(ofn.lpstrFile);
		// your new String
		std::string str(ws.begin(), ws.end());
		return str;
	}
	return std::string("");
}

std::string ShowOpenFileDialog(LPWSTR ext, HWND owner) {
	OPENFILENAME ofn;
	WCHAR fileName[MAX_PATH];
	ZeroMemory(fileName, MAX_PATH);
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = owner;
	ofn.lpstrFilter = ext;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = (LPWSTR)"";

	std::string fileNameStr;

	if (GetOpenFileName(&ofn)) {
		std::wstring ws(ofn.lpstrFile);
		// your new String
		std::string str(ws.begin(), ws.end());
		return str;
	}
	return std::string("");
}

std::string ReadShaderSourceFile(std::string path, bool* result) {
	std::fstream newfile;
	newfile.open(path.c_str(), std::ios::in);
	if (newfile.is_open()) {
		std::string tp;
		std::string res = "";
		getline(newfile, res, '\0');
		newfile.close();
		*result = true;
		return res;
	}
	else {
		*result = false;
	}
	return std::string("");
}

std::string GetExecutableDir()
{
	return getExecutableDir();
}

std::string FetchURL(std::string baseURL, std::string path){
	httplib::Client cli(baseURL);
	auto res = cli.Get(path.c_str());
	//if(res->status == 200)
	return res->body;
	return "";
}

bool FileExists(std::string path, bool writeAccess){
	if( (_access( path.c_str(), 0 )) != -1 )
	{
		if(!writeAccess)
			return true;
		if( (_access( path.c_str(), 2 )) != -1 )
			return true;
	}
	return false;
}

void DownloadFile(std::string baseURL, std::string urlPath, std::string path, int size){
	std::ofstream outfile;
	httplib::Client cli(baseURL);
	int done = 0;
	outfile.open(path.c_str(), std::ios::binary | std::ios::out);
	std::cout << "Starting download - " << baseURL << urlPath << std::endl;
	auto res = cli.Get(urlPath.c_str(),
		[&](const char *data, size_t data_length) {
			done = done + data_length;
			if (size > 0) {
				float percent = (float)done / (float)size;
				std::cout << "Downloaded " << (int)(percent * 100) << "%      \r";
			}
			outfile.write(data, data_length);  
			return true;
		});
	std::cout << "Download Complete - " << path << std::endl;
	outfile.close();
}

void SaveToFile(std::string filename, std::string content){
	std::ofstream outfile;
	outfile.open(filename);
	outfile << content;
	outfile.close();
}

void Log(const char* log)
{
	std::cout << log << std::endl;
};


void Log(std::string log)
{
	std::cout << log << std::endl;
};

