#pragma once
#include <string>
#include <Texture2D.h>

struct GitHubData {

	std::string name;
	Texture2D* avatar;


};

void SetupSupportersTribute();

void ShowSupportersTribute(bool* pOpen);
