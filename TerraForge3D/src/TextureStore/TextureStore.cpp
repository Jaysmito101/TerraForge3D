#include "TextureStore/TextureStore.h"
#include "Platform.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <filesystem>

#define TEXTURE_STORE_ITEM_DND(text, var) \
				if(item.var.size() > 3) \
				{ \
					ImGui::Selectable(text, &tmp); \
					if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) \
					{	\
						ImGui::SetDragDropPayload("TerraForge3D_Texture", item.var.data(), sizeof(char) * item.var.size()); \
						ImGui::Image((ImTextureID)(uint64_t)item.texThumbnail->GetRendererID(), ImVec2(128, 128)); \
						ImGui::Text("%s", item.name.c_str()); \
						ImGui::EndDragDropSource(); \
					} \
				}

#ifdef TERR3D_WIN32
inline char *strcasestr(const char *str, const char *pattern)
{
	size_t i;

	if (!*pattern)
	{
		return (char *)str;
	}

	for (; *str; str++)
	{
		if (toupper((unsigned char)*str) == toupper((unsigned char)*pattern))
		{
			for (i = 1;; i++)
			{
				if (!pattern[i])
				{
					return (char *)str;
				}

				if (toupper((unsigned char)str[i]) != toupper((unsigned char)pattern[i]))
				{
					break;
				}
			}
		}
	}

	return NULL;
}
#endif

TextureStore* TextureStore::sInstance = nullptr;

nlohmann::json TextureStore::LoadTextureDatabaseJ()
{
	bool loadFromFile = true;

	if(IsNetWorkConnected() && rand() % 2 == 0)
	{
		loadFromFile = false;
	}

	if(!FileExists(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "texture_database.terr3d"))
	{
		loadFromFile = false;
	}

	if(loadFromFile)
	{
		Log("Loading texture database from file");
		bool tmp = false;
		std::string tmpStr = ReadShaderSourceFile(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "texture_database.terr3d", &tmp);

		if(tmp == false)
		{
			Log("Failed to load texture database from file");
			return nlohmann::json();
		}

		try
		{
			return nlohmann::json::parse(tmpStr);
		}

		catch(...)
		{
			Log("Failed to parse texture database from file");
			return nlohmann::json();
		}
	}

	else
	{
		Log("Fetching texture database from web database.");
		std::string tmp = FetchURL("https://api.polyhaven.com", "/assets?t=textures");

		if (tmp.size() < 3)
		{
			Log("Failed to fetch texture database from web database.");
			return nlohmann::json();
		}

		SaveToFile(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "texture_database.terr3d", tmp);
		return nlohmann::json::parse(tmp);
	}
}

void TextureStore::VerifyTextureThumbs()
{
	MkDir(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "texture_thumbnails" PATH_SEPARATOR);

	for(auto it = textureDatabaseJ.begin() ; it != textureDatabaseJ.end() ; it++)
	{
		if(!FileExists(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "texture_thumbnails" PATH_SEPARATOR + it.key() + ".png"))
		{
			Log("Downloading thumbnail for texture: " + it.key());
			DownloadFile("https://cdn.polyhaven.com", "/asset_img/thumbs/" +
					it.key() + ".png?width=100&height=100", GetExecutableDir()
					+ PATH_SEPARATOR "Data" PATH_SEPARATOR "cache"
					PATH_SEPARATOR "texture_thumbnails" PATH_SEPARATOR +
					it.key() + ".png");
		}
	}
}

nlohmann::json TextureStore::LoadDownloadedTextureDatabaseJ()
{
	if(!FileExists(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "configs" PATH_SEPARATOR "texture_database_downloaded.terr3d"))
	{
		Log("No Textures downloaded yet");
		return nlohmann::json();
	}

	bool tmp = false;
	std::string tmpStr = ReadShaderSourceFile(GetExecutableDir() +
			PATH_SEPARATOR "Data" PATH_SEPARATOR "configs" PATH_SEPARATOR
			"texture_database_downloaded.terr3d", &tmp);

	if(tmp == false)
	{
		Log("Failed to load downloaded texture database from file");
		return nlohmann::json();
	}

	nlohmann::json tmpJ;

	try
	{
		tmpJ = nlohmann::json::parse(tmpStr);
	}

	catch(...)
	{
		Log("Failed to parse downloaded texture database from file");
		return nlohmann::json();
	}

	return tmpJ;
}


