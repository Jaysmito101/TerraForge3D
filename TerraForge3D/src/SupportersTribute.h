#pragma once

#include <string>
#include <vector>

#include <json.hpp>

class Texture2D;


struct GitHubData {
	std::string name = "";
	Texture2D* avatar = nullptr;
};

class SupportersTribute
{
public:
	SupportersTribute();
	~SupportersTribute();
	void ShowSettings(bool* pOpen);

private:
	void LoadstargazersData(nlohmann::json& data);
	void LoadcontributorsData(nlohmann::json& data);

public:
	std::vector<GitHubData> stargazers;
	std::vector<GitHubData> contributors;
};


