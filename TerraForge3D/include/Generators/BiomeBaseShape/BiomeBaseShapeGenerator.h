#pragma once

#include <json/json.hpp>
#include "Base/Base.h"
#include "Utils/Utils.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"

#define BASE_SHAPE_UI_PROPERTY(x) m_RequireUpdation = x || m_RequireUpdation


static bool s_TempBool = false;

class ApplicationState;

class BiomeBaseShapeGenerator
{
public:
	BiomeBaseShapeGenerator(ApplicationState* appState, const std::string& name) : m_AppState(appState), m_Name(name) { m_ID = GenerateId(8); }
	virtual ~BiomeBaseShapeGenerator() = default;
	virtual bool ShowSettings() = 0;
	virtual void Update(GeneratorData* buffer, GeneratorTexture* seedTexture) = 0;
	virtual nlohmann::json OnSave() = 0;
	virtual void OnLoad(nlohmann::json data) = 0;
	inline nlohmann::json Save() { return OnSave(); }
	void Load(nlohmann::json data) { OnLoad(data); }

	inline const std::string& GetName() const { return m_Name; }

protected:
	ComputeShader* m_Shader = nullptr;
	ApplicationState* m_AppState = nullptr;
	std::string m_Name = "";
	std::string m_ID = "";
	bool m_RequireUpdation = true;
};