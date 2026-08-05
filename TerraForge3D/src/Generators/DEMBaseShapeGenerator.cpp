#include "Generators/DEMBaseShapeGenerator.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"
#include "Profiler.h"
#include "imgui/imgui_internal.h"
#include "webp/decode.h"

#include <cmath>
#include <cstdio>

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
	bool loadedFromConfig = false;
	if (m_AppState->configManager != nullptr) {
		loadedFromConfig = m_AppState->configManager->GetString("apiKeys", "maptilerCloud", m_APIKey);
	}

	if (!loadedFromConfig && PathExist(m_APIKeyConfigPath))
	{
		m_APIKey = ReadShaderSourceFile(m_APIKeyConfigPath, &s_TempBool);
		if (!m_APIKey.empty() && m_AppState->configManager != nullptr) { 
			m_AppState->configManager->SetString("apiKeys", "maptilerCloud", m_APIKey);
		}
	}	
	std::snprintf(m_APIKeyInput, sizeof(m_APIKeyInput), "%s", m_APIKey.c_str());
	
	m_LoadingTexture = std::make_shared<Texture2D>(m_AppState->constants.texturesDir + PATH_SEPARATOR "loading.png", false);
	m_NullTexture = std::make_shared<Texture2D>(m_AppState->constants.texturesDir + PATH_SEPARATOR "black.jpg", false);

	// Note: this texture is just for visualizing the map
	//       it is not used for the actual terrain generation
	//       thus it is fixed to a low resolution (512x512) later
	//       this might be configurable through UI
	m_MapVisualzeTexture = std::make_shared<GeneratorTexture>(512, 512);

	// const auto shaderSource = ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "dem" PATH_SEPARATOR "map_gen.glsl", &s_TempBool);
	// m_Shader = std::make_shared<ComputeShader>(shaderSource);
	m_Shader = m_AppState->resourceManager->LoadComputeShader("generation/dem/map_gen");
}

DEMBaseShapeGenerator::~DEMBaseShapeGenerator()
{
}

