#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING


#include <Utils.h>
#include <TextureStore.h>
#include <Texture2D.h>

#include <thread>
#include <vector>
#include <atomic>
#include <filesystem>
#include <experimental/filesystem>

#include <imgui.h>
#include <json.hpp>

static char* strcasestr(const char* str, const char* pattern) {
	size_t i;

	if (!*pattern)
		return (char*)str;

	for (; *str; str++) {
		if (toupper((unsigned char)*str) == toupper((unsigned char)*pattern)) {
			for (i = 1;; i++) {
				if (!pattern[i])
					return (char*)str;
				if (toupper((unsigned char)str[i]) != toupper((unsigned char)pattern[i]))
					break;
			}
		}
	}
	return NULL;
}



// This can be increased ddepending upon thr system more the threads
// faster will it get downloaded slower will performance be during download
#define NUM_DOWNLOADER_THREADS 3

nlohmann::json texture_database;
nlohmann::json texture_files_database;
nlohmann::json download_database;
std::string execDir;
std::vector<Texture2D*> textureThumbnails;
std::atomic<bool> isLoadingThumbs;
std::string currTex = "";
std::string currTexID = "";
bool* reqRefresh;
Texture2D* phlogo;
bool intProob = false;
bool isConnected = true;
int counterT = 0;

void LoadThumbnails() {
	isLoadingThumbs = true;
	Log("Starting to load thumbnails ...");
	for (Texture2D* tex : textureThumbnails)
		if (tex)
			delete tex;
	textureThumbnails.clear();
	isLoadingThumbs = false;
	system((std::string("mkdir \"") + execDir + "\\Data\\cache\\texture_thumbnails\"").c_str());
	int co = 0;
	for (auto it = texture_database.begin(); it != texture_database.end(); ++it)
	{
		textureThumbnails.push_back(new Texture2D(execDir + "\\Data\\cache\\texture_thumbnails\\" + it.key() + ".png", false, true));
		co++;
		std::cout << "Loaded " << co << " thumbnails.          \r";
	}
	Log("\nLoaded thumbnails");

}

void ChacheThumbnails() {
	nlohmann::json& data = texture_database;
	Log("Chaching Thumbnails...");
	int co = 0;
	int uco = 0;
	std::vector<std::string> batch;
	int batchsize = data.size() / NUM_DOWNLOADER_THREADS;

	for (auto it = data.begin(); it != data.end();)
	{
		// Preparing Batch
		batch.clear();
		for (int i = 0; i < batchsize && it != data.end();) {
			if (!FileExists(execDir + "\\Data\\cache\\texture_thumbnails\\" + it.key() + ".png")) {
				batch.push_back(it.key());
				i++;
				co++;
			}
			else {
				uco++;
			}
			it++;
		}
		// Deploy Batch
		std::thread t = std::thread([batch] {
			for (std::string asset_id : batch) {
				DownloadFile("https://cdn.polyhaven.com", "/asset_img/thumbs/" + asset_id + ".png?width=300&height=300", execDir + "\\Data\\cache\\texture_thumbnails\\" + asset_id + ".png");
				Log("Downloaded " + execDir + "\\Data\\cache\\texture_thumbnails\\" + asset_id + ".png");
			}
			});
		t.detach();
	}
	Log("Found " + std::to_string(uco) + " Chached thumbnails.");
	Log("Chached " + std::to_string(co) + " thumbnails.");
}

void ChacheFilesData() {
	if (FileExists(execDir + "\\Data\\cache\\texfiles.terr3d")) {
		bool tmp = false;
		nlohmann::json tp = nlohmann::json::parse(ReadShaderSourceFile(execDir + "\\Data\\cache\\texfiles.terr3d", &tmp));
		if (tp.size() == texture_database.size()) {
			Log("Found cached files data for " + std::to_string(tp.size()) + " items.");
			texture_files_database = tp;
			return;
		}
	}
	nlohmann::json data;
	int count = 0;
	Log("Caching files data ...");
	for (auto it = texture_database.begin(); it != texture_database.end(); ++it)
	{
		std::string tmp = FetchURL("https://api.polyhaven.com", "/files/" + it.key());
		try {
			data[it.key()] = nlohmann::json::parse(tmp);
			count++;
			std::cout << "Cached files data for " << it.key() << ". Total : " << count << " items.                                \r";
		}
		catch (...) {
			std::cout << "Error in caching data for " << it.key() << "               \r";
		}
	}
	texture_files_database = data;
	SaveToFile(execDir + "\\Data\\cache\\texfiles.terr3d", data.dump());
	Log("\nCached files data.");
}

