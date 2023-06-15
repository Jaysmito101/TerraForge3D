#pragma once

#include <json.hpp>
#include "Base/Base.h"
#include "Utils/Utils.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Exporters/Serializer.h"
#include "Misc/CustomInspector.h"

#define BASE_SHAPE_UI_PROPERTY(x) m_RequireUpdation = x || m_RequireUpdation

typedef std::tuple<uint32_t, uint32_t, uint32_t> TextureCacheKey;

template <>
struct std::hash<TextureCacheKey>
{
	std::size_t operator()(const TextureCacheKey& k) const
	{
		using std::size_t;
		using std::hash;
		using std::string;
		return ((hash<uint32_t>()(std::get<0>(k))
			^ (hash<uint32_t>()(std::get<1>(k)) << 1)) >> 1)
			^ (hash<uint32_t>()(std::get<2>(k)) << 1);
	}
};


class ApplicationState;

class DEMBaseShapeGenerator
{
public:
	DEMBaseShapeGenerator(ApplicationState* appState);
	~DEMBaseShapeGenerator();

	bool ShowSettings();
	void Update(GeneratorData* buffer, GeneratorTexture* seedTexture);

	void Load(SerializerNode data);
	SerializerNode Save();

	inline bool RequireUpdation() const { return m_RequireUpdation; }
	inline bool HasTileLoaded(uint32_t x, uint32_t y, uint32_t z) const { return m_TextureCache.find(TextureCacheKey(x, y, z)) != m_TextureCache.end(); }
	inline std::shared_ptr<Texture2D> GetTile(uint32_t x, uint32_t y, uint32_t z) { if (!HasTileLoaded(x, y, z)) return LoadTile(x, y, z); return m_TextureCache.at(TextureCacheKey(x, y, z)); }
	
	static bool IsTileValid(uint32_t x, uint32_t y, uint32_t z);

	std::shared_ptr<Texture2D> LoadTile(uint32_t x, uint32_t y, uint32_t z);

private:
	void DownloadTerrainRGBTexture(TextureCacheKey key);

private:
	ApplicationState* m_AppState = nullptr;
	int32_t m_ZoomResolution = 0;
	float m_ZoomOnMap = 1.0f;
	float m_MapStrength = 1.0f;
	float m_CalculationTime = 0.0f;
	int m_TilesUsingCount = 0;
	glm::vec2 m_MapCenter = glm::vec2(0.0f);
	
	
	bool m_RequireUpdation = true;
	std::shared_ptr<GeneratorTexture> m_MapVisualzeTexture;
	std::shared_ptr<Texture2D> m_LoadingTexture;
	std::shared_ptr<Texture2D> m_NullTexture;
	std::vector<TextureCacheKey> m_TextureDownloadQueue;
	std::shared_ptr<ComputeShader> m_Shader;


	std::unordered_map<TextureCacheKey, std::shared_ptr<Texture2D>> m_TextureCache;
	std::string m_APIKey = "";
	char m_APIKeyInput[1024];
	std::string m_APIKeyConfigPath;
	std::string m_APIHostURL;
	std::string m_APIPathURLFormat;
	std::string m_TerrainRGBDataCacheDir;
	std::string m_TerrainRGBDataCacheFileFormat;
};