void TextureStore::LoadTextureDatabase()
{
	textureDatabaseJ = LoadTextureDatabaseJ();
	downloadedTextureDatabaseJ = LoadDownloadedTextureDatabaseJ();
	textureStoreItems.clear();
	downloadedTextureStoreItems.clear();

	for(auto it = textureDatabaseJ.begin() ; it != textureDatabaseJ.end() ; it++)
	{
		TextureStoreItem item;
		item.name = it.key();
		item.thumbnailPath = GetExecutableDir() + PATH_SEPARATOR "Data"
			PATH_SEPARATOR "cache" PATH_SEPARATOR "texture_thumbnails"
			PATH_SEPARATOR + item.name + ".png";
		item.download_count = it.value()["download_count"];

		for(auto it2 = it.value()["authors"].begin() ; it2 != it.value()["authors"].end() ; it2++)
		{
			item.authours.push_back(it2.key());
		}

		if(downloadedTextureDatabaseJ.find(item.name) != downloadedTextureDatabaseJ.end())
		{
			item.downloaded = true;
			item.abledo = downloadedTextureDatabaseJ[item.name]["abledo"];
			item.normal = downloadedTextureDatabaseJ[item.name]["normal"];
			item.roughness = downloadedTextureDatabaseJ[item.name]["roughness"];
			item.metallic = downloadedTextureDatabaseJ[item.name]["metallic"];
			item.ao = downloadedTextureDatabaseJ[item.name]["ao"];
			item.arm = downloadedTextureDatabaseJ[item.name]["arm"];
			item.baseDir = downloadedTextureDatabaseJ[item.name]["baseDir"];
			downloadedTextureStoreItems.push_back((int)textureStoreItems.size());
		}

		textureStoreItems.push_back(item);
	}
}

void TextureStore::LoadTextureThumbs()
{
	Log("Loading texture thumbnails");
	int i=0;

	for(auto &it : textureStoreItems)
	{
		i++;

		if(!FileExists(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "texture_thumbnails" PATH_SEPARATOR + it.name + ".png"))
		{
			Log("Thumbnail for texture: " + it.name + " not found.");
			it.texThumbnail = new Texture2D(GetExecutableDir() + PATH_SEPARATOR
					"Data" PATH_SEPARATOR "textures" PATH_SEPARATOR
					"white.png", false);
		}

		else
		{
			it.texThumbnail = new Texture2D(it.thumbnailPath, false);
		}

		if(i % 20 == 0)
		{
			std::cout << ("Loaded " + std::to_string(i) + " of " + std::to_string(textureStoreItems.size()) + " texture thumbnails\r");
		}
	}

	Log("Texture thumbnails loaded");
}

void TextureStore::SaveDownloadsDatabase()
{
	nlohmann::json tmp2;

	for(int id : downloadedTextureStoreItems)
	{
		nlohmann::json tmp;
		tmp["abledo"]	= textureStoreItems[id].abledo;
		tmp["normal"]	= textureStoreItems[id].normal;
		tmp["roughness"]= textureStoreItems[id].roughness;
		tmp["metallic"]	= textureStoreItems[id].metallic;
		tmp["ao"]	= textureStoreItems[id].ao;
		tmp["arm"]	= textureStoreItems[id].arm;
		tmp["baseDir"]	= textureStoreItems[id].baseDir;
		tmp2[textureStoreItems[id].name] = tmp;
	}

	SaveToFile(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR
			"configs" PATH_SEPARATOR "texture_database_downloaded.terr3d",
			tmp2.dump(4));
}