void SetupTextureStore(std::string executablePath, bool* reqrfrsh) {
	//isConnected = IsNetWorkConnected();
	reqRefresh = reqrfrsh;
	execDir = executablePath;
	phlogo = new Texture2D(execDir + "\\Data\\logos\\polyhavenlogo.png");
	Log("Setting up Texture Store ...");
	bool tmpb = false;
	download_database["downloaded"] = std::vector<std::string>();
	try {
		download_database = nlohmann::json::parse(ReadShaderSourceFile(execDir + "\\Data\\configs\\downloaded_textures.terr3d", &tmpb));
	}
	catch (...) {}
	if (IsNetWorkConnected()) {
		Log("Internet Connection is Live.\nFetching Latest data from servers.");
		intProob = false;
		std::string tmp = FetchURL("https://api.polyhaven.com", "/assets?t=textures");
		SaveToFile(executablePath + "\\Data\\cache\\texture_database.terr3d", tmp);
		texture_database = nlohmann::json::parse(tmp);

		for (auto it = texture_database.begin(); it != texture_database.end(); ++it)
		{
			auto tm = std::find(download_database["downloaded"].begin(), download_database["downloaded"].end(), it.key());
			if (tm != download_database["downloaded"].end()) {
				it.value()["downloaded"] = true;
				it.value()["downloaddata"] = download_database[std::string(tm.value())];
			}
			else {
				it.value()["downloaded"] = false;
			}
		}
		std::thread t1 = std::thread(ChacheFilesData);
		t1.detach();

		std::thread t2 = std::thread(ChacheThumbnails);
		t2.detach();
	}
	else {
		Log("Internet Connection not reachable!");
		intProob = true;
		if (FileExists(executablePath + "\\Data\\cache\\texture_database.terr3d", true) && FileExists(execDir + "\\Data\\cache\\texfiles.terr3d")) {
			Log("Found Cached Data!\nLoading Texture Database From Cache!");
			bool tmp = false;
			texture_database = nlohmann::json::parse(ReadShaderSourceFile(executablePath + "\\Data\\cache\\texture_database.terr3d", &tmp));
			nlohmann::json tp = nlohmann::json::parse(ReadShaderSourceFile(execDir + "\\Data\\cache\\texfiles.terr3d", &tmp));
			Log("Found cached files data for " + std::to_string(tp.size()) + " items.");
		}
		else {
			Log("Could not Find Cached Data!\nTexture Store will not work till Internet Connection is Live!");
		}
	}

}

void UpdateTextureStore() {
	counterT++;
	if (counterT > 5) {
		counterT = 0;
		isConnected = IsNetWorkConnected();
	}
}

static void DownloadTexture(std::string id, int k) {
	if (!IsNetWorkConnected()) {
		Log("Cannot download texture wthout an internet Connection!");
		return;
		return;
	}
	nlohmann::json data = texture_files_database[id];
	int size = data["Diffuse"][std::to_string(k) + "k"]["png"]["size"];
	std::string url = data["Diffuse"][std::to_string(k) + "k"]["png"]["url"];
	system((std::string("mkdir \"") + (execDir)+"\\Data\\textures\\" + id + "\\" + std::to_string(k) +"\"").c_str());
	DownloadFile("https://dl.polyhaven.org", url.substr(24),  ((execDir) + "\\Data\\textures\\" + id + "\\" + std::to_string(k) + "\\diffuse.png"), size);
	download_database["downloaded"].push_back(id);
	nlohmann::json tmp;
	tmp["dpath"] = ((execDir)+"\\Data\\textures\\" + id + "\\" + std::to_string(k) + "\\diffuse.png");
	tmp["durl"] = url;
	tmp["k"] = std::to_string(k) + "K";
	tmp["ko"] = k;
	download_database[id] = tmp;
	SaveToFile(execDir + "\\Data\\configs\\downloaded_textures.terr3d", download_database.dump());
	for (auto it = texture_database.begin(); it != texture_database.end(); ++it)
	{
		auto tm = std::find(download_database["downloaded"].begin(), download_database["downloaded"].end(), it.key());
		if (tm != download_database["downloaded"].end()) {
			it.value()["downloaded"] = true;
			it.value()["downloaddata"] = download_database[std::string(tm.value())];
		}
		else {
			it.value()["downloaded"] = false;
		}
	}
}