bool DEMBaseShapeGenerator::ShowSettings()
{
	if (m_ViewInteractionPending &&
		std::chrono::steady_clock::now() - m_LastViewInteractionTime >= std::chrono::milliseconds(kTileRequestDebounceMilliseconds))
	{
		m_ViewInteractionPending = false;
		m_RequireUpdation = true;
	}

	if (ImGui::CollapsingHeader("Statistics"))
	{
		ImGui::Text("Tiles Using : %d", m_TilesUsingCount);
	}
	ImGui::TextDisabled("Elevation provider: MapTiler Cloud (Terrain RGB)");
	ImGui::TextWrapped("Paste an API key created in your MapTiler Cloud account. This key is used to download DEM elevation tiles.");
	ImGui::InputText("MapTiler Cloud API Key", m_APIKeyInput, 1024);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Create or copy a key from your MapTiler Cloud account: cloud.maptiler.com/account/keys/");
	ImGui::TextDisabled("Saved in the TerraForge3D user config, not in project files.");
	if (m_APIKey != m_APIKeyInput && m_APIKeyInput[0] != '\0' && ImGui::Button("Apply"))
	{
		m_APIKey = m_APIKeyInput;
		const bool savedInConfig = m_AppState->configManager != nullptr
			&& m_AppState->configManager->SetString("apiKeys", "maptilerCloud", m_APIKey);
		if (!savedInConfig)
		{
			// Keep a fallback for unusual startup/config-write failures.
			SaveToFile(m_APIKeyConfigPath, m_APIKey);
			TF3D_LOG_WARN("Could not save the MapTiler key in the user config store; saved the legacy DEM key file instead.");
		}
		m_RequireUpdation = true;
	}
	if (m_APIKey.empty())
		ImGui::TextDisabled("Add a MapTiler Cloud key before downloading elevation tiles.");

	if (ImGui::Checkbox("Automatic Tile Zoom", &m_AutoZoomResolution))
	{
		m_RequireUpdation = true;
		MarkViewInteraction();
	}
	if (!m_AutoZoomResolution)
	{
		if (ImGui::SliderInt("Manual Tile Zoom", &m_ZoomResolution, 0, kMaxTileZoom))
		{
			m_RequireUpdation = true;
			MarkViewInteraction();
		}
	}
	int previewZoom = GetEffectiveZoomResolution();
	int previewTileCount = GetVisibleTileCount(previewZoom);
	ImGui::Text("Effective tile zoom: %d (%s)", previewZoom, m_AutoZoomResolution ? "automatic" : "manual");
	ImGui::TextDisabled("Visible tiles: %d/%d | pending requests: %d/%d", previewTileCount, kMaxVisibleTiles, static_cast<int>(m_TextureDownloadQueue.size()), kMaxPendingTileRequests);
	ImGui::TextDisabled("Rendered tiles: %d high-res | %d fallback-covered", m_TilesUsingCount, m_TilesFallbackCount);
	ImGui::TextDisabled("At most %d new requests per refresh; downloads wait %d ms after view movement.",
		kMaxRequestsPerRefresh, kTileRequestDebounceMilliseconds);

	m_MapStrength = std::clamp(m_MapStrength, 0.0f, 20.0f);
	BIOME_UI_PROPERTY(ImGui::DragFloat("Strength", &m_MapStrength, 0.01f, 0.0f, 20.0f));

	// The Map Widget
	// ImGui::ImageButton((ImTextureID)GetTile(x, y, m_ZoomResolution).get()->GetRendererID(), ImVec2(400, 400));
	ImGui::ImageButton(m_MapVisualzeTexture->GetTextureID(), ImVec2(400, 400));
	ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);

	if (ImGui::IsItemHovered() && (std::abs(ImGui::GetIO().MouseWheel) > 0.05 || ( ImGui::IsMouseDown(ImGuiMouseButton_Middle) && (std::abs(ImGui::GetIO().MouseDelta.x) > 0.001f || std::abs(ImGui::GetIO().MouseDelta.y) > 0.01f))))
	{
		const ImVec2 imageMin = ImGui::GetItemRectMin();
		const ImVec2 imageMax = ImGui::GetItemRectMax();
		const ImVec2 mousePosition = ImGui::GetIO().MousePos;
		const glm::vec2 imageSize(
			std::max(imageMax.x - imageMin.x, 1.0f),
			std::max(imageMax.y - imageMin.y, 1.0f));
		const glm::vec2 cursorUV = glm::clamp(
			glm::vec2(
				(mousePosition.x - imageMin.x) / imageSize.x,
				(mousePosition.y - imageMin.y) / imageSize.y),
			glm::vec2(0.0f), glm::vec2(1.0f));

		if (std::abs(ImGui::GetIO().MouseWheel) > 0.05f)
		{
			const float oldZoom = m_ZoomOnMap;
			const glm::vec2 cursorWorld = cursorUV / oldZoom - m_MapCenter;
			m_ZoomOnMap *= std::pow(1.18f, ImGui::GetIO().MouseWheel);
			m_ZoomOnMap = std::clamp(m_ZoomOnMap, 1.0f, 4096.0f);
			m_MapCenter = cursorUV / m_ZoomOnMap - cursorWorld;
		}

		m_MapCenter.x += ImGui::GetIO().MouseDelta.x * 0.006f / m_ZoomOnMap;
		m_MapCenter.y += ImGui::GetIO().MouseDelta.y * 0.006f / m_ZoomOnMap;
		m_ZoomOnMap = std::clamp(m_ZoomOnMap, 1.0f, 4096.0f);
		m_MapCenter.x = std::clamp(m_MapCenter.x, -4.0f, 4.0f);
		m_MapCenter.y = std::clamp(m_MapCenter.y, -4.0f, 4.0f);
		MarkViewInteraction();
	}

	if (ImGui::DragFloat("View Zoom", &m_ZoomOnMap, 0.05f, 1.0f, 4096.0f)) MarkViewInteraction();
	if (ImGui::DragFloat2("Center Position", glm::value_ptr(m_MapCenter), 0.01f)) MarkViewInteraction();
	m_ZoomOnMap = std::clamp(m_ZoomOnMap, 1.0f, 4096.0f);
	m_MapCenter.x = std::clamp(m_MapCenter.x, -4.0f, 4.0f);
	m_MapCenter.y = std::clamp(m_MapCenter.y, -4.0f, 4.0f);
	m_MapStrength = std::clamp(m_MapStrength, 0.0f, 20.0f);


	return m_RequireUpdation;
}

