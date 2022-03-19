#pragma once

#include <string>
#include <vector>

#include "Base/Texture2D.h"

#include "json/json.hpp"

class ApplicationState;

struct TextureStoreItem
{
	std::string name = "";
	int download_count = 0;

	std::string thumbnailPath = "";
	Texture2D *texThumbnail = nullptr;

	std::string baseDir = "";

	std::string abledo = "";
	std::string normal = "";
	std::string roughness = "";
	std::string metallic = "";
	std::string ao = "";
	std::string arm = "";

	bool downloaded = false;

	std::vector<std::string> authours = {};
};

class TextureStore
{
public:
	TextureStore(ApplicationState *appState);
	~TextureStore();

	void ShowSettings(bool *pOpen);

	void DownloadTexture(int id, int res);
	void DeleteTexture(int id);

	void SaveDownloadsDatabase();

	inline static TextureStore *Get() { return sInstance; }

private:
	void LoadTextureThumbs();
	void VerifyTextureThumbs();
	void LoadTextureDatabase();
	nlohmann::json LoadDownloadedTextureDatabaseJ();
	nlohmann::json LoadTextureDatabaseJ();

	void ShowAllTexturesSettings();
	void ShowDownloadedTexturesSettings();


private:
	static TextureStore* sInstance;
	nlohmann::json textureDatabaseJ;
	nlohmann::json downloadedTextureDatabaseJ;
	std::string uid;

	char searchStr[4096];


public:
	std::vector<TextureStoreItem> textureStoreItems;
	std::vector<int> downloadedTextureStoreItems;
	ApplicationState *appState;
};