static void DeleteTexture(std::string id, int k) {
	try {
		if (std::experimental::filesystem::remove(((execDir)+"\\Data\\textures\\" + id + "\\" + std::to_string(k) + "\\diffuse.png")))
			std::cout << "File " << ((execDir)+"\\Data\\textures\\" + id + "\\" + std::to_string(k) + "\\diffuse.png") << " deleted.\n";
		else
			std::cout << "File " << ((execDir)+"\\Data\\textures\\" + id + "\\" + std::to_string(k) + "\\diffuse.png") << " not found.\n";
	}
	catch (const std::experimental::filesystem::filesystem_error& err) {
		std::cout << "filesystem error: " << err.what() << '\n';
	}
	download_database["downloaded"].erase(std::find(download_database["downloaded"].begin(), download_database["downloaded"].end(), id));
	for (auto it = texture_database.begin(); it != texture_database.end(); ++it)
	{
		auto tm = std::find(download_database["downloaded"].begin(), download_database["downloaded"].end(), it.key());
		if (tm != download_database["downloaded"].end()) {
			it.value()["downloaded"] = true;
			it.value()["downloaddata"] = download_database[std::string(tm.value())];
		}
		else {
			it.value()["downloaded"] = false;
		}
	}
}

static void ShowPopUpSettings(nlohmann::json data) {
	if (ImGui::BeginPopupContextWindow())
	{
		ImGui::Text("Name :");
		ImGui::SameLine();
		ImGui::Text(std::string(data["name"]).c_str());
		ImGui::Text("Author :");
		for (auto it = data["authors"].begin(); it != data["authors"].end(); ++it)
		{
			ImGui::Text(std::string(it.key()).c_str());
		}

		if (data["downloaded"] == false) {
			if (ImGui::Button("Download 1K")) {
				DownloadTexture(data["key"], 1);
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Button("Download 2K")) {
				DownloadTexture(data["key"], 2);
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Button("Download 4K")) {
				DownloadTexture(data["key"], 4);
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Button("Download 8K")) {
				DownloadTexture(data["key"], 8);
				ImGui::CloseCurrentPopup();
			}
		}
		else {
			ImGui::Text("Resolution : ");
			ImGui::SameLine();
			ImGui::Text(std::string(data["downloaddata"]["k"]).c_str());
			if (ImGui::Button("Use")) {
				currTexID = data["key"];
				currTex = data["downloaddata"]["dpath"];
				*reqRefresh = true;
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Button("Delete")) {
				DeleteTexture(data["key"], data["downloaddata"]["ko"]);
			}
		}

		ImGui::EndPopup();
	}
}