void DEMBaseShapeGenerator::MarkViewInteraction()
{
	m_ViewInteractionPending = true;
	m_LastViewInteractionTime = std::chrono::steady_clock::now();
	m_RequireUpdation = true;
}

std::shared_ptr<Texture2D> DEMBaseShapeGenerator::FindBestAvailableTile(
	uint32_t x, uint32_t y, uint32_t z, TextureCacheKey& resolvedKey)
{
	if (z == 0) return nullptr;
	bool requestedNearestFallback = false;
	for (int32_t fallbackZoom = static_cast<int32_t>(z) - 1; fallbackZoom >= 0; --fallbackZoom)
	{
		const uint32_t shift = z - static_cast<uint32_t>(fallbackZoom);
		const TextureCacheKey candidateKey(x >> shift, y >> shift, static_cast<uint32_t>(fallbackZoom));
		auto cached = m_TextureCache.find(candidateKey);
		if (cached != m_TextureCache.end())
		{
			resolvedKey = candidateKey;
			return cached->second;
		}

		const auto path = fmt::vformat(m_TerrainRGBDataCacheFileFormat,
			fmt::make_format_args(std::get<0>(candidateKey), std::get<1>(candidateKey), std::get<2>(candidateKey)));
		if (!PathExist(path))
		{
			if (!requestedNearestFallback)
			{
				LoadTile(std::get<0>(candidateKey), std::get<1>(candidateKey), std::get<2>(candidateKey));
				requestedNearestFallback = true;
			}
			continue;
		}

		auto tile = LoadTile(std::get<0>(candidateKey), std::get<1>(candidateKey), std::get<2>(candidateKey));
		if (HasTileLoaded(std::get<0>(candidateKey), std::get<1>(candidateKey), std::get<2>(candidateKey)))
		{
			resolvedKey = candidateKey;
			return tile;
		}
	}
	return nullptr;
}

void DEMBaseShapeGenerator::GetVisibleTileRange(int32_t zoomResolution, int32_t& minTileX, int32_t& maxTileX, int32_t& minTileY, int32_t& maxTileY) const
{
	zoomResolution = std::clamp(zoomResolution, 0, kMaxTileZoom);
	const int32_t tileCount = 1 << zoomResolution;
	const float tileSize = 1.0f / static_cast<float>(tileCount);
	const float viewZoom = std::clamp(m_ZoomOnMap, 1.0f, 4096.0f);
	const float viewEnd = 1.0f / viewZoom;
	const float edgeEpsilon = 0.00001f;

	auto calculateRange = [&](float center, int32_t& minTile, int32_t& maxTile)
	{
		const int32_t rawMin = static_cast<int32_t>(std::ceil((-center / tileSize) - 1.0f - edgeEpsilon));
		const int32_t rawMax = static_cast<int32_t>(std::ceil(((viewEnd - center) / tileSize) - edgeEpsilon)) - 1;
		minTile = std::max(0, rawMin);
		maxTile = std::min(tileCount - 1, rawMax);
	};

	calculateRange(m_MapCenter.x, minTileX, maxTileX);
	calculateRange(m_MapCenter.y, minTileY, maxTileY);
}

int32_t DEMBaseShapeGenerator::GetVisibleTileCount(int32_t zoomResolution) const
{
	int32_t minTileX = 0, maxTileX = -1, minTileY = 0, maxTileY = -1;
	GetVisibleTileRange(zoomResolution, minTileX, maxTileX, minTileY, maxTileY);
	if (maxTileX < minTileX || maxTileY < minTileY) return 0;
	return (maxTileX - minTileX + 1) * (maxTileY - minTileY + 1);
}

