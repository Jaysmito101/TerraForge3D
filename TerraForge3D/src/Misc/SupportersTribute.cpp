#include "Misc/SupportersTribute.h"

#include "Base/Texture2D.h"
#include "Platform.h"

#include <Utils.h>
#include <cstdlib>
#include <imgui.h>
#include <vector>
#include <string>
#include <json.hpp>


void SupportersTribute::LoadstargazersData(nlohmann::json &data)
{
	bool isNetWorkConnected = IsNetWorkConnected();

	for (nlohmann::json item : data)
	{
		if (!item.is_object() || !item.contains("login") || !item.contains("node_id") || !item.contains("avatar_url")) continue;
		GitHubData st;
		st.name = item["login"];

		if (!PathExist(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "github_avatars"))
		{
			MkDir(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "github_avatars");
		}

		if (!FileExists(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "github_avatars" PATH_SEPARATOR + st.name + "_" + std::string(item["node_id"])) && isNetWorkConnected)
		{
			std::string urlFull = item["avatar_url"];
			std::string baseURL = urlFull.substr(0, 37);
			std::string pathURL = urlFull.substr(38);
			DownloadFile(baseURL, pathURL, GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "github_avatars" PATH_SEPARATOR + st.name + "_" + std::string(item["node_id"]));
		}

		st.avatar = new Texture2D(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "github_avatars" PATH_SEPARATOR + st.name + "_" + std::string(item["node_id"]));
		stargazers.push_back(st);
	}
}

void SupportersTribute::LoadcontributorsData(nlohmann::json &data)
{
	bool isNetWorkConnected = IsNetWorkConnected();

	for (nlohmann::json item : data)
	{
		if (!item.is_object() || !item.contains("login") || !item.contains("node_id") || !item.contains("avatar_url")) continue;
		GitHubData st;
		st.name = item["login"];

		if (!PathExist(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "github_avatars"))
		{
			MkDir(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "github_avatars");
		}

		if (!FileExists(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "github_avatars" PATH_SEPARATOR + st.name + "_" + std::string(item["node_id"])) && isNetWorkConnected)
		{
			std::string urlFull = item["avatar_url"];
			std::string baseURL = urlFull.substr(0, 37);
			std::string pathURL = urlFull.substr(38);
			DownloadFile(baseURL, pathURL, GetExecutableDir() + PATH_SEPARATOR
					"Data" PATH_SEPARATOR "cache" PATH_SEPARATOR
					"github_avatars" PATH_SEPARATOR + st.name + "_" +
					std::string(item["node_id"]));
		}

		st.avatar = new Texture2D(GetExecutableDir() + PATH_SEPARATOR "Data"
				PATH_SEPARATOR "cache" PATH_SEPARATOR "github_avatars"
				PATH_SEPARATOR + st.name + "_" +
				std::string(item["node_id"]));
		contributors.push_back(st);
	}
}