static int numTextoShow = 50;
static bool tmpbo = false;
static void ShowTextureThumbs(char* searchStr, int sstrl) {
	int co = 0;
	std::string texThumbChildID = "TexThumbChildID##1";
	if (textureThumbnails.size() != texture_database.size() || isLoadingThumbs)
		return;
	ImGui::DragInt("Number of Texture", &numTextoShow, 1, 0, textureThumbnails.size());
	for (auto it = texture_database.begin(); it != texture_database.end() && co < numTextoShow;)
	{
		if (sstrl ==0 || strcasestr(it.key().c_str(), searchStr)) {
			tmpbo = it.value()["downloaded"];
			if (tmpbo) {
				ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
			}
			it.value()["key"] = it.key();
			ImGui::BeginChild(texThumbChildID.c_str(), ImVec2(150, 200));
			ImGui::Image((ImTextureID)textureThumbnails[co]->GetRendererID(), ImVec2(150, 150));
			ImGui::Text(std::string(it.value()["name"]).c_str());
			ShowPopUpSettings(it.value());
			ImGui::EndChild();
			if (tmpbo) {
				ImGui::PopStyleColor();
			}
			ImGui::SameLine();
		}
			++it;
			co++;
			texThumbChildID += "1";
		if (it == texture_database.end())
			break;

		if (sstrl ==0 || strcasestr(it.key().c_str(), searchStr)) {
			it.value()["key"] = it.key();
			tmpbo = it.value()["downloaded"];
			if (tmpbo) {
				ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
			}
			ImGui::BeginChild(texThumbChildID.c_str(), ImVec2(150, 200));
			ImGui::Image((ImTextureID)textureThumbnails[co]->GetRendererID(), ImVec2(150, 150));
			ImGui::Text(std::string(it.value()["name"]).c_str());
			ShowPopUpSettings(it.value());
			ImGui::EndChild();
			if (tmpbo) {
				ImGui::PopStyleColor();
			}
			ImGui::SameLine();
		}
			++it;
			co++;
			texThumbChildID += "2";
		if (it == texture_database.end())
			break;

		if (sstrl == 0 || strcasestr(it.key().c_str(), searchStr)) {
			it.value()["key"] = it.key();
			tmpbo = it.value()["downloaded"];
			if (tmpbo) {
				ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
			}
			ImGui::BeginChild(texThumbChildID.c_str(), ImVec2(150, 200));
			ImGui::Image((ImTextureID)textureThumbnails[co]->GetRendererID(), ImVec2(150, 150));
			ImGui::Text(std::string(it.value()["name"]).c_str());
			ShowPopUpSettings(it.value());
			ImGui::EndChild();
			if (tmpbo) {
				ImGui::PopStyleColor();
			}
			ImGui::SameLine();
		}
			++it;
			co++;
			texThumbChildID += "3";
		if (it == texture_database.end())
			break;

		if (sstrl ==0 || strcasestr(it.key().c_str(), searchStr)) {
			it.value()["key"] = it.key();
			tmpbo = it.value()["downloaded"];
			if (tmpbo) {
				ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
			}
			ImGui::BeginChild(texThumbChildID.c_str(), ImVec2(150, 200));
			ImGui::Image((ImTextureID)textureThumbnails[co]->GetRendererID(), ImVec2(150, 150));
			ImGui::Text(std::string(it.value()["name"]).c_str());
			ShowPopUpSettings(it.value());
			ImGui::EndChild();
			if (tmpbo) {
				ImGui::PopStyleColor();
			}
		}
			++it;
			co++;
			texThumbChildID += "4";
	}
}

static void ShowAbout() {
	ImGui::Image((ImTextureID)phlogo->GetRendererID(), ImVec2(200, 200));
	ImGui::NewLine();
	ImGui::Text("All the the textures in the texture store from PolyHaven!");
	ImGui::Text("Not just free, but CC0, meaning you can use them for");
	ImGui::Text("absolutely any purpose without restrictions.");
	if (ImGui::Button("Open Website")) {
		ShellExecute(NULL, L"open", L"https://polyhaven.com/", NULL, NULL, SW_SHOWNORMAL);
	}
}

static char searchStr[1024];

void ShowTextureStore(bool* pOpen) {
	ImGui::Begin("Texture Store", pOpen);

	if (ImGui::Button("Update")) {
		LoadThumbnails();
	}


	if (intProob) {
		ImGui::SameLine();
		if (ImGui::Button("Retry to Connect")) {
			if (IsNetWorkConnected()) {
				SetupTextureStore(execDir, reqRefresh);
			}
			else {
				Log("Internet Connection Still not available!");
			}
		}
	}

	if (!isConnected) {
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "Internet Connection Not Abailable! Texture Store will not work as expected!");
	}

	ImGui::Text("Work In Progress...");

	ImGui::InputTextWithHint("##TextureStorePolyHavenSearch", "Search", searchStr, 1024);

	ImGui::Separator();
	if (ImGui::BeginTabBar("##shaderEditorTabs", ImGuiTabBarFlags_None))
	{
		bool rout = ImGui::BeginTabItem("Poly Haven");
		if (rout)
		{
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 1));
			ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10);
			ShowTextureThumbs(searchStr, strlen(searchStr));
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
			ImGui::EndTabItem();
		}
		rout = ImGui::BeginTabItem("About");
		if (rout)
		{
			ShowAbout();
			ImGui::EndTabItem();
		}
	}
	ImGui::EndTabBar();
	ImGui::End();
}

std::string GetTextureStoreSelectedTexture() {
	return currTex;
}