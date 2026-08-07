#include "Generators/BiomeBaseShapeGenerator.h"
#include "Data/ApplicationState.h"
#include "Generators/NoiseAlgorithmConfig.h"
#include "Profiler.h"

namespace tf3d::generators
{

    BiomeBaseShapeGenerator::BiomeBaseShapeGenerator(ApplicationState *appState)
    {
        m_RequireUpdation = false;
        m_AppState        = appState;
        m_ID              = GenerateId(8);
        m_Source          = "";
        m_ShaderPath      = "";
        m_Name            = "";
        m_Inspector       = std::make_shared<CustomInspector>();
        std::string catalogError;
        if (!m_NoiseAlgorithms.LoadFromFile(NoiseAlgorithmCatalog::IndexPath(m_AppState->constants.shadersDir), &catalogError)) {
            TF3D_LOG_ERROR("{}", catalogError);
        }
    }

    BiomeBaseShapeGenerator::~BiomeBaseShapeGenerator()
    {
    }

    bool BiomeBaseShapeGenerator::ShowSettings()
    {
        ImGui::PushID(m_ID.c_str());
        if (!m_Description.empty() && m_Inspector->GetDescription().empty()) {
            ImGui::TextWrapped("%s", m_Description.c_str());
            ImGui::Separator();
        }
        BIOME_UI_PROPERTY(m_Inspector->Render());
        ImGui::PopID();
        return m_RequireUpdation;
    }

    void BiomeBaseShapeGenerator::Update(GeneratorData *buffer, GeneratorTexture *seedTexture)
    {
        TF3D_PROFILE_SCOPE_LAZY_DOMAIN(std::string("generation/base-shape/") + m_Name, PerformanceMonitor::Domain::Generation);
        buffer->Bind(0);
        m_Shader->Bind();
        int textureSlot = 4;
        m_Inspector->ForEachValue([&](const auto &valueName, const auto &uniformValue) {
            std::string uniformName = std::string("u_") + valueName;
            switch (uniformValue.GetType()) {
                case CustomInspectorValueType::Int:
                    m_Shader->SetUniform1i(uniformName, uniformValue.template Get<int32_t>());
                    break;
                case CustomInspectorValueType::Float:
                    m_Shader->SetUniform1f(uniformName, uniformValue.template Get<float>());
                    break;
                case CustomInspectorValueType::Bool:
                    m_Shader->SetUniform1i(uniformName, uniformValue.template Get<bool>() ? 1 : 0);
                    break;
                case CustomInspectorValueType::Vector2:
                    m_Shader->SetUniform2f(uniformName, uniformValue.template Get<glm::vec2>());
                    break;
                case CustomInspectorValueType::Vector3:
                    m_Shader->SetUniform3f(uniformName, uniformValue.template Get<glm::vec3>());
                    break;
                case CustomInspectorValueType::Vector4:
                    m_Shader->SetUniform4f(uniformName, uniformValue.template Get<glm::vec4>());
                    break;
                case CustomInspectorValueType::Texture:
                    if (const auto texture = uniformValue.template Get<std::shared_ptr<Texture2D>>())
                        m_Shader->SetUniform1i(uniformName, texture->Bind(textureSlot++));
                    break;
                case CustomInspectorValueType::String: // for future
                case CustomInspectorValueType::Unknown:
                default:
                    break;
            }
        });
        m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
        m_Shader->SetUniform1i("u_UseSeedTexture", seedTexture != nullptr ? 1 : 0);
        if (seedTexture)
            m_Shader->SetUniform1i("u_SeedTexture", seedTexture->Bind(1));
        const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
        const auto dispatchSize  = m_AppState->mainMap.tileResolution / workgroupSize;
        TF3D_PROFILE_GPU_SCOPE("generation/base-shape/gpu");
        TF3D_PROFILE_VALUE_DOMAIN("generation/base-shape/dispatch", dispatchSize, dispatchSize, 1,
                                  PerformanceMonitor::Domain::Generation);
        m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
        m_Shader->SetMemoryBarrier();
        m_RequireUpdation = false;
    }