void TextureStore::DeleteTexture(int id)
{
	TextureStoreItem item = textureStoreItems[id];
	DeleteFileT(textureStoreItems[id].abledo);
	DeleteFileT(textureStoreItems[id].ao);
	DeleteFileT(textureStoreItems[id].normal);
	DeleteFileT(textureStoreItems[id].metallic);
	DeleteFileT(textureStoreItems[id].roughness);
	DeleteFileT(textureStoreItems[id].arm);
	DeleteFileT(textureStoreItems[id].baseDir + PATH_SEPARATOR "displacement.png"); // TEMPORARY
	textureStoreItems[id].downloaded = false;
	downloadedTextureStoreItems.erase(std::find(downloadedTextureStoreItems.begin(), downloadedTextureStoreItems.end(), id));
	SaveDownloadsDatabase();
}

void TextureStore::DownloadTexture(int id, int res)
{
	std::string tmpStr = FetchURL("https://api.polyhaven.com", "/files/" + textureStoreItems[id].name);
	nlohmann::json tmpJ;

	try
	{
		tmpJ = nlohmann::json::parse(tmpStr);
	}

	catch(...)
	{
		Log("Failed to download texture : "  + textureStoreItems[id].name);
		return;
	}

	std::string baseDir = GetExecutableDir() + PATH_SEPARATOR "Data"
		PATH_SEPARATOR "textures" PATH_SEPARATOR + textureStoreItems[id].name;
	MkDir(baseDir);
	textureStoreItems[id].baseDir = baseDir;
	tmpStr = tmpJ["Diffuse"][std::to_string(res) + "k"]["png"]["url"];
	DownloadFile("https://dl.polyhaven.org", tmpStr.substr(24), baseDir + PATH_SEPARATOR "albedo.png");
	textureStoreItems[id].abledo = baseDir + PATH_SEPARATOR "albedo.png";
	tmpStr = tmpJ["nor_gl"][std::to_string(res) + "k"]["png"]["url"];
	DownloadFile("https://dl.polyhaven.org", tmpStr.substr(24), baseDir + PATH_SEPARATOR "normal.png");
	textureStoreItems[id].normal = baseDir + PATH_SEPARATOR "normal.png";
	tmpStr = tmpJ["Rough"][std::to_string(res) + "k"]["png"]["url"];
	DownloadFile("https://dl.polyhaven.org", tmpStr.substr(24), baseDir + PATH_SEPARATOR "roughness.png");
	textureStoreItems[id].roughness = baseDir + PATH_SEPARATOR "roughness.png";
	tmpStr = tmpJ["AO"][std::to_string(res) + "k"]["png"]["url"];
	DownloadFile("https://dl.polyhaven.org", tmpStr.substr(24), baseDir + PATH_SEPARATOR "ao.png");
	textureStoreItems[id].ao = baseDir + PATH_SEPARATOR "ao.png";
	tmpStr = tmpJ["arm"][std::to_string(res) + "k"]["png"]["url"];
	DownloadFile("https://dl.polyhaven.org", tmpStr.substr(24), baseDir + PATH_SEPARATOR "arm.png");
	textureStoreItems[id].arm = baseDir + PATH_SEPARATOR "arm.png";
	tmpStr = tmpJ["Displacement"][std::to_string(res) + "k"]["png"]["url"];
	DownloadFile("https://dl.polyhaven.org", tmpStr.substr(24), baseDir + PATH_SEPARATOR "displacement.png");
	textureStoreItems[id].downloaded = true;
	downloadedTextureStoreItems.push_back(id);
	SaveDownloadsDatabase();
}

