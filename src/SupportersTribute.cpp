#include "SupportersTribute.h"
#include <Utils.h>
#include <imgui.h>
#include <vector>
#include <string>
#include <json.hpp>

std::vector<GitHubData> stargazers;
std::vector<GitHubData> contributors;

void LoadstargazersData(nlohmann::json& data) {
	bool isNetWorkConnected = IsNetWorkConnected();
	for (nlohmann::json item : data) {
		GitHubData st;
		st.name = item["login"];
		if (!PathExist(GetExecutableDir() + "\\Data\\cache\\github_avatars"))
			MkDir(GetExecutableDir() + "\\Data\\cache\\github_avatars");
		if (!FileExists(GetExecutableDir() + "\\Data\\cache\\github_avatars\\" + st.name + "_" + std::string(item["node_id"])) && isNetWorkConnected) {
			std::string urlFull = item["avatar_url"];
			std::string baseURL = urlFull.substr(0, 37);
			std::string pathURL = urlFull.substr(38);
			DownloadFile(baseURL, pathURL, GetExecutableDir() + "\\Data\\cache\\github_avatars\\" + st.name + "_" + std::string(item["node_id"]));
		}
		st.avatar = new Texture2D(GetExecutableDir() + "\\Data\\cache\\github_avatars\\" + st.name + "_" + std::string(item["node_id"]));
		stargazers.push_back(st);
	}
}

void LoadcontributorsData(nlohmann::json& data) {
	bool isNetWorkConnected = IsNetWorkConnected();
	for (nlohmann::json item : data) {
		GitHubData st;
		st.name = item["login"];
		if (!PathExist(GetExecutableDir() + "\\Data\\cache\\github_avatars"))
			MkDir(GetExecutableDir() + "\\Data\\cache\\github_avatars");
		if (!FileExists(GetExecutableDir() + "\\Data\\cache\\github_avatars\\" + st.name + "_" + std::string(item["node_id"])) && isNetWorkConnected) {
			std::string urlFull = item["avatar_url"];
			std::string baseURL = urlFull.substr(0, 37);
			std::string pathURL = urlFull.substr(38);
			DownloadFile(baseURL, pathURL, GetExecutableDir() + "\\Data\\cache\\github_avatars\\" + st.name + "_" + std::string(item["node_id"]));
		}
		st.avatar = new Texture2D(GetExecutableDir() + "\\Data\\cache\\github_avatars\\" + st.name + "_" + std::string(item["node_id"]));
		contributors.push_back(st);
	}
}


void SetupSupportersTribute()
{
	if (IsNetWorkConnected()) {
		Log("Internet Connection is Live!\nFetching Latest Supporters data.");
		
		{
			std::string stargazersRawData = FetchURL("https://api.github.com", "/repos/Jaysmito101/TerraGen3D/stargazers");
			SaveToFile(GetExecutableDir() + "\\Data\\cache\\stargazers.terr3dcache", stargazersRawData);
			nlohmann::json stargazersData = nlohmann::json::parse(stargazersRawData);
			LoadstargazersData(stargazersData);
		}

		{
			std::string contributorsRawData = FetchURL("https://api.github.com", "/repos/Jaysmito101/TerraGen3D/contributors");
			SaveToFile(GetExecutableDir() + "\\Data\\cache\\contributors.terr3dcache", contributorsRawData);
			nlohmann::json contributorsData = nlohmann::json::parse(contributorsRawData);
			LoadcontributorsData(contributorsData);
		}
	}
	else {
		bool tmp = false;
		Log("Failed to connect to Internet!\nTrying to load cached data.");

		if (FileExists(GetExecutableDir() + "\\Data\\cache\\stargazers.terr3dcache")) {
			Log("Found Stargazers Cached Data!");
			std::string stargazersRawData = ReadShaderSourceFile(GetExecutableDir() + "\\Data\\cache\\stargazers.terr3dcache", &tmp);
			nlohmann::json stargazersData = nlohmann::json::parse(stargazersRawData);
			LoadstargazersData(stargazersData);
		}
		else {
			Log("Stargazers Cached Data not found!");
		}

		if (FileExists(GetExecutableDir() + "\\Data\\cache\\contributors.terr3dcache")) {
			Log("Found Contributors Cached Data!");
			std::string contributorsRawData = ReadShaderSourceFile(GetExecutableDir() + "\\Data\\cache\\stargazers.terr3dcache", &tmp);
			nlohmann::json contributorsData = nlohmann::json::parse(contributorsRawData);
			LoadcontributorsData(contributorsData);
		}
		else {
			Log("Contributors Cached Data not found!");
		}
	}
}

void ShowSupportersTribute(bool* pOpen)
{
	ImGui::Begin("Supporters", pOpen);

	ImGui::Separator();
	ImGui::Text("Contributors");

	for (GitHubData st : contributors) {
		uint32_t avTexId = 0;
		if (st.avatar)
			avTexId = st.avatar->GetRendererID();
		ImGui::Image((ImTextureID)avTexId, ImVec2(30, 30));
		ImGui::SameLine();
		ImGui::Text(st.name.c_str());
	}
	ImGui::Separator();

	ImGui::Separator();
	ImGui::Text("Stargazers");

	for (GitHubData st : stargazers) {
		uint32_t avTexId = 0;
		if (st.avatar)
			avTexId = st.avatar->GetRendererID();
		ImGui::Image((ImTextureID)avTexId, ImVec2(30, 30));
		ImGui::SameLine();
		ImGui::Text(st.name.c_str());
	}
	ImGui::Separator();
	ImGui::End();
}