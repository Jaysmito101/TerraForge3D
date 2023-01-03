#pragma once
#include <glm/glm.hpp>

#include <string>

class ApplicationState;
class CubeMapManager;
class TextureCubemap;

class SkyManager
{
public:
	SkyManager(ApplicationState *appState);
	~SkyManager();

	void ShowSettings(bool *pOpen);

	void RenderSky(glm::mat4 vie, glm::mat4 pers);

private:
	void LoadHDRI(std::string path);
	void ChangeCubemapTile(int face);

public:
	TextureCubemap *cubemap = nullptr;
	CubeMapManager *cubemapManager = nullptr;
	bool useBox = false;
	bool useProcedural = true;
	float cltime = 0.0f;
	float upf = 0.35f;
	float cirrus = 0.4f;
	float cumulus = 0.8f;
	float fsun[3] = {0.0f, 0.2f, 0.1f};
	ApplicationState *appState = nullptr;
};