    void BiomeBaseShapeGenerator::Load(SerializerNode data)
    {
        m_Name         = data->Get<std::string>("Name", "Default Name");
        m_ID           = data->Get<std::string>("ID", GenerateId(8));
        m_Description  = data->Get<std::string>("Description", m_Description);
        m_Source       = data->Get<std::string>("Source");
        m_ShaderPath   = data->Get<std::string>("ShaderPath", "");
        auto inspector = data->Get<SerializerNode>("Inspector");
        if (inspector)
            m_Inspector->LoadState(inspector);
        else
            TF3D_LOG_ERROR("Failed to load inspector data for generator '{}'", m_Name);
        // m_Shader = std::make_shared<ComputeShader>(BuildShaderSource());
        m_Shader          = m_AppState->resourceManager->GetComputeShader("BaseShapeGen_" + m_Name, BuildShaderSource());
        m_RequireUpdation = true;
    }

    SerializerNode BiomeBaseShapeGenerator::Save()
    {
        SerializerNode node = CreateSerializerNode();
        node->Set("Name", m_Name);
        node->Set("ID", m_ID);
        node->Set("Description", m_Description);
        node->Set("Source", m_Source);
        node->Set("ShaderPath", m_ShaderPath);
        node->Set("Inspector", m_Inspector->SaveState());
        return node;
    }

    nlohmann::json BiomeBaseShapeGenerator::ParseData(const std::string &config)
    {
        const std::string seperatorLine = "// CODE";
        std::string metaDataString      = config.substr(0, config.find(seperatorLine));
        m_Source                        = config.substr(config.find(seperatorLine) + seperatorLine.size() + 1);
        nlohmann::json metaData;
        try {
            metaData             = nlohmann::json::parse(metaDataString);
            metaData["HasError"] = false;
        } catch (nlohmann::json::parse_error ex) {
            metaData["HasError"]     = true;
            std::string error        = ex.what();
            metaData["ErrorMessage"] = error;
        }
        return metaData;
    }

    bool BiomeBaseShapeGenerator::LoadInspectorFromConfig(const nlohmann::json &config)
    {
        m_Name               = config.value("Name", "Unnamed");
        auto inspectorConfig = config;
        if (!ApplyNoiseAlgorithmMetadata(inspectorConfig, m_NoiseAlgorithms)) {
            TF3D_LOG_ERROR("Failed to apply noise algorithm metadata for base shape '{}'.", m_Name);
            return false;
        }
        if (!m_Inspector->LoadConfig(inspectorConfig))
            return false;
        return true;
    }

