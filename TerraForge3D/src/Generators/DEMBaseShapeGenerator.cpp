#include "Generators/DEMBaseShapeGenerator.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"
#include "Profiler.h"
#include "imgui/imgui_internal.h"
#include "webp/decode.h"

static bool AABBContains01(const glm::vec2& minA, const glm::vec2& maxA) 
{
	static const glm::vec2 minB = glm::vec2(0.0f);
	static const glm::vec2 maxB = glm::vec2(1.0f);
	return (maxA.x >= minB.x) && (minA.x <= maxB.x) && (maxA.y >= minB.y) && (minA.y <= maxB.y);
}

DEMBaseShapeGenerator::DEMBaseShapeGenerator(ApplicationState* appState)
{
	m_AppState = appState;
	m_RequireUpdation = true;

	m_APIHostURL = "https://api.maptiler.com";
	m_APIPathURLFormat = "/tiles/terrain-rgb-v2/{0}/{1}/{2}.webp?key={3}";

	m_APIKeyConfigPath = m_AppState->constants.configsDir + PATH_SEPARATOR "maptiler_api_key.terr3d";
	m_TerrainRGBDataCacheDir = m_AppState->constants.cacheDir + PATH_SEPARATOR "dem_data" PATH_SEPARATOR "terrain_rgb" PATH_SEPARATOR;
	MkDir(m_TerrainRGBDataCacheDir); // this will create the directory if it doesn't exist
	m_TerrainRGBDataCacheFileFormat = m_TerrainRGBDataCacheDir + "{}_{}_{}.webp";

	m_APIKeyInput[0] = '\0';
	if (PathExist(m_APIKeyConfigPath))
	{
		m_APIKey = ReadShaderSourceFile(m_APIKeyConfigPath, &s_TempBool);
		strcpy(m_APIKeyInput, m_APIKey.c_str());
	}	
	
	m_LoadingTexture = std::make_shared<Texture2D>(m_AppState->constants.texturesDir + PATH_SEPARATOR "loading.png", false);
	m_NullTexture = std::make_shared<Texture2D>(m_AppState->constants.texturesDir + PATH_SEPARATOR "black.jpg", false);

	// Note: this texture is just for visualizing the map
	//       it is not used for the actual terrain generation
	//       thus it is fixed to a low resolution (512x512) later
	//       this might be configurable through UI
	m_MapVisualzeTexture = std::make_shared<GeneratorTexture>(512, 512);

	const auto shaderSource = ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "dem" PATH_SEPARATOR "map_gen.glsl", &s_TempBool);
	m_Shader = std::make_shared<ComputeShader>(shaderSource);

}

DEMBaseShapeGenerator::~DEMBaseShapeGenerator()
{
}

