#define _CRT_SECURE_NO_WARNINGS

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib/httplib.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <Utils.h>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include  <cstdio>
#include  <cstdlib>
#ifdef _WIN32
#include <io.h>
#elif __linux__
#include <inttypes.h>
#include <unistd.h>
#define __int64 int64_t
#define _close close
#define _read read
#define _lseek64 lseek64
#define _O_RDONLY O_RDONLY
#define _open open
#define _lseeki64 lseek64
#define _lseek lseek
#define stricmp strcasecmp
#endif
//SAF_Handle.cpp line:458 old line:INFILE = _open(infilename, _O_RDONLY | _O_BINARY);
#ifdef __linux__
//  INFILE = _open(infilename, _O_RDONLY);
#else
//  INFILE = _open(infilename, _O_RDONLY | _O_BINARY);
#endif

#include "GLFW/glfw3.h"
#include "Application.h"
#include "Data/ProjectData.h"

#ifndef TERR3D_WIN32
#include <libgen.h>         // dirname
#include <unistd.h>         // readlink
#include <linux/limits.h>   // PATH_MAX
#define MAX_PATH PATH_MAX
#else
#include <atlstr.h>
#include <windows.h>
#include <Commdlg.h>
#endif


static std::string getExecutablePath()
{
	char rawPathName[MAX_PATH];
#ifdef TERR3D_WIN32
	GetModuleFileNameA(NULL, rawPathName, MAX_PATH);
#else
	readlink("/proc/self/exe", rawPathName, PATH_MAX);
#endif
	return std::string(rawPathName);
}

static std::string getExecutableDir()
{
	std::string executablePath = getExecutablePath();
	std::string directory = executablePath.substr(0, executablePath.find_last_of("\\/"));
	return directory;
}