int32_t DEMBaseShapeGenerator::GetEffectiveZoomResolution() const
{
	const float viewZoom = std::clamp(m_ZoomOnMap, 1.0f, 4096.0f);
	int32_t requestedZoom = m_AutoZoomResolution
		? static_cast<int32_t>(std::floor(std::log2(viewZoom))) + 3
		: m_ZoomResolution;
	requestedZoom = std::clamp(requestedZoom, 0, kMaxTileZoom);

	while (requestedZoom > 0 && GetVisibleTileCount(requestedZoom) > kMaxVisibleTiles)
		--requestedZoom;
	return requestedZoom;
}

void DEMBaseShapeGenerator::Update(GeneratorData* buffer, GeneratorTexture* seedTexture)
{
	TF3D_PROFILE_SCOPE("generation/dem-base-shape");
	auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
	const auto now = std::chrono::steady_clock::now();
	if (m_ViewInteractionPending
		&& now - m_LastViewInteractionTime >= std::chrono::milliseconds(kTileRequestDebounceMilliseconds))
	{
		m_ViewInteractionPending = false;
	}
	m_AllowTileRequestsThisUpdate = !m_ViewInteractionPending;
	m_ZoomOnMap = std::clamp(m_ZoomOnMap, 1.0f, 4096.0f);
	m_MapCenter.x = std::clamp(m_MapCenter.x, -4.0f, 4.0f);
	m_MapCenter.y = std::clamp(m_MapCenter.y, -4.0f, 4.0f);
	m_EffectiveZoomResolution = GetEffectiveZoomResolution();
	m_VisibleTileCount = GetVisibleTileCount(m_EffectiveZoomResolution);
	m_RequestsScheduledThisUpdate = 0;

	int32_t minTileX = 0, maxTileX = -1, minTileY = 0, maxTileY = -1;
	GetVisibleTileRange(m_EffectiveZoomResolution, minTileX, maxTileX, minTileY, maxTileY);
	const int32_t requestedZoom = std::clamp(m_ZoomResolution, 0, kMaxTileZoom);
	m_TilesSkippedCount = std::max(0, GetVisibleTileCount(requestedZoom) - m_VisibleTileCount);

	buffer->Bind(0);
	m_Shader->Bind();
	
	// clear the buffer
	m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
	m_Shader->SetUniform1i("u_Mode", 0);
	m_Shader->Dispatch(m_AppState->mainMap.tileResolution / workgroupSize, m_AppState->mainMap.tileResolution / workgroupSize, 1);
	m_Shader->SetMemoryBarrier();

	// generate the map data
	m_Shader->SetUniform1i("u_Mode", 1);
	m_Shader->SetUniform1f("u_ZoomOnMap", m_ZoomOnMap);
	m_Shader->SetUniform1f("u_MapStrength", m_MapStrength);

	m_TilesUsingCount = 0;
	m_TilesFallbackCount = 0;
	struct PreviewTile
	{
		TextureCacheKey key;
		std::shared_ptr<Texture2D> texture;
	};
	std::vector<PreviewTile> fallbackTiles;
	std::vector<PreviewTile> highResolutionTiles;
	const auto effectiveZoom = static_cast<uint32_t>(m_EffectiveZoomResolution);
	for (int yi = minTileY; yi <= maxTileY; ++yi)
	{
		for (int xi = minTileX; xi <= maxTileX; ++xi)
		{
			const auto tileX = static_cast<uint32_t>(xi);
			const auto tileY = static_cast<uint32_t>(yi);
			auto exactTile = m_TextureCache.find(TextureCacheKey(tileX, tileY, effectiveZoom));
			if (exactTile != m_TextureCache.end())
			{
				highResolutionTiles.push_back({ TextureCacheKey(tileX, tileY, effectiveZoom), exactTile->second });
				continue;
			}

			TextureCacheKey fallbackKey;
			auto fallback = FindBestAvailableTile(tileX, tileY, effectiveZoom, fallbackKey);
			if (fallback != nullptr)
			{
				const auto duplicate = std::find_if(fallbackTiles.begin(), fallbackTiles.end(),
					[&fallbackKey](const PreviewTile& candidate) { return candidate.key == fallbackKey; });
				if (duplicate == fallbackTiles.end())
					fallbackTiles.push_back({ fallbackKey, fallback });
				++m_TilesFallbackCount;
			}

			auto tile = GetTile(tileX, tileY, effectiveZoom);
			if (HasTileLoaded(tileX, tileY, effectiveZoom))
				highResolutionTiles.push_back({ TextureCacheKey(tileX, tileY, effectiveZoom), tile });
		}
	}

	auto renderPreviewTile = [&](const PreviewTile& previewTile)
	{
		const auto tileZoom = std::get<2>(previewTile.key);
		const float tileSize = 1.0f / static_cast<float>(1u << tileZoom);
		const glm::vec2 startPos = (m_MapCenter + glm::vec2(
			tileSize * static_cast<float>(std::get<0>(previewTile.key)),
			tileSize * static_cast<float>(std::get<1>(previewTile.key)))) * m_ZoomOnMap;
		const glm::vec2 endPos = startPos + glm::vec2(tileSize * m_ZoomOnMap);
		m_Shader->SetUniform1f("u_RegionTileSize", tileSize * m_ZoomOnMap);
		m_Shader->SetUniform1i("u_DEMTexture", previewTile.texture->Bind(0));
		m_Shader->SetUniform4f("u_RegionToUpdate", glm::vec4(startPos, endPos));
		m_Shader->Dispatch(m_AppState->mainMap.tileResolution / workgroupSize, m_AppState->mainMap.tileResolution / workgroupSize, 1);
	};

	for (const auto& tile : fallbackTiles) {
		renderPreviewTile(tile);
	}
	for (const auto& tile : highResolutionTiles)
	{
		renderPreviewTile(tile);
		++m_TilesUsingCount;
	}

	// update the visualizer map
	// Mode 2 reads the SSBO written by all mode 1 dispatches.
	m_Shader->SetMemoryBarrier();
	m_Shader->SetUniform1i("u_Mode", 2);
	m_MapVisualzeTexture->BindForCompute(1);
	m_Shader->Dispatch(m_MapVisualzeTexture->GetWidth() / workgroupSize, m_MapVisualzeTexture->GetHeight() / workgroupSize, 1);
	m_Shader->SetMemoryBarrier();
	m_RequireUpdation = false;
}