bool DEMBaseShapeGenerator::ShowSettings()
{
	if (ImGui::CollapsingHeader("Statistics"))
	{
		ImGui::Text("Time Taken : %f", m_CalculationTime);
		ImGui::Text("Tiles Using : %d", m_TilesUsingCount);
	}
	ImGui::InputText("API Key", m_APIKeyInput, 1024);
	if (m_APIKey != m_APIKeyInput && strlen(m_APIKeyInput) > 0 && ImGui::Button("Apply"))
	{
		m_APIKey = m_APIKeyInput;
		// save the api key
		SaveToFile(m_APIKeyConfigPath, m_APIKey);
		m_RequireUpdation = true;
	}
	BIOME_UI_PROPERTY(ImGui::SliderInt("Zoom Resolution", &m_ZoomResolution, 0, 14));

	BIOME_UI_PROPERTY(ImGui::DragFloat("Strength", &m_MapStrength, 0.01f));

	// The Map Widget
	// ImGui::ImageButton((ImTextureID)GetTile(x, y, m_ZoomResolution).get()->GetRendererID(), ImVec2(400, 400));
	ImGui::ImageButton(m_MapVisualzeTexture->GetTextureID(), ImVec2(400, 400));
	ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);

	if (ImGui::IsItemHovered() && (std::abs(ImGui::GetIO().MouseWheel) > 0.05 || ( ImGui::IsMouseDown(ImGuiMouseButton_Middle) && (std::abs(ImGui::GetIO().MouseDelta.x) > 0.001f || std::abs(ImGui::GetIO().MouseDelta.y) > 0.01f))))
	{
		m_ZoomOnMap = m_ZoomOnMap + ImGui::GetIO().MouseWheel * 0.1f * std::exp(std::pow(m_ZoomOnMap - 1.0f, 0.2f));
		m_MapCenter.x += ImGui::GetIO().MouseDelta.x * 0.006f / m_ZoomOnMap;
		m_MapCenter.y += ImGui::GetIO().MouseDelta.y * 0.006f / m_ZoomOnMap;
		m_RequireUpdation = true;
	}

	BIOME_UI_PROPERTY(ImGui::DragFloat("Zoom", &m_ZoomOnMap, 1.0f, 1.0f));
	BIOME_UI_PROPERTY(ImGui::DragFloat2("Center Position", glm::value_ptr(m_MapCenter), 0.01f));
	if(m_RequireUpdation) m_ZoomOnMap = std::clamp(m_ZoomOnMap, 1.0f, 12.0f * 200.0f);


	return m_RequireUpdation;
}

void DEMBaseShapeGenerator::Update(GeneratorData* buffer, GeneratorTexture* seedTexture)
{
	START_PROFILER();
	auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
	auto tileSize = 1.0f / (1 << m_ZoomResolution);

	buffer->Bind(0);
	
	m_Shader->Bind();
	
	// clear the buffer
	m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
	m_Shader->SetUniform1i("u_Mode", 0);
	m_Shader->Dispatch(m_AppState->mainMap.tileResolution / workgroupSize, m_AppState->mainMap.tileResolution / workgroupSize, 1);

	// generate the map data
	m_Shader->SetUniform1i("u_Mode", 1);
	m_Shader->SetUniform1f("u_ZoomOnMap", m_ZoomOnMap);
	m_Shader->SetUniform1f("u_MapStrength", m_MapStrength);
	m_Shader->SetUniform1f("u_RegionTileSize", tileSize * m_ZoomOnMap);

	
	m_TilesUsingCount = 0;
	for (int yi = 0; yi < static_cast<int>(1 << m_ZoomResolution); yi++)
//	for (int yi = tileStartY; yi < tileEndY; yi++)
	{
		for (int xi = 0; xi < static_cast<int>(1 << m_ZoomResolution); xi++)
//		for (int xi = tileStartY; xi < tileEndX; xi++)
		{
			glm::vec2 startPos = (m_MapCenter + glm::vec2(tileSize * xi, tileSize * yi)) * m_ZoomOnMap;
			glm::vec2 endPos = startPos + glm::vec2(tileSize * m_ZoomOnMap);
			if (!AABBContains01(startPos, endPos)) continue;
			m_Shader->SetUniform1i("u_DEMTexture", GetTile(xi, yi, m_ZoomResolution)->Bind(0));
			m_Shader->SetUniform4f("u_RegionToUpdate", glm::vec4(startPos, endPos));
			// TODO: [PLAN] do not dispatch based on map tile but texture tile
			m_Shader->Dispatch(m_AppState->mainMap.tileResolution / workgroupSize, m_AppState->mainMap.tileResolution / workgroupSize, 1);
			m_TilesUsingCount++;
		}
	}

	// update the visualizer map
	m_Shader->SetUniform1i("u_Mode", 2);
	m_MapVisualzeTexture->BindForCompute(1);
	m_Shader->Dispatch(m_MapVisualzeTexture->GetWidth() / workgroupSize, m_MapVisualzeTexture->GetHeight() / workgroupSize, 1);
	END_PROFILER(m_CalculationTime);
	m_RequireUpdation = false;
}