void TextureStore::ShowAllTexturesSettings()
{
	int searchLength = (int)strlen(searchStr);
	ImGui::Columns(4, NULL);
	float width = ImGui::GetContentRegionAvail().x;

	if(width <= 5)
	{
		width = 50;
	}

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.3f, 0.3f, 0.3f, 0.7f));

	for(int i=0; i<textureStoreItems.size(); i++)
	{
		bool tmp = false;
		TextureStoreItem &item = textureStoreItems[i];

		if(searchLength == 0 || strcasestr(item.name.c_str(), searchStr) != NULL)
		{
			tmp = item.downloaded;


			if(item.downloaded)
			{
				ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.5f, 0.5f, 0.5f, 0.7f));
			}

			ImGui::PushID(item.name.data());
			ImGui::BeginChild("##texture_thumb", ImVec2(width, 300), true);
			ImGui::Image((ImTextureID)(uint64_t)item.texThumbnail->GetRendererID(), ImVec2(width, 150));
			ImGui::Text(item.name.data());

			if(!item.downloaded)
			{
				if(ImGui::Button("Download 1K"))
				{
					DownloadTexture(i, 1);
					tmp = false;
				}

				if(ImGui::Button("Download 2K"))
				{
					DownloadTexture(i, 2);
					tmp = false;
				}

				if(ImGui::Button("Download 4K"))
				{
					DownloadTexture(i, 4);
					tmp = false;
				}
			}

			else
			{
				static bool tmp = false;


				TEXTURE_STORE_ITEM_DND("Albedo", abledo)
				TEXTURE_STORE_ITEM_DND("Normal", normal)
				TEXTURE_STORE_ITEM_DND("Metallic", metallic)
				TEXTURE_STORE_ITEM_DND("Roughness", roughness)
				TEXTURE_STORE_ITEM_DND("AO", ao)
				TEXTURE_STORE_ITEM_DND("ARM", arm)

				
				if(ImGui::Button("Delete"))
				{
					DeleteTexture(i);
				}
			}

			//ImGui::Text("Downloads : %d", item.download_count);
			//ImGui::Text("Authors :");
			//for(auto author : item.authours)
			//{
			//    ImGui::BulletText(author.c_str());
			//}
			ImGui::EndChild();
			ImGui::PopID();

			if(tmp)
			{
				ImGui::PopStyleColor();
			}

			ImGui::NextColumn();
		}
	}

	ImGui::PopStyleColor();
	ImGui::Columns(1);
}

void TextureStore::ShowDownloadedTexturesSettings()
{
	ImGui::Columns(4, NULL);
	float width = ImGui::GetContentRegionAvail().x;

	if(width <= 5)
	{
		width = 50;
	}

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.5f, 0.5f, 0.5f, 0.7f));

	for(int i=0; i<downloadedTextureStoreItems.size(); i++)
	{
		TextureStoreItem &item = textureStoreItems[downloadedTextureStoreItems[i]];
		ImGui::PushID(i);
		ImGui::BeginChild("##texture_thumb", ImVec2(width, 300), true);
		ImGui::Image((ImTextureID)(uint64_t)item.texThumbnail->GetRendererID(), ImVec2(width, 120));
		ImGui::Text(item.name.c_str());

		if(ImGui::Button("Delete##DTS"))
		{
			DeleteTexture(downloadedTextureStoreItems[i]);
		}

		//ImGui::Text("Downloads : %d", item.download_count);
		//ImGui::Text("Authors :");
		//for(auto author : item.authours)
		//{
		//    ImGui::BulletText(author.c_str());
		//}
		ImGui::EndChild();
		ImGui::PopID();
		ImGui::NextColumn();
	}

	ImGui::PopStyleColor();
	ImGui::Columns(1);
}

void TextureStore::ShowSettings(bool *pOpen)
{
	ImGui::Begin("Textute Store Settings", pOpen);
	ImGui::InputTextWithHint("##TextureStorePolyHavenSearch", "Search ...", searchStr, 4096);
	ImGui::Separator();

	if (ImGui::BeginTabBar("##textureStoreTabBar"))
	{
		if (ImGui::BeginTabItem("All Textures"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
			ShowAllTexturesSettings();
			ImGui::PopStyleVar();
			ImGui::PopStyleVar();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Downloaded Textures"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
			ShowDownloadedTexturesSettings();
			ImGui::PopStyleVar();
			ImGui::PopStyleVar();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("About"))
		{
			ImGui::Text("This texture store provides collection of free PBR textures.\nThis is powered by Polyhaven.");
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

TextureStore::TextureStore(ApplicationState *as)
{
	sInstance = this;
	uid = GenerateId(32);
	memset(searchStr, 0, sizeof(searchStr) / sizeof(searchStr[0]));
	appState = as;
	LoadTextureDatabase();
	std::thread t([&]()
	{
		VerifyTextureThumbs();
	});
	t.detach();
	LoadTextureThumbs();
}

TextureStore::~TextureStore()
{
	for(auto &it : textureStoreItems)
	{
		delete it.texThumbnail;
	}
}