    std::string BiomeBaseShapeGenerator::BuildShaderSource()
    {
        std::string source = "";
        source += "#version 430 core\n\n";
        source += m_NoiseAlgorithms.ShaderDefines();
        source += "\n";
        source += "// work group size\n";
        source += "layout (local_size_x = " + std::to_string(m_AppState->constants.gpuWorkgroupSize) + ", local_size_y = " + std::to_string(m_AppState->constants.gpuWorkgroupSize) + ", local_size_z = 1) in;\n\n";
        source += "// output field texture\n";
        source += "layout(TF3D_FIELD_FORMAT, binding = 0) writeonly uniform image2D DataTexture;\n\n";
        source += "// uniform variables\n";
        source += "// default uniforms\n";
        source += "uniform int u_Resolution;\n";
        source += "uniform bool u_UseSeedTexture;\n";
        source += "uniform sampler2D u_SeedTexture;\n";
        source += "// custom uniforms\n";
        m_Inspector->ForEachValue([&](const auto &valueName, const auto &uniformValue) {
            switch (uniformValue.GetType()) {
                case CustomInspectorValueType::Int:
                    source += "uniform int u_" + valueName + " = 0;\n";
                    break;
                case CustomInspectorValueType::Float:
                    source += "uniform float u_" + valueName + " = 0.0f;\n";
                    break;
                case CustomInspectorValueType::Bool:
                    source += "uniform bool u_" + valueName + " = false;";
                    break;
                case CustomInspectorValueType::Vector2:
                    source += "uniform vec2 u_" + valueName + " = vec2(0.0f);\n";
                    break;
                case CustomInspectorValueType::Vector3:
                    source += "uniform vec3 u_" + valueName + " = vec3(0.0f);\n";
                    break;
                case CustomInspectorValueType::Vector4:
                    source += "uniform vec4 u_" + valueName + " = vec4(0.0f);\n";
                    break;
                case CustomInspectorValueType::Texture:
                    source += "uniform sampler2D u_" + valueName + ";\n";
                    break;
                case CustomInspectorValueType::String:
                    source += "// uniform string u_" + valueName + ";\n";
                    break;
                case CustomInspectorValueType::Unknown:
                    source += "// uniform unknownType u_" + valueName + ";\n";
                    break;
                default:
                    break;
            }
        });
        source += "\n\n";
        source += "// utility function to convert pixel coord to\n";
        source += "// offset inside the output buffer\n";
        source += "uint PixelCoordToDataOffset(uint x, uint y)\n";
        source += "{\n\treturn y * u_Resolution + x;\n}\n\n";
        source += "// body\n";
        bool includeSuccess          = false;
        const std::string shaderPath = m_ShaderPath.empty() ? "generation/base_shape/" + m_Name + ".glsl" : m_ShaderPath;
        const auto expandedSource    = m_AppState->resourceManager->PreprocessShaderSource(m_Source, shaderPath, &includeSuccess);
        if (includeSuccess)
            source += expandedSource;
        else
            source += m_Source;
        source += "\n\n";
        source += "// main\n";
        source += "void main()\n{\n";
        source += "\tuvec2 offsetv2 = gl_GlobalInvocationID.xy;\n";
        source += "\tif (offsetv2.x >= uint(u_Resolution) || offsetv2.y >= uint(u_Resolution)) return;\n";
        source += "\tivec2 pixelCoord = ivec2(offsetv2);\n";
        source += "\tvec2 uv = offsetv2 / float(u_Resolution);\n";
        source += "\tvec3 seed = vec3(uv * 2.0f - vec2(1.0f), 0.0f);\n";
        source += "\tif (u_UseSeedTexture)\n\t{\n\t\tseed = texture(u_SeedTexture, uv).rgb; \n\t}\n";
        source += "\timageStore(DataTexture, pixelCoord, vec4(evaluateBaseShape(uv, seed), 0.0, 0.0, 0.0));\n}\n";
        return source;
    }

    bool BiomeBaseShapeGenerator::LoadConfig(const std::string &config)
    {
        auto metaData = ParseData(config);
        if (metaData["HasError"].get<bool>()) {
            TF3D_LOG_ERROR("Failed to load base-shape generator configuration: {}", metaData["ErrorMessage"].get<std::string>());
            return false;
        }
        const std::string source = m_Source;
        return LoadConfig(metaData, source, "generation/base_shape/" + m_Name + ".glsl");
    }

    bool BiomeBaseShapeGenerator::LoadConfig(const nlohmann::json &config, const std::string &source, const std::string &shaderPath)
    {
        if (!config.is_object()) {
            TF3D_LOG_ERROR("Failed to load base-shape generator: metadata is not an object.");
            return false;
        }
        m_ID          = config.value("ID", m_ID.empty() ? GenerateId(8) : m_ID);
        m_Description = config.value("Description", "");
        m_Source      = source;
        m_ShaderPath  = shaderPath;
        if (!LoadInspectorFromConfig(config))
            return false;
        m_Shader          = m_AppState->resourceManager->GetComputeShader("BaseShapeGen_" + m_ID, BuildShaderSource());
        m_RequireUpdation = true;
        return m_Shader.has_value();
    }

} // namespace tf3d::generators