void DEMBaseShapeGenerator::Load(SerializerNode data)
{
	m_ZoomOnMap = data->GetFloat("ZoomOnMap", 1.0f);
	m_ZoomResolution = data->GetInteger("ZoomResolution", 0);
	m_AutoZoomResolution = data->GetInteger("AutoZoomResolution", 1) != 0;
	m_MapStrength = data->GetFloat("MapStrength", 1.0f);
	m_MapCenter.x = data->GetFloat("MapCenter_X", 0.0f);
	m_MapCenter.y = data->GetFloat("MapCenter_Y", 0.0f);
	m_RequireUpdation = true;
}

SerializerNode DEMBaseShapeGenerator::Save()
{
	auto node = CreateSerializerNode();
	node->SetFloat("ZoomOnMap", m_ZoomOnMap);
	node->SetInteger("ZoomResolution", m_ZoomResolution);
	node->SetInteger("AutoZoomResolution", m_AutoZoomResolution ? 1 : 0);
	node->SetFloat("MapStrength", m_MapStrength);
	node->SetFloat("MapCenter_X", m_MapCenter.x);
	node->SetFloat("MapCenter_Y", m_MapCenter.y);
	return node;
}

bool DEMBaseShapeGenerator::IsTileValid(uint32_t x, uint32_t y, uint32_t z)
{
	if (z > static_cast<uint32_t>(kMaxTileZoom))
	{
		return false;
	}

	const uint32_t tileCount = 1u << z;
	if (x >= tileCount || y >= tileCount)
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
		TF3D_LOG_WARN("Invalid DEM tile coordinates: ({}, {}, {})", x, y, z);
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
	auto path = fmt::vformat(m_TerrainRGBDataCacheFileFormat, fmt::make_format_args(x, y, z));

	// check is the texture is downloading
	if (std::find(m_TextureDownloadQueue.begin(), m_TextureDownloadQueue.end(), textureCacheKey) != m_TextureDownloadQueue.end())
	{
		return m_LoadingTexture;
	}

	auto now = std::chrono::steady_clock::now();
	auto retryIt = m_TileRetryAfter.find(textureCacheKey);
	if (retryIt != m_TileRetryAfter.end() && now < retryIt->second)
		return m_NullTexture;

	// if not, download it
	if (!PathExist(path))
	{
		if (m_APIKey.empty()) return m_NullTexture;
		if (!m_AllowTileRequestsThisUpdate) return m_LoadingTexture;
		if (static_cast<int32_t>(m_TextureDownloadQueue.size()) >= kMaxPendingTileRequests
			|| m_RequestsScheduledThisUpdate >= kMaxRequestsPerRefresh
			|| now < m_NextTileRequestTime)
			return m_LoadingTexture;

		++m_RequestsScheduledThisUpdate;
		m_NextTileRequestTime = now + std::chrono::milliseconds(kTileRequestIntervalMilliseconds);
		DownloadTerrainRGBTexture(textureCacheKey);
		return m_LoadingTexture;
	}

	int w = 1, h = 1, sz = 0;
	uint8_t* data = (uint8_t*)ReadBinaryFile(path, &sz);
	if (!data || sz <= 0)
	{
		delete[] data;
		std::remove(path.c_str());
		m_TileRetryAfter[textureCacheKey] = now + std::chrono::seconds(30);
		return m_NullTexture;
	}

	auto rgbData = WebPDecodeRGBA(data, sz, &w, &h);
	if (!rgbData || w <= 0 || h <= 0)
	{
		delete[] data;
		if (rgbData) free(rgbData);
		std::remove(path.c_str());
		m_TileRetryAfter[textureCacheKey] = now + std::chrono::seconds(30);
		TF3D_LOG_WARN("Ignoring invalid DEM tile cache entry: {}", path);
		return m_NullTexture;
	}

	auto texPtr = std::make_shared<Texture2D>(w, h);
	m_TextureCache[textureCacheKey] = texPtr;
	texPtr->SetData(rgbData, 0, true);
	delete[] data;
	free(rgbData);
	m_TileRetryAfter.erase(textureCacheKey);

	return texPtr;
}