const char HEX_MAP[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

char replace(unsigned char c)
{
	return HEX_MAP[c & 0x0f];
}

std::string UChar2Hex(unsigned char c)
{
	{
		std::string hex;
		// First four bytes
		char left = (c >> 4);
		// Second four bytes
		char right = (c & 0x0f);
		hex += replace(left);
		hex += replace(right);
		return hex;
	}
}

#ifdef TERR3D_WIN32

std::wstring s2ws(const std::string &s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t *buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}

#endif

std::string ShowSaveFileDialog(std::string ext)
{
#ifdef TERR3D_WIN32
	OPENFILENAME ofn;
	WCHAR fileName[MAX_PATH];
	ZeroMemory(fileName, MAX_PATH);
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	//ofn.lpstrFilter = s2ws(ext).c_str();
	ofn.lpstrFilter = L"*.*\0";
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = (LPWSTR)"";
	std::string fileNameStr;

	if (GetSaveFileName(&ofn))
	{
		std::wstring ws(ofn.lpstrFile);
		std::string str = "";
		for (auto ch : ws) str += (char)ch;
		return str;
	}

	return std::string("");
#else
	char filename[PATH_MAX];
	FILE *f = popen("zenity --file-selection --save", "r");
	fgets(filename, PATH_MAX, f);
	pclose(f);
	filename[strcspn(filename, "\n")] = 0;
	return std::string(filename);
#endif
}

std::string openfilename()
{
	return ShowOpenFileDialog(".obj");
}


bool DeleteFileT(std::string path)
{
	try
	{
		if (std::filesystem::remove(path))
		{
			std::cout << "File " << path << " deleted.\n";
		}

		else
		{
			std::cout << "File " << path << " could not be deleted.\n";
			return false;
		}
	}

	catch(const std::filesystem::filesystem_error &err)
	{
		std::cout << "filesystem error: " << err.what() << '\n';
		return false;
	}

	return true;
}

std::string ShowOpenFileDialog(std::string ext)
{
#ifdef TERR3D_WIN32
	OPENFILENAME ofn;
	WCHAR fileName[MAX_PATH];
	ZeroMemory(fileName, MAX_PATH);
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	//ofn.lpstrFilter = s2ws(ext).c_str();
	ofn.lpstrFilter = L"*.*\0";
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = (LPWSTR)"";
	std::string fileNameStr;

	if (GetOpenFileName(&ofn))
	{
		std::wstring ws(ofn.lpstrFile);
		// your new String
		std::string str(ws.begin(), ws.end());
		return str;
	}

	return std::string("");
#else
	char filename[PATH_MAX];
	FILE *f = popen("zenity --file-selection", "r");
	fgets(filename, PATH_MAX, f);
	pclose(f);
	filename[strcspn(filename, "\n")] = 0;
	return std::string(filename);
#endif
}

std::string ReadShaderSourceFile(std::string path, bool *result)
{
	std::fstream newfile;
	newfile.open(path.c_str(), std::ios::in);

	if (newfile.is_open())
	{
		std::string tp;
		std::string res = "";
		getline(newfile, res, '\0');
		newfile.close();
		*result = true;
		return res;
	}

	else
	{
		*result = false;
	}

	return std::string("");
}

std::string GetExecutablePath()
{
	return getExecutablePath();
}

std::string GetExecutableDir()
{
	return getExecutableDir();
}

std::string GenerateId(uint32_t length)
{
	std::string id;
	srand(time(NULL));

	for (int i = 0; i < length; i++)
	{
		id += std::to_string(rand() % 9);
	}

	return id;
}

std::string FetchURL(std::string baseURL, std::string path)
{
	httplib::Client cli(baseURL);
	auto res = cli.Get(path.c_str());
	cli.set_read_timeout(10);

	if (res.error() == httplib::Error::Success)
	{
		try
		{
			return res->body;
		}

		catch (...) {}
	}
	else
	{
		Log("Error in fetching " + baseURL+ path + " ERROR  : " + httplib::to_string(res.error()));
	}

	return "";
}

char *UChar2Char(unsigned char *data, int length)
{
	char *odata = new char[length];

	for (int i = 0; i < length; i++)
	{
		odata[i] = (char)data[i];
	}

	return odata;
}

bool FileExists(std::string path, bool writeAccess)
{
#ifdef TERR3D_WIN32

	if ((_access(path.c_str(), 0)) != -1)
	{
		if (!writeAccess)
		{
			return true;
		}

		if ((_access(path.c_str(), 2)) != -1)
		{
			return true;
		}
	}

	return false;
#else
	struct stat buffer;
	return (stat (path.c_str(), &buffer) == 0);
#endif
}

bool PathExist(const std::string &s)
{
	struct stat buffer;
	return (stat(s.c_str(), &buffer) == 0);
}


#ifdef TERR3D_WIN32
#include <wininet.h>
#pragma comment(lib, "Wininet.lib")
#endif

bool IsNetWorkConnected()
{
#ifdef TERR3D_WIN32
	bool bConnect = InternetCheckConnectionW(L"http://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0);
	return bConnect;
#else
	// Return true if internet connection is alive else false
	return true;
#endif
}

bool PowerOfTwoDropDown(const char* label, int32_t* value, int start, int end)
{
	if (!value) return false;
	static char buffer[32];
	int tmp = (int)(log((double)*value) / log(2.0));
	sprintf(buffer, "%d", (int)pow(2, tmp));
	if (ImGui::BeginCombo(label, buffer))
	{
		for (int i = start; i <= end; i++)
		{
			bool is_selected = (tmp == i);
			sprintf(buffer, "%d", (int)pow(2, i));
			if (ImGui::Selectable(buffer, is_selected)) tmp = i;
			if (is_selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	tmp = (int)pow(2, tmp);
	bool reslt = (*value != tmp);
	*value = tmp;
	return reslt;
}


bool ShowLayerUpdationMethod(const char* label, int* method)
{
	if (!method) return false;
	static const char* items[] = {"Set", "Add", "Subtract", "Multiply"};
	int selct = *method;
	if (ImGui::BeginCombo(label, items[selct]))
	{
		for (int i = 0; i <= 4; i++)
		{
			bool is_selected = (selct == i);
			if (ImGui::Selectable(items[i], is_selected)) selct= i;
			if (is_selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	bool reslt = (*method != selct);
	*method = selct;
	return reslt;
}

float UpdateLayerWithUpdateMethod(float origv, float newv, int method)
{
	if (method == 0) return newv;
	else if (method == 1) return origv + newv;
	else if (method == 2) return origv - newv;
	else if (method == 3) return origv * newv;
	return origv;
}

char *ReadBinaryFile(std::string path, int *fSize, uint32_t sizeToLoad)
{
	std::ifstream in(path, std::ifstream::ate | std::ifstream::binary);
	int size = in.tellg();
	in.close();

	if(sizeToLoad > 0)
	{
		size = size < sizeToLoad ? size : sizeToLoad;
	}

	std::ifstream f1(path, std::fstream::binary);

	if (size < 1)
	{
		size = 1024;
	}

	char *buffer = new char[size];
	f1.read(buffer, size);
	f1.close();
	*fSize = size;
	return buffer;
}

char *ReadBinaryFile(std::string path, uint32_t sizeToLoad)
{
	int size;
	return ReadBinaryFile(path, &size, sizeToLoad);
}

Hash MD5File(std::string path)
{
	int size = 1;
	unsigned char *data = (unsigned char *)ReadBinaryFile(path, &size);
	unsigned char hash[MD5_DIGEST_LENGTH];
	MD5(data, size, hash);
	Hash resultHash(hash, MD5_DIGEST_LENGTH);
	delete[] data;
	return resultHash;
}

void DownloadFile(std::string baseURL, std::string urlPath, std::string path, int size)
{
	try
	{
		std::ofstream outfile;
		httplib::Client cli(baseURL);
		int done = 0;
		outfile.open(path.c_str(), std::ios::binary | std::ios::out);
		std::cout << "Starting download - " << baseURL << urlPath << std::endl;
		auto res = cli.Get(urlPath.c_str(),
		                   [&](const char *data, size_t data_length)
		{
			done = done + data_length;

			if (size > 0)
			{
				float percent = (float)done / (float)size;
				std::cout << "Downloaded " << (int)(percent * 100) << "%      \r";
			}

			outfile.write(data, data_length);
			return true;
		});
		std::cout << "Download Complete - " << path << std::endl;
		outfile.close();
	}

	catch(...) {}
}

void SaveToFile(std::string filename, std::string content)
{
	std::ofstream outfile;
	outfile.open(filename);
	outfile << content;
	outfile.close();
}

void Log(const char *log)
{
	std::cout << log << std::endl;
};


void Log(std::string log)
{
	std::cout << log << std::endl;
};

#ifdef TERR3D_WIN32

// From https://stackoverflow.com/a/20256714/14911094
void RegSet(HKEY hkeyHive, const char *pszVar, const char *pszValue)
{
	HKEY hkey;
	char szValueCurrent[1000];
	DWORD dwType;
	DWORD dwSize = sizeof(szValueCurrent);
	int iRC = RegGetValue(hkeyHive, CA2CT(pszVar), NULL, RRF_RT_ANY, &dwType, szValueCurrent, &dwSize);
	bool bDidntExist = iRC == ERROR_FILE_NOT_FOUND;

	if (iRC != ERROR_SUCCESS && !bDidntExist)
	{
		Log("RegGetValue( " + std::string(pszVar) + " ) Failed : " + strerror(iRC));
	}

	if (!bDidntExist)
	{
		if (dwType != REG_SZ)
		{
			Log("RegGetValue( " + std::string(pszVar) + " ) found type unhandled " + std::to_string(dwType));
		}

		if (strcmp(szValueCurrent, pszValue) == 0)
		{
			Log("RegSet( \"" + std::string(pszVar) + "\" \"" + std::string(pszValue) + "\" ): already correct");
			return;
		}
	}

	DWORD dwDisposition;
	iRC = RegCreateKeyEx(hkeyHive, CA2CT(pszVar), 0, 0, 0, KEY_ALL_ACCESS, NULL, &hkey, &dwDisposition);

	if (iRC != ERROR_SUCCESS)
	{
		Log("RegSetValue( " + std::string(pszVar) + " ) Failed : " + strerror(iRC));
	}

	iRC = RegSetValueEx(hkey, L"", 0, REG_SZ, (BYTE *)pszValue, strlen(pszValue) + 1);

	if (iRC != ERROR_SUCCESS)
	{
		Log("RegSetValue( " + std::string(pszVar) + " ) Failed: " + strerror(iRC));
	}

	if (bDidntExist)
	{
		Log("RegSet( " + std::string(pszVar) +" ): set to \"" + std::string(pszValue) + "\"");
	}

	else
	{
		Log("RegSet( " + std::string(pszVar) +" ): changed \"" + std::string(szValueCurrent) + "\" to \"" + std::string(pszValue) + "\"");
	}

	RegCloseKey(hkey);
}

#endif

void AccocFileType()
{
#ifdef  TERR3D_WIN32
	RegSet(HKEY_CURRENT_USER, "Software\\Classes\\.terr3d", "TerraGen3D.TerraGen3D.1");
	// Not needed.
	RegSet(HKEY_CURRENT_USER, "Software\\Classes\\.terr3d\\Content Type", "application/json");
	RegSet(HKEY_CURRENT_USER, "Software\\Classes\\.terr3d\\PerceivedType", "json");
	//Not needed, but may be be a way to have wordpad show up on the default list.
	RegSet( HKEY_CURRENT_USER, "Software\\Classes\\.terr3d\\OpenWithProgIds\\TerraGen3D.TerraGen3D.1", "" );
	RegSet(HKEY_CURRENT_USER, "Software\\Classes\\TerraGen3D.TerraGen3D.1", "TerraGen3D");
	RegSet(HKEY_CURRENT_USER, "Software\\Classes\\TerraGen3D.TerraGen3D.1\\Shell\\Open\\Command", (getExecutablePath() + " %1").c_str());
#endif //  TERR3D_WIN32
}

void MkDir(std::string path)
{
	if (!PathExist(path))
	{
#ifdef TERR3D_WIN32
		system((std::string("mkdir \"") + path + "\"").c_str());
#else
		system((std::string("mkdir -p \"") + path + "\"").c_str());
#endif
	}
}

#ifndef TERR3D_WIN32
int get_file_size(char *source)
{
	FILE *fichier = fopen(source, "rb");
	fseek(fichier, 0, SEEK_END);
	int size = ftell(fichier);
	fseek(fichier, 0, SEEK_SET);
	fclose(fichier);
	return size;
}
#endif

void CopyFileData(std::string source, std::string destination)
{
#ifdef TERR3D_WIN32
	CopyFileW(CString(source.c_str()), CString(destination.c_str()), false);
#else
	int srcsize = get_file_size(source.data());
	char *data = (char *)malloc(srcsize);
	int fsource = open(source.c_str(), O_RDONLY);
	int fdest = open(destination.c_str(), O_WRONLY | O_CREAT, 0777);
	read(fsource, data, srcsize);
	write(fdest, data, srcsize);
	close(fsource);
	close(fdest);
	free(data);
#endif
}

bool IsKeyDown(int key)
{
	return glfwGetKey(Application::Get()->GetWindow()->GetNativeWindow(), key) == GLFW_PRESS;
}

bool IsMouseButtonDown(int button)
{
	return glfwGetMouseButton(Application::Get()->GetWindow()->GetNativeWindow(), button);
}

void ShowMessageBox(std::string message, std::string title)
{
#ifdef TERR3D_WIN32
	MessageBox(NULL, s2ws(message).c_str(), s2ws(title).c_str(), 0);
#else
	system(("zenity --info --text=" + message + " --title="+title + "").c_str());
#endif
}

bool LoadTexture(Texture2D *texture, bool loadToAssets, bool preserveData, bool readAlpha, ProjectManager *projectManager)
{
	std::string fileName = ShowOpenFileDialog(".png");

	if (fileName.size() > 3)
	{
		if (texture)
		{
			delete texture;
		}

		texture = new Texture2D(fileName, preserveData, readAlpha);

		if(projectManager == nullptr)
		{
			projectManager = ProjectManager::Get();
		}

		if (loadToAssets)
		{
			projectManager->SaveTexture(texture);
		}

		return true;
	}

	return false;
}

void ToggleSystemConsole()
{
	static bool state = false;
	state = !state;
#ifdef TERR3D_WIN32
	ShowWindow(GetConsoleWindow(), state ? SW_SHOW : SW_HIDE);
#else
	std::cout << "Toogle Console Not Supported on Linux!" << std::endl;
#endif
}

void OpenURL(std::string url)
{
#ifdef  TERR3D_WIN32
	std::string op = std::string("start ").append(url);
	system(op.c_str());
#else
	std::string op = std::string("xdg-open ").append(url);
	system(op.c_str());
#endif //  TERR3D_WIN32
}
