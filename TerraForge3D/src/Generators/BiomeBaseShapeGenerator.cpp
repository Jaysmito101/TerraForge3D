#include "Generators/BiomeBaseShapeGenerator.h"
#include "Data/ApplicationState.h"

#define LOAD_VALUE_FROM_CONFIG(jsonName, varName, jsonValueName, defaultValue) if (jsonName.contains(jsonValueName)) varName = jsonName[jsonValueName].get<decltype(varName)>(); else varName = defaultValue;

BiomeBaseShapeGenerator::BiomeBaseShapeGenerator(ApplicationState* appState)
{
	m_RequireUpdation = false;
	m_AppState = appState;
	m_ID = GenerateId(8);
	m_Source = "";
	m_Name = "";
	m_Inspector = std::make_shared<CustomInspector>();
}

BiomeBaseShapeGenerator::~BiomeBaseShapeGenerator()
{
}

bool BiomeBaseShapeGenerator::ShowSettings()
{
	static bool ttp = false;
	ImGui::PushID(m_ID.c_str());
	BIOME_UI_PROPERTY(m_Inspector->Render());
	ImGui::PopID();
	return m_RequireUpdation;
}

void BiomeBaseShapeGenerator::Update(GeneratorData* buffer, GeneratorTexture* seedTexture)
{
	buffer->Bind(0);
	m_Shader->Bind();
	const auto& values = m_Inspector->GetValues();
	int textureSlot = 4;
	for (const auto& [valueName, uniformValue] : values)
	{
		std::string uniformName = std::string("u_") + valueName;
		switch (uniformValue.GetType())
		{
		case CustomInspectorValueType_Int: m_Shader->SetUniform1i(uniformName, uniformValue.GetInt()); break;
		case CustomInspectorValueType_Float	 : m_Shader->SetUniform1f(uniformName, uniformValue.GetFloat()); break;
		case CustomInspectorValueType_Bool	 : m_Shader->SetUniform1i(uniformName, uniformValue.GetInt()); break;
		case CustomInspectorValueType_Vector2: m_Shader->SetUniform2f(uniformName, uniformValue.GetVector2()); break;
		case CustomInspectorValueType_Vector3: m_Shader->SetUniform3f(uniformName, uniformValue.GetVector3()); break;
		case CustomInspectorValueType_Vector4: m_Shader->SetUniform4f(uniformName, uniformValue.GetVector4());  break;
		case CustomInspectorValueType_Texture: if(uniformValue.GetTexture()) m_Shader->SetUniform1i(uniformName, uniformValue.GetTexture()->Bind(textureSlot++)); break;
		case CustomInspectorValueType_String: // for future	
		case CustomInspectorValueType_Unknown:	
		default:
			break;
		}
	}
	m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
	m_Shader->SetUniform1i("u_UseSeedTexture", seedTexture != nullptr ? 1 : 0);
	if (seedTexture) m_Shader->SetUniform1i("u_SeedTexture", seedTexture->Bind(1));
	const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
	m_Shader->Dispatch(m_AppState->mainMap.tileResolution / workgroupSize, m_AppState->mainMap.tileResolution / workgroupSize, 1);
	m_Shader->SetMemoryBarrier();
	m_RequireUpdation = false;
}

void BiomeBaseShapeGenerator::Load(SerializerNode data)
{
	m_Name = data->GetString("Name", "Default Name");
	m_ID = data->GetString("ID", GenerateId(8));
	m_Source = data->GetString("Source");
	auto inspector = data->GetChildNode("Inspector");
	if (inspector) m_Inspector->Load(inspector);
	else Log("Failed to load inspector data for generator: " + m_Name);
	m_Shader = std::make_shared<ComputeShader>(BuildShaderSource());
	m_RequireUpdation = true;
}

SerializerNode BiomeBaseShapeGenerator::Save()
{
	SerializerNode node = CreateSerializerNode();
	node->SetString("Name", m_Name);
	node->SetString("ID", m_ID);
	node->SetString("Source", m_Source);
	node->SetChildNode("Inspector", m_Inspector->Save());
	return node;
}

nlohmann::json BiomeBaseShapeGenerator::ParseData(const std::string& config)
{
	const std::string seperatorLine = "// CODE";
	std::string metaDataString = config.substr(0, config.find(seperatorLine));
	m_Source = config.substr(config.find(seperatorLine) + seperatorLine.size() + 1);
	nlohmann::json metaData;
	try
	{
		metaData = nlohmann::json::parse(metaDataString);
		metaData["HasError"] = false;
	}
	catch (nlohmann::json::parse_error ex)
	{
		metaData["HasError"] = true;
		std::string error = ex.what();
		metaData["ErrorMessage"] = error;
	}
	return metaData;
}

