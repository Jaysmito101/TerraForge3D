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
	data = nlohmann::json();
	data["type"] = "SAVEFILE";
	data["serailizerVersion"] = TERR3D_SERIALIZER_VERSION;
	data["maxSerailizerVersion"] = TERR3D_MAX_SERIALIZER_VERSION;
	data["minSerailizerVersion"] = TERR3D_MIN_SERIALIZER_VERSION;
	data["versionHash"] = MD5File(GetExecutablePath()).ToString();
	data["name"] = "TerraForge3D v2.2.0";
	data["mode"] = appState->mode;
	// SAVE NODE GENERATORS DATA
	TRY_CATCH_ERR_SERIALIZE(data["appData"] = appState->globals.appData;, "Failed to save App Data");
	TRY_CATCH_ERR_SERIALIZE(data["appStyles"] = GetStyleData();, "Failed to save app styles");
	TRY_CATCH_ERR_SERIALIZE(data["imguiData"] = std::string(ImGui::SaveIniSettingsToMemory());, "Failed to save ImGui Data.");
	TRY_CATCH_ERR_SERIALIZE(data["camera"] = appState->cameras.Save();, "Failed to save cameras.");
	TRY_CATCH_ERR_SERIALIZE(data["windows"] = appState->windows.Save();, "Failed to save window states");
	TRY_CATCH_ERR_SERIALIZE(data["states"] = appState->states.Save();, "Failed to save states.");
	TRY_CATCH_ERR_SERIALIZE(data["globals"] = appState->globals.Save();, "Failed to save globals.");
	TRY_CATCH_ERR_SERIALIZE(data["sea"] = appState->seaManager->Save();, "Failed to save Sea.");
	TRY_CATCH_ERR_SERIALIZE(data["foliage"] = appState->foliageManager->Save();, "Failed to save foliage manager.");
	TRY_CATCH_ERR_SERIALIZE(data["projectID"] = appState->projectManager->GetId();, "Failed to save project id.");
	TRY_CATCH_ERR_SERIALIZE(data["textureBaker"] = appState->textureBaker->Save();, "Failed to save texture baker settings.");
	TRY_CATCH_ERR_SERIALIZE(appState->projectManager->SaveDatabase(); data["projectDatabase"] = appState->projectManager->GetDatabase();, "Failed to save project database.");
	TRY_CATCH_ERR_SERIALIZE(data["lighting"] = appState->lightManager->Save();, "Failed to save lighting data.");
	TRY_CATCH_ERR_SERIALIZE(data["shading"] = appState->shadingManager->Save();, "Failed to save shading data.");
	TRY_CATCH_ERR_SERIALIZE(data["generators"] = appState->meshGenerator->Save();, "Failed to save generators.");
	TRY_CATCH_ERR_SERIALIZE(

	    if (!appState->mode == ApplicationMode::CUSTOM_BASE)
{
	std::string baseHash = MD5File(appState->globals.currentBaseModelPath)
		                       .ToString();
		std::string projectAsset = appState->projectManager->GetAsset(baseHash);

		if (projectAsset == "")
		{
			MkDir(appState->projectManager->GetResourcePath() + PATH_SEPARATOR "models");
			CopyFileData(appState->globals.currentBaseModelPath, appState->projectManager->GetResourcePath() + PATH_SEPARATOR "models" PATH_SEPARATOR + baseHash + appState->globals.currentBaseModelPath.substr(appState->globals.currentBaseModelPath.find_last_of(".")));
			appState->projectManager->RegisterAsset(baseHash, "models" PATH_SEPARATOR + baseHash + appState->globals.currentBaseModelPath.substr(appState->globals.currentBaseModelPath.find_last_of(".")));
		}

		data["baseid"] = baseHash;
	}
	, "Failed to save custom base model."
	);
	return data;
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
	data = d;
	std::cout << "Wating for remesh cycle to finish ...\n";

	while (appState->states.remeshing);

	std::cout << "Finished remesing." << std::endl;
	TRY_CATCH_ERR_DESERIALIZE(

	    if (data["versionHash"] != MD5File(GetExecutablePath()).ToString())
{
	onError("The file you are tryng to open was made with a different version of TerraForge3D!\nTrying to check Serializer compatibility", false);
	}
	, "Failed to verify file version."
	);
	TRY_CATCH_ERR_DESERIALIZE_WITH_CODE(
	    int serializerVersion = data["serailizerVersion"];

	    if (serializerVersion < TERR3D_MIN_SERIALIZER_VERSION)
{
	onError("This file cannot be opened as it was serialized using serializer V" + std::to_string(serializerVersion) + " but the minimum serializer version required is V" + std::to_string(TERR3D_MIN_SERIALIZER_VERSION), false);
		return nullptr;
	}

	if (serializerVersion > TERR3D_MAX_SERIALIZER_VERSION)
{
	onError("This file cannot be opened as it was serialized using serializer V" + std::to_string(serializerVersion) + " but the maximum serializer version supported is V" + std::to_string(TERR3D_MAX_SERIALIZER_VERSION), false);
		return nullptr;
	}
	, "Failed to verify Serializer",
	return nullptr;
	);

	if (data["type"] != "SAVEFILE")
	{
		onError("This file is not a savefile.", false);
		return nullptr;
	}

	TRY_CATCH_ERR_DESERIALIZE(LoadThemeFromStr(data["styleData"]);, "Failed to load theme.");
	TRY_CATCH_ERR_DESERIALIZE(appState->globals.appData = data["appData"];, "Failed to app data.");
	TRY_CATCH_ERR_DESERIALIZE(appState->globals.appData = data["appData"];, "Failed to app data.");
	TRY_CATCH_ERR_DESERIALIZE(appState->mode = data["mode"];, "Failed to app mode.");
	TRY_CATCH_ERR_DESERIALIZE(appState->projectManager->SetId(data["projectID"]);, "Failed to load Project ID.");
	TRY_CATCH_ERR_DESERIALIZE(appState->projectManager->SetDatabase(data["projectDatabase"]);, "Failed to load project database.");
	std::cout << "Loaded Project ID : " << appState->projectManager->GetId() << std::endl;
	TRY_CATCH_ERR_DESERIALIZE(appState->cameras.Load(data["camera"]);, "Failed to load camera.");
	TRY_CATCH_ERR_DESERIALIZE(appState->windows.Load(data["windows"]);, "Failed to load windows.");
	TRY_CATCH_ERR_DESERIALIZE(appState->states.Load(data["states"]);, "Failed to load states.");
	TRY_CATCH_ERR_DESERIALIZE(appState->globals.Load(data["globals"]);, "Failed to load globals.");
	TRY_CATCH_ERR_DESERIALIZE(appState->textureBaker->Load(data["textureBaker"]);, "Failed to load texture baker settings.");
	TRY_CATCH_ERR_DESERIALIZE(appState->seaManager->Load(data["sea"]);, "Failed to load sea.");
	TRY_CATCH_ERR_DESERIALIZE(appState->foliageManager->Load(data["foliage"]);, "Failed to load foliage.");
	TRY_CATCH_ERR_DESERIALIZE(appState->lightManager->Load(data["lighting"]);, "Failed to load lighting.");
	TRY_CATCH_ERR_DESERIALIZE(appState->meshGenerator->Load(data["generators"]);, "Failed to load generators.");
	TRY_CATCH_ERR_DESERIALIZE(appState->shadingManager->Load(data["shading"]);, "Failed to load shading data.");
	TRY_CATCH_ERR_DESERIALIZE_WITH_CODE(

	    if (appState->mode == ApplicationMode::CUSTOM_BASE)
{
	std::string baseHash = data["baseid"];
		std::string projectAsset = appState->projectManager->GetAsset(baseHash);

		if (projectAsset == "")
		{
			std::cout << "Failed to load base model from save file!\n";
			appState->states.usingBase = true;
		}

		else
		{
			if (!appState->models.customBase)
			{
				delete appState->models.customBase;
				delete appState->models.customBaseCopy;
			}

			appState->globals.currentBaseModelPath = appState->projectManager->GetResourcePath() + PATH_SEPARATOR + projectAsset;
			appState->models.customBase = LoadModel(appState->globals.currentBaseModelPath);
			appState->models.customBaseCopy = LoadModel(appState->globals.currentBaseModelPath);
			appState->states.usingBase = false;
		}
	}
	, "Failed to load custom base model. Falling back to plane.",

	if (!appState->models.customBase)
{
	delete appState->models.customBase;
	delete appState->models.customBaseCopy;
}
appState->states.usingBase = true;
);
	return appState;
}

