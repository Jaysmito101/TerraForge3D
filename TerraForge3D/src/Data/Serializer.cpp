#include "Data/Serializer.h"
#include "Utils/Utils.h"
#include "Base/ModelImporter.h"
#include "Data/ProjectData.h"
#include "Data/VersionInfo.h"
#include "Data/ApplicationState.h"
#include "Misc/AppStyles.h"
#include "Shading/ShadingManager.h"
#include "Platform.h"

#ifdef TERR3D_WIN32
#include "dirent/dirent.h"
#else
#include "sys/stat.h"
#include "dirent.h"
#endif
#include "zip.h"

#include <fstream>
#include <iostream>


#define TRY_CATCH_ERR_SERIALIZE(code, message) try{ code } catch(...){onError(message, true);}
#define TRY_CATCH_ERR_DESERIALIZE(code, message) try{ code } catch(...){onError(message, false);}
#define TRY_CATCH_ERR_DESERIALIZE_WITH_CODE(code, message, errorCode) try{ code } catch(...){onError(message, false); errorCode}

static void zip_walk(struct zip_t *zip, const char *path, bool isFristLayer = true, std::string prevPath = "")
{
	DIR *dir;
	struct dirent *entry;
	char fullpath[MAX_PATH];
	struct stat s;
	memset(fullpath, 0, MAX_PATH);
	dir = opendir(path);
	assert(dir);

	while ((entry = readdir(dir)))
	{
		// skip "." and ".."
		if (!strcmp(entry->d_name, ".\0") || !strcmp(entry->d_name, "..\0"))
		{
			continue;
		}

		snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
		stat(fullpath, &s);

		if (S_ISDIR(s.st_mode))
		{
			zip_walk(zip, fullpath, false, prevPath + entry->d_name + PATH_SEPARATOR);
		}

		else
		{
			zip_entry_open(zip, (prevPath + entry->d_name).c_str());
			zip_entry_fwrite(zip, fullpath);
			zip_entry_close(zip);
		}
	}

	closedir(dir);
}


Serializer::Serializer(ApplicationState *state)
	:appState(state)
{
	onError = [](std::string message, bool whileSerialize)
	{
		std::cout << (whileSerialize ? "Serailizer Error : " : "Deserailizer Error : ") << message << std::endl;;
	};
}

Serializer::Serializer(ApplicationState *state, std::function<void(std::string, bool)> errorFunc)
	:appState(state)
{
}

Serializer::~Serializer()
{
}

nlohmann::json Serializer::Serialize()
{
	nlohmann::json tmp = nlohmann::json();
	return tmp;
}

void Serializer::PackProject(std::string path)
{
	if (path.find(".terr3dpack") == std::string::npos)
	{
		path = path + ".terr3dpack";
	}

	Serialize();
	std::ofstream outfile;
	outfile.open(appState->projectManager->GetResourcePath() + PATH_SEPARATOR "project.terr3d");
	outfile << data.dump(4, ' ', false);
	outfile.close();
	MkDir(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "temp");
	std::string uid = GenerateId(64);
	SaveFile(GetExecutableDir() + "Data" + PATH_SEPARATOR "temp" PATH_SEPARATOR + uid);
	zip_t *packed = zip_open(path.c_str(), 9, 'w');
	zip_walk(packed, appState->projectManager->GetResourcePath().c_str());
	zip_close(packed);
}

void Serializer::LoadPackedProject(std::string path)
{
	if (path.find(".terr3dpack") == std::string::npos)
	{
		path = path + ".terr3dpack";
	}

	std::string uid = GenerateId(64);
	MkDir(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "temp"
			PATH_SEPARATOR "pcache_" + uid);
	zip_extract(path.c_str(), (GetExecutableDir() + PATH_SEPARATOR "Data"
				PATH_SEPARATOR "temp" PATH_SEPARATOR "pcache_" + uid).c_str(),
			[](const char *filename, void *arg)
	{
		return 1;
	}, (void *)0);

	if (FileExists(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "temp" PATH_SEPARATOR "pcache_" + uid + PATH_SEPARATOR "project.terr3d", true))
	{
		bool tmp = false;
		std::string sdata = ReadShaderSourceFile(GetExecutableDir() +
				PATH_SEPARATOR "Data" PATH_SEPARATOR "temp" PATH_SEPARATOR
				"pcache_" + uid + PATH_SEPARATOR "project.terr3d", &tmp);

		if (sdata.size() <= 0)
		{
			Log("Emppty Project File!");
			return;
		}

		nlohmann::json data;

		try
		{
			data = nlohmann::json::parse(sdata);
		}

		catch (...)
		{
			Log("Failed to Parse Project file!");
			return;
		}

		std::string projId = data["projectID"];
		zip_extract(path.c_str(), (GetExecutableDir() + PATH_SEPARATOR "Data"
					PATH_SEPARATOR "cache" PATH_SEPARATOR "project_data"
					PATH_SEPARATOR "project_" + projId).c_str(), [](const char
						*filename, void *arg)
		{
			Log(std::string("Extracted ") + filename);
			return 1;
		}, (void *)0);
		std::string oriDir = path.substr(0, path.rfind(PATH_SEPARATOR));
		std::string oriName = path.substr(path.rfind(PATH_SEPARATOR) + 1);
		CopyFileData((GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR
					"cache" PATH_SEPARATOR "project_data" PATH_SEPARATOR
					"project_" + projId + PATH_SEPARATOR
					"project.terr3d").c_str(), oriDir + PATH_SEPARATOR +
				oriName + ".terr3d");
		LoadFile(oriDir + PATH_SEPARATOR + oriName + ".terr3d");
	}

	else
	{
		Log("Not a valid terr3dpack file!");
	}
}

void Serializer::SaveFile(std::string path)
{
	if (path.size() <= 3)
	{
		return;
	}

	if (path.find(".terr3d") == std::string::npos)
	{
		path = path + ".terr3d";
	}

	if (path != appState->globals.currentOpenFilePath)
	{
		appState->globals.currentOpenFilePath = path;
	}

	Serialize();
	std::ofstream outfile;
	outfile.open(appState->projectManager->GetResourcePath() + PATH_SEPARATOR "project.terr3d");
	outfile << data.dump(4, ' ', false);
	outfile.close();
	outfile.open(path);
	outfile << data.dump(4, ' ', false);
	outfile.close();
}

void Serializer::LoadFile(std::string path)
{
	if (path.size() <= 3)
	{
		return;
	}

	if (path.find(".terr3d") == std::string::npos)
	{
		path = path + ".terr3d";
	}

	bool flagRd = true;
	std::string sdata = ReadShaderSourceFile(path, &flagRd);

	if (!flagRd)
	{
		Log("Could not read file : " + path);
		return;
	}

	if (sdata.size() == 0)
	{
		Log("Empty file.");
		return;
	}

	nlohmann::json tmp;

	try
	{
		tmp = nlohmann::json::parse(sdata);
	}

	catch (...)
	{
		Log("Failed to Parse file : " + path);
	}

	if (path != appState->globals.currentOpenFilePath)
	{
		appState->globals.currentOpenFilePath = path;
	}

	Deserialize(tmp);
}

ApplicationState *Serializer::Deserialize(nlohmann::json d)
{
	return nullptr;
}