bool BiomeBaseShapeGenerator::LoadInspectorFromConfig(const nlohmann::json& config)
{
	m_Inspector->Clear();
	LOAD_VALUE_FROM_CONFIG(config, m_Name, "Name", "Unnamed");
	if (config.contains("Params"))
	{
		std::string widgetTypeName, widgetLabel;
		const auto& parametersArray = config["Params"];
		for (size_t i = 0; i < parametersArray.size(); i++)
		{
			const auto& parameter = parametersArray[i];
			const auto& value = m_Inspector->AddVairableFromConfig(parameter);
			LOAD_VALUE_FROM_CONFIG(parameter, widgetTypeName, "Widget", "Input");
			LOAD_VALUE_FROM_CONFIG(parameter, widgetLabel, "Label", value.GetName());
			auto& widget = m_Inspector->AddWidgetFromString(widgetLabel, widgetTypeName, value.GetName());
			if (parameter.contains("Sensitivity")) widget.SetSpeed(parameter["Sensitivity"].get<float>());
			if (parameter.contains("Options")) widget.SetDropdownOptions(parameter["Options"].get<std::vector<std::string>>());
			if (parameter.contains("Constraints")) widget.SetConstraints(parameter["Constraints"][0].get<float>(), parameter["Constraints"][1].get<float>(), parameter["Constraints"][2].get<float>(), parameter["Constraints"][3].get<float>());
		    if (parameter.contains("Tooltip")) widget.SetTooltip(parameter["Tooltip"].get<std::string>());
			if (parameter.contains("Conditional"))
			{
				int32_t conditionValue = 1;
				LOAD_VALUE_FROM_CONFIG(parameter, conditionValue, "ConditionalValue", 1);
				widget.SetRenderOnCondition(parameter["Conditional"].get<std::string>(), conditionValue);
			}
		}
	}
	else
	{
		m_Inspector->AddTextWidget("No parameters available");
	}

	return true;
}

std::string BiomeBaseShapeGenerator::BuildShaderSource()
{
	std::string source = "";
	source += "#version 430 core\n\n";
	source += "// work group size\n";
	source += "layout (local_size_x = " + std::to_string(m_AppState->constants.gpuWorkgroupSize) + ", local_size_y = " + std::to_string(m_AppState->constants.gpuWorkgroupSize) + ", local_size_z = 1) in;\n\n";
	source += "// output data buffer\n";
	source += "layout(std430, binding = 0) buffer DataBuffer\n";
	source += "{\n\tfloat data[];\n};\n\n";
	source += "// uniform variables\n";
	source += "// default uniforms\n";
	source += "uniform int u_Resolution;\n";
	source += "uniform bool u_UseSeedTexture;\n";
	source += "uniform sampler2D u_SeedTexture;\n";
	source += "// custom uniforms\n";
	const auto& values = m_Inspector->GetValues();
	for (const auto& [valueName, uniformValue] : values)
	{
		switch (uniformValue.GetType())
		{
		case CustomInspectorValueType_Int: source += "uniform int u_" + valueName + " = 0;\n"; break;
		case CustomInspectorValueType_Float: source += "uniform float u_" + valueName + " = 0.0f;\n"; break;
		case CustomInspectorValueType_Bool: source += "uniform bool u_" + valueName + " = false;"; break;
		case CustomInspectorValueType_Vector2: source += "uniform vec2 u_" + valueName + " = vec2(0.0f);\n"; break;
		case CustomInspectorValueType_Vector3: source += "uniform vec3 u_" + valueName + " = vec3(0.0f);\n"; break; 
		case CustomInspectorValueType_Vector4: source += "uniform vec4 u_" + valueName + " = vec4(0.0f);\n"; break;
		case CustomInspectorValueType_Texture: source += "uniform sampler2D u_" + valueName + ";\n"; break;
		case CustomInspectorValueType_String: source += "// uniform string u_" + valueName + ";\n"; break;
		case CustomInspectorValueType_Unknown: source += "// uniform unknownType u_" + valueName + ";\n"; break;
		default:
			break;
		}
	}
	source += "\n\n";
	source += "// utility function to convert pixel coord to\n";
	source += "// offset inside the output buffer\n";
	source += "uint PixelCoordToDataOffset(uint x, uint y)\n";
	source += "{\n\treturn y * u_Resolution + x;\n}\n\n";
	source += "// body\n";
	source += m_Source;
	source += "\n\n";
	source += "// main\n";
	source += "void main()\n{\n";
	source += "\tuvec2 offsetv2 = gl_GlobalInvocationID.xy;\n";
	source += "\tuint offset = PixelCoordToDataOffset(offsetv2.x, offsetv2.y);\n";
	source += "\tvec2 uv = offsetv2 / float(u_Resolution);\n";
	source += "\tvec3 seed = vec3(uv * 2.0f - vec2(1.0f), 0.0f);\n";
	source += "\tif (u_UseSeedTexture)\n\t{\n\t\tseed = texture(u_SeedTexture, uv).rgb; \n\t}\n";
	source += "\tdata[offset] = evaluateBaseShape(uv, seed);\n}\n";
	return source;
}

bool BiomeBaseShapeGenerator::LoadConfig(const std::string& config)
{
	auto metaData = ParseData(config);
	if (metaData["HasError"].get<bool>())
	{
		Log("Error in loading base shape generator config : " + metaData["ErrorMessage"].get<std::string>());
		return false;
	}
	if (!LoadInspectorFromConfig(metaData)) return false;
	m_Shader = std::make_shared<ComputeShader>(BuildShaderSource());
	return true;
}