void DEMBaseShapeGenerator::Load(SerializerNode data)
{
	m_ZoomOnMap = data->GetInteger("ZoomOnMap", 1.0f);
	m_MapStrength = data->GetFloat("MapStrength", 1.0f);
	m_MapCenter.x = data->GetFloat("MapCenter_X", 0.0f);
	m_MapCenter.y = data->GetFloat("MapCenter_Y", 0.0f);
	m_RequireUpdation = true;
}

SerializerNode DEMBaseShapeGenerator::Save()
{
	auto node = CreateSerializerNode();
	node->SetInteger("ZoomOnMap", m_ZoomOnMap);
	node->SetFloat("MapStrength", m_MapStrength);
	node->SetFloat("MapCenter_X", m_MapCenter.x);
	node->SetFloat("MapCenter_Y", m_MapCenter.y);
	return node;
}

bool DEMBaseShapeGenerator::IsTileValid(uint32_t x, uint32_t y, uint32_t z)
{
	// z should be between 0 and 12
	if (z < 0u || z > 12u)
	{
		return false;
	}

	// x and y should be between 0 and 2^z
	if (x < 0u || x > (1u << z) - 1u || y < 0u || y > (1u << z) - 1u)
	{
		return false;
	}

	return true;
}

std::shared_ptr<Texture2D> DEMBaseShapeGenerator::LoadTile(uint32_t x, uint32_t y, uint32_t z)
{
	// check if tile is valid
	if (!IsTileValid(x, y, z))
	{
		Log(std::format("Tile {} {} {} is not valid", x, y, z));
		return m_NullTexture;
	}

	auto textureCacheKey = TextureCacheKey(x, y, z);
	// check if tile is already loaded
	auto result = m_TextureCache.find(textureCacheKey);
	if (result != m_TextureCache.end())
	{
		return result->second;
	}


	// check if tile is already downloaded
	auto path = std::vformat(m_TerrainRGBDataCacheFileFormat, std::make_format_args(x, y, z));

	// check is the texture is downloading
	if (std::find(m_TextureDownloadQueue.begin(), m_TextureDownloadQueue.end(), textureCacheKey) != m_TextureDownloadQueue.end())
	{
		return m_LoadingTexture;
	}	

	// if not, download it
	if (!PathExist(path))
	{
		DownloadTerrainRGBTexture(textureCacheKey);
		return m_LoadingTexture;
	}

	// load the tile

	// CLEAN UP THIS CODE
	static int w = 1, h = 1, sz = 0;
	uint8_t* data = (uint8_t*)ReadBinaryFile(path, &sz);
	auto rgbData = WebPDecodeRGBA(data, sz, &w, &h);
	auto texPtr = std::make_shared<Texture2D>(w, h);
	m_TextureCache[textureCacheKey] = texPtr;
	texPtr->SetData(rgbData, 0, true);
	delete[] data;
	free(rgbData);
	// CLEAN UP THIS CODE

	return texPtr;
}

void DEMBaseShapeGenerator::DownloadTerrainRGBTexture(TextureCacheKey key)
{
	m_TextureDownloadQueue.push_back(key);
	auto downloadWorker = [this, key]()
	{
		auto [x, y, z] = key;
		auto path = std::vformat(m_TerrainRGBDataCacheFileFormat, std::make_format_args(x, y, z));
		auto urlPath = std::vformat(m_APIPathURLFormat, std::make_format_args(z, x, y, m_APIKey));
		DownloadFile(m_APIHostURL, urlPath, path);
	};
	m_AppState->jobSystem->AddFunctionWorker(downloadWorker)->onComplete = [this, key](auto*)->void { m_TextureDownloadQueue.erase(std::find(m_TextureDownloadQueue.begin(), m_TextureDownloadQueue.end(), key)); m_RequireUpdation = true; m_AppState->eventManager->RaiseEvent("ForceUpdate", "ForceUpdate"); };
}