void DEMBaseShapeGenerator::DownloadTerrainRGBTexture(TextureCacheKey key)
{
	m_TextureDownloadQueue.push_back(key);
	auto downloadWorker = [this, key]()
	{
		auto [x, y, z] = key;
		auto path = fmt::vformat(m_TerrainRGBDataCacheFileFormat, fmt::make_format_args(x, y, z));
		auto temporaryPath = path + ".part";
		auto urlPath = fmt::vformat(m_APIPathURLFormat, fmt::make_format_args(z, x, y, m_APIKey));
		std::remove(temporaryPath.c_str());
		DownloadFile(m_APIHostURL, urlPath, temporaryPath);
		if (PathExist(temporaryPath))
		{
			std::remove(path.c_str());
			std::rename(temporaryPath.c_str(), path.c_str());
		}
	};
	m_AppState->jobSystem->AddFunctionWorker(downloadWorker)->onComplete = [this, key](auto*)->void
	{
		auto it = std::find(m_TextureDownloadQueue.begin(), m_TextureDownloadQueue.end(), key);
		if (it != m_TextureDownloadQueue.end()) m_TextureDownloadQueue.erase(it);
		auto [x, y, z] = key;
		auto path = fmt::vformat(m_TerrainRGBDataCacheFileFormat, fmt::make_format_args(x, y, z));
		if (!PathExist(path))
			m_TileRetryAfter[key] = std::chrono::steady_clock::now() + std::chrono::seconds(30);
		m_RequireUpdation = true;
		m_AppState->eventManager->RaiseEvent("ForceUpdate", "ForceUpdate");
	};
}
