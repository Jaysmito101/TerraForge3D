#pragma once

#include <json.hpp>
#include "Base/Base.h"
#include "Utils/Utils.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Exporters/Serializer.h"
#include "Misc/CustomInspector.h"

#define BASE_SHAPE_UI_PROPERTY(x) m_RequireUpdation = x || m_RequireUpdation

static bool s_TempBool = false;

class ApplicationState;

class BiomeBaseShapeGenerator
{
public:
	BiomeBaseShapeGenerator(ApplicationState* appState);
	~BiomeBaseShapeGenerator();
	bool LoadConfig(const std::string& config);
	bool ShowSettings();
	void Update(GeneratorData* buffer, GeneratorTexture* seedTexture);
	void Load(SerializerNode data);
	SerializerNode Save();
	
	inline const std::string& GetName() const { return m_Name; }
	inline const std::string& GetID() const { return m_ID; }
	inline const std::string& GetSource() const { return m_Source; }
	inline bool RequireUpdation() const { return m_RequireUpdation; }

private:
	nlohmann::json ParseData(const std::string& config);
	bool LoadInspectorFromConfig(const nlohmann::json& metaData);
	std::string BuildShaderSource();

protected:
	ApplicationState* m_AppState = nullptr;
	std::shared_ptr<ComputeShader> m_Shader;
	std::shared_ptr<CustomInspector> m_Inspector;
	std::string m_Name = "";
	std::string m_ID = "";
	std::string m_Source = "";
	bool m_RequireUpdation = true;
};