SupportersTribute::SupportersTribute()
{
	const char *githubTokenValue = std::getenv("TERR3D_GITHUB_TOKEN");
	const std::string githubToken = githubTokenValue ? githubTokenValue : "";
	const bool hasGithubToken = !githubToken.empty();
	stargazersUnavailable = !hasGithubToken;

	if (hasGithubToken && IsNetWorkConnected() && (!FileExists(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "stargazers.terr3dcache") || rand() % 10 == 0))
	{
		Log("Internet Connection is Live!\nFetching Latest Supporters data.");
		{
			std::string repoDataStr = FetchURL("https://api.github.com", "/repos/Jaysmito101/TerraForge3D", githubToken);
			nlohmann::json repoData = nlohmann::json::parse(repoDataStr, nullptr, false);
			if (repoData.is_object() && repoData.contains("stargazers_count") &&
				(repoData["stargazers_count"].is_number_integer() || repoData["stargazers_count"].is_number_unsigned()))
			{
				int stargazerCount = repoData["stargazers_count"];
				int perPage = 30;
				int lastPage = stargazerCount / perPage;
				std::string stargazersRawData = FetchURL("https://api.github.com", "/repos/Jaysmito101/TerraForge3D/stargazers?per_page=30&page=" + std::to_string(lastPage), githubToken);
				nlohmann::json stargazersData = nlohmann::json::parse(stargazersRawData, nullptr, false);
				if (stargazersData.is_array())
				{
					SaveToFile(GetExecutableDir() + PATH_SEPARATOR +"Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "stargazers.terr3dcache", stargazersRawData);
					LoadstargazersData(stargazersData);
				}
			}
		}
		{
			std::string contributorsRawData = FetchURL("https://api.github.com", "/repos/Jaysmito101/TerraForge3D/contributors");
			SaveToFile(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "contributors.terr3dcache", contributorsRawData);
			nlohmann::json contributorsData = nlohmann::json::parse(contributorsRawData, nullptr, false);
			if (contributorsData.is_array()) LoadcontributorsData(contributorsData);
		}
	}

	else
	{
		bool tmp = false;
		Log("Trying to load cached data.");

		if (hasGithubToken && FileExists(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "stargazers.terr3dcache"))
		{
			Log("Found Stargazers Cached Data!");
			std::string stargazersRawData = ReadShaderSourceFile(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "stargazers.terr3dcache", &tmp);
			nlohmann::json stargazersData = nlohmann::json::parse(stargazersRawData, nullptr, false);
			if (stargazersData.is_array()) LoadstargazersData(stargazersData);
		}

		else if (hasGithubToken)
		{
			Log("Stargazers Cached Data not found!");
		}

		else
		{
			Log("Stargazers are unavailable without TERR3D_GITHUB_TOKEN.");
		}

		if (FileExists(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "contributors.terr3dcache"))
		{
			Log("Found Contributors Cached Data!");
			std::string contributorsRawData = ReadShaderSourceFile(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "contributors.terr3dcache", &tmp);
			nlohmann::json contributorsData = nlohmann::json::parse(contributorsRawData, nullptr, false);
			if (contributorsData.is_array()) LoadcontributorsData(contributorsData);
		}

		else
		{
			Log("Contributors Cached Data not found!");
		}
	}
}

SupportersTribute::~SupportersTribute()
{
	for(auto &st : stargazers)
	{
		if(st.avatar)
		{
			delete st.avatar;
		}
	}

	for(auto &co : contributors)
	{
		if(co.avatar)
		{
			delete co.avatar;
		}
	}
}

void SupportersTribute::ShowSettings(bool *pOpen)
{
	ImGui::Begin("Supporters", pOpen, ImGuiWindowFlags_NoResize);
	ImGui::SetWindowSize(ImVec2(250, 250));
	ImGui::Separator();
	ImGui::Text("Contributors");

	for (GitHubData st : contributors)
	{
		uint32_t avTexId = 0;

		if (st.avatar)
		{
			avTexId = st.avatar->GetRendererID();
		}

		ImGui::Image((ImTextureID)(uint64_t)avTexId, ImVec2(30, 30));
		ImGui::SameLine();
		ImGui::Text(st.name.c_str());
	}

	ImGui::Separator();
	ImGui::Separator();
	ImGui::Text("Stargazers");

	if (stargazersUnavailable)
	{
		ImGui::TextWrapped("Stargazers aren't available on the public GitHub API due to GitHub changes.");
		if (ImGui::SmallButton("GitHub API changes"))
		{
			OpenURL("https://github.blog/changelog/2026-06-30-upcoming-access-restrictions-to-public-api-endpoints-and-ui-views/");
		}
		ImGui::TextWrapped("Please set TERR3D_GITHUB_TOKEN to see them.");
	}

	for (GitHubData st : stargazers)
	{
		uint32_t avTexId = 0;

		if (st.avatar)
		{
			avTexId = st.avatar->GetRendererID();
		}

		ImGui::Image((ImTextureID)(uint64_t)avTexId, ImVec2(30, 30));
		ImGui::SameLine();
		ImGui::Text(st.name.c_str());
	}

	ImGui::Separator();
	ImGui::End();
}
