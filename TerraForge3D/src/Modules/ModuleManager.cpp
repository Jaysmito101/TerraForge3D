#include "ModuleManager.h"
#include "Utils.h"
#include "zip.h"
#include "json.hpp"

ModuleManager::ModuleManager()
{
    if (!FileExists((GetExecutableDir() + "\\Data\\modules\\data.terr3d")))
    {
        MkDir((GetExecutableDir() + "\\Data\\modules"));
        SaveToFile((GetExecutableDir() + "\\Data\\modules\\data.terr3d"), "{\n            \"modules\" : [] ,\n                \"node_modules\" : [] ,\n                \"noise_layer_modules\" : [] ,\n                \"ui_helper_modules\" : []\n        }");
    }
    bool tmp = false;
    std::string modulesFolder = GetExecutableDir() + "\\modules\\";
    nlohmann::json mod_data = nlohmann::json::parse(ReadShaderSourceFile((GetExecutableDir() + "\\Data\\modules\\data.terr3d"), &tmp));
    for (std::string id : mod_data["ui_helper_modules"])
    {
        uiModules.push_back(new UIModule(id, this));
    }
    for (std::string id : mod_data["noise_layer_modules"])
    {
        nlModules.push_back(new NoiseLayerModule(id, this));
    }
    for (std::string id : mod_data["node_modules"])
    {
        noModules.push_back(new NodeModule(id, this));
    }
}

ModuleManager::~ModuleManager()
{
    for (Module* m : uiModules)
        delete m;
    uiModules.clear();
    for (std::pair<std::string, void*> mod : dllHandles)
        FreeLibrary((HMODULE)mod.second);
    dllHandles.clear();
}

void ModuleManager::InstallModule(std::string path)
{
    if (path.size() < 5)
        return;
    std::string id = MD5File(path).ToString();
    if (id.size() < 5)
        return;
    MkDir((GetExecutableDir() + "\\Data\\modules\\" + id));
    zip_extract(path.c_str(), (GetExecutableDir() + "\\Data\\modules\\" + id).c_str(), [](const char* filename, void* arg) { std::cout << "Extracting " << filename << "...\n"; return 1; }, (void*)0);
    bool tmp = false;
    nlohmann::json data = nlohmann::json::parse(ReadShaderSourceFile((GetExecutableDir() + "\\Data\\modules\\" + id + "\\data.terr3d"), &tmp));
    nlohmann::json mod_data = nlohmann::json::parse(ReadShaderSourceFile((GetExecutableDir() + "\\Data\\modules\\data.terr3d"), &tmp));
    mod_data["id"] = data;
    mod_data["modules"].push_back(id);
    if (data["node"])
    {
        mod_data["node_modules"].push_back(id);
    }
    if (data["noiselayer"])
    {
        mod_data["noise_layer_modules"].push_back(id);
    }
    if (data["uihelper"])
    {
        mod_data["ui_helper_modules"].push_back(id);
    }
    SaveToFile((GetExecutableDir() + "\\Data\\modules\\data.terr3d"), mod_data.dump(4));

    // TEMP
#ifdef  TERR3D_WIN32
    MessageBox(NULL, L"Module has been installed! Restart Application to load it!", L"Success", 0);
#endif //  TERR3D_WIN32
}

void ModuleManager::UinstallModule(std::string id)
{
    if (dllHandles.find(id) == dllHandles.end())
        return;
    dllHandles.erase(dllHandles.find(id));
    bool tmp = false;
    nlohmann::json mod_data = nlohmann::json::parse(ReadShaderSourceFile((GetExecutableDir() + "\\Data\\modules\\data.terr3d"), &tmp));
    nlohmann::json data = mod_data[id];
    mod_data.erase(id);
    mod_data["modules"].erase(mod_data["modules"].find(id));
    if (data["node"])
        mod_data["node_modules"].erase(mod_data["modules"].find(id));
    if (data["noiselayer"])
        mod_data["noise_layer_modules"].erase(mod_data["modules"].find(id));
    if (data["uihelper"])
        mod_data["ui_helper_modules"].erase(mod_data["modules"].find(id));
    SaveToFile((GetExecutableDir() + "\\Data\\modules\\data.terr3d"), mod_data.dump(4));
}

void* ModuleManager::GetDLLHandle(std::string id)
{
    void* result = nullptr;
    if (dllHandles.find(id) != dllHandles.end())
    {
        return dllHandles[id];
    }

    std::string modulePath = GetExecutableDir() + "\\Data\\modules\\" + id + "\\module.dll";
    std::wstring wP = s2ws(modulePath);
    result = LoadLibrary(wP.c_str());
    if (result)
        dllHandles[id] = result;
    return result;
}

void* ModuleManager::GetFunctionPointer(std::string id, std::string functionName)
{
    void* result = nullptr;
    if (dllHandles.find(id) == dllHandles.end())
    {
        return result;
    }
    result = GetProcAddress((HMODULE)dllHandles[id], functionName.c_str());
    return result;
}

NodeModule* ModuleManager::FindNodeModule(std::string id)
{
    for (NodeModule* mod : noModules)
        if (mod->id == id)
            return mod;
    return nullptr;
}
