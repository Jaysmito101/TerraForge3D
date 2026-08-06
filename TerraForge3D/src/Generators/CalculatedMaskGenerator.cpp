#include "Generators/CalculatedMaskGenerator.h"
#include "Generators/NoiseAlgorithmConfig.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Utils/Utils.h"

namespace tf3d::generators
{

    namespace
    {
        bool IsValidShaderSymbol(const std::string &symbol)
        {
            if (symbol.empty())
                return false;
            for (size_t index = 0; index < symbol.size(); ++index) {
                const char character = symbol[index];
                const bool isLetter  = (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z');
                const bool isDigit   = character >= '0' && character <= '9';
                if (!(isLetter || isDigit || character == '_') || (index == 0 && isDigit))
                    return false;
            }
            return true;
        }

        std::string InjectMaskModeDefines(const std::string &source, const nlohmann::json &metadata)
        {
            std::string defines;
            for (const auto &type : metadata["Types"]) {
                defines += "#define " + type["ShaderSymbol"].get<std::string>() + " " +
                           std::to_string(type["Mode"].get<int>()) + "\n";
            }
            const size_t firstLineEnd = source.find('\n');
            if (firstLineEnd == std::string::npos)
                return source;
            return source.substr(0, firstLineEnd + 1) + defines + source.substr(firstLineEnd + 1);
        }
    } // namespace

    CalculatedMaskGenerator::CalculatedMaskGenerator(ApplicationState *state)
        : m_AppState(state)
    {
        std::string catalogError;
        if (!m_NoiseAlgorithms.LoadFromFile(NoiseAlgorithmCatalog::IndexPath(m_AppState->constants.shadersDir), &catalogError)) {
            TF3D_LOG_ERROR("{}", catalogError);
        } else {
            m_Settings.noiseAlgorithm = m_NoiseAlgorithms.DefaultValue();
        }
        m_Texture   = std::make_shared<GeneratorTexture>(m_Size, m_Size, GeneratorTextureStorage::R16);
        m_Inspector = std::make_shared<CustomInspector>();
        m_Inspector->SetShowResetButton(false);
        m_MetadataLoaded        = LoadMetadata();
        bool shaderSourceLoaded = false;
        const auto shaderSource = m_AppState->resourceManager->LoadShaderSource(
            "generation/utils/mask_preview", false, &shaderSourceLoaded);
        if (shaderSourceLoaded && m_NoiseAlgorithms.IsValid()) {
            const auto shaderSourceWithNoiseDefines = m_NoiseAlgorithms.InjectShaderDefines(shaderSource);
            const auto finalShaderSource            = m_MetadataLoaded
                                                          ? InjectMaskModeDefines(shaderSourceWithNoiseDefines, m_Metadata)
                                                          : shaderSourceWithNoiseDefines;
            m_Shader                                = m_AppState->resourceManager->GetComputeShader("CalculatedMaskPreview", finalShaderSource);
        }
        if (!m_MetadataLoaded) {
            TF3D_LOG_ERROR("Calculated mask metadata could not be loaded; calculated mask settings are unavailable.");
        }
    }

    CalculatedMaskGenerator::~CalculatedMaskGenerator()
    {
    }

    void CalculatedMaskGenerator::Resize(int size)
    {
        if (size <= 0)
            return;
        m_Size = size;
        m_Texture->Resize(size, size);
        m_Dirty = true;
    }

    void CalculatedMaskGenerator::Invalidate()
    {
        m_Dirty = true;
    }

    int CalculatedMaskGenerator::GetSelectedTypeIndex() const
    {
        if (m_Inspector == nullptr || !m_Inspector->HasVariable("MaskType"))
            return static_cast<int>(m_Settings.type);
        return glm::clamp(m_Inspector->GetVariable("MaskType").GetInt(), 0, static_cast<int>(CalculatedMaskType::Count) - 1);
    }

    bool CalculatedMaskGenerator::LoadInspectorForType(int typeIndex)
    {
        if (m_Inspector == nullptr || !m_Metadata.contains("Types") || !m_Metadata["Types"].is_array())
            return false;
        if (typeIndex < 0 || typeIndex >= static_cast<int>(CalculatedMaskType::Count) || typeIndex >= static_cast<int>(m_Metadata["Types"].size()))
            return false;

        const auto &types = m_Metadata["Types"];
        std::vector<std::string> typeNames;
        typeNames.reserve(types.size());
        for (const auto &type : types)
            typeNames.push_back(type.value("Name", type.value("ID", "Mask")));

        nlohmann::json config = types[typeIndex];
        config["Params"]      = nlohmann::json::array();
        auto source           = m_Metadata.value("Source", nlohmann::json::object());
        source["Default"]     = typeIndex;
        source["Options"]     = typeNames;
        config["Params"].push_back(source);
        if (types[typeIndex].contains("Params") && types[typeIndex]["Params"].is_array()) {
            for (const auto &parameter : types[typeIndex]["Params"])
                config["Params"].push_back(parameter);
        }
        if (m_Metadata.contains("Buttons"))
            config["Buttons"] = m_Metadata["Buttons"];
        if (!ApplyNoiseAlgorithmMetadata(config, m_NoiseAlgorithms))
            return false;

        if (!m_Inspector->LoadConfig(config))
            return false;
        m_Inspector->SetShowResetButton(false);
        SyncSettingsFromInspector();
        m_Settings.type       = static_cast<CalculatedMaskType>(typeIndex);
        m_Settings.shaderMode = types[typeIndex].value("Mode", typeIndex);
        return true;
    }

    bool CalculatedMaskGenerator::LoadMetadata()
    {
        if (m_AppState == nullptr)
            return false;
        const std::string metadataPath = m_AppState->constants.shadersDir + PATH_SEPARATOR +
                                         "generation" PATH_SEPARATOR "utils" PATH_SEPARATOR "calculated_mask.json";
        bool loaded               = false;
        const auto metadataSource = ReadShaderSourceFile(metadataPath, &loaded);
        if (!loaded)
            return false;
        try {
            m_Metadata = nlohmann::json::parse(metadataSource);
        } catch (const nlohmann::json::parse_error &exception) {
            TF3D_LOG_ERROR("Failed to parse calculated mask metadata '{}': {}", metadataPath, exception.what());
            return false;
        }
        if (!m_Metadata.contains("Types") || !m_Metadata["Types"].is_array() ||
            m_Metadata["Types"].size() != static_cast<size_t>(CalculatedMaskType::Count)) {
            TF3D_LOG_ERROR("Calculated mask metadata '{}' must define exactly {} mask types.", metadataPath, static_cast<int>(CalculatedMaskType::Count));
            return false;
        }
        for (size_t typeIndex = 0; typeIndex < m_Metadata["Types"].size(); ++typeIndex) {
            const auto &type               = m_Metadata["Types"][typeIndex];
            const std::string id           = type.value("ID", "");
            const std::string shaderSymbol = type.value("ShaderSymbol", "");
            if (id.empty() || shaderSymbol.empty() || !IsValidShaderSymbol(shaderSymbol) || !type.contains("Mode") ||
                !type["Mode"].is_number_integer() || type["Mode"].get<int>() < 0) {
                TF3D_LOG_ERROR("Calculated mask metadata entry {} must define a valid ID, integer Mode, and ShaderSymbol.", typeIndex);
                return false;
            }
            for (size_t previousIndex = 0; previousIndex < typeIndex; ++previousIndex) {
                const auto &previous = m_Metadata["Types"][previousIndex];
                if (id == previous.value("ID", "") || type["Mode"].get<int>() == previous.value("Mode", -1) ||
                    shaderSymbol == previous.value("ShaderSymbol", "")) {
                    TF3D_LOG_ERROR("Calculated mask metadata contains duplicate IDs, modes, or shader symbols.");
                    return false;
                }
            }
        }
        return LoadInspectorForType(static_cast<int>(m_Settings.type));
    }

    int CalculatedMaskGenerator::FindTypeIndexByID(const std::string &id) const
    {
        if (id.empty() || !m_Metadata.contains("Types") || !m_Metadata["Types"].is_array())
            return -1;
        for (size_t typeIndex = 0; typeIndex < m_Metadata["Types"].size(); ++typeIndex) {
            if (m_Metadata["Types"][typeIndex].value("ID", "") == id)
                return static_cast<int>(typeIndex);
        }
        return -1;
    }

    void CalculatedMaskGenerator::SyncSettingsFromInspector()
    {
        if (m_Inspector == nullptr)
            return;
        if (m_Inspector->HasVariable("Range")) {
            const auto &range = m_Inspector->GetVariable("Range");
            if (range.GetType() == CustomInspectorValueType_Vector2) {
                const glm::vec2 value = range.GetVector2();
                m_Settings.minimum    = std::min(value.x, value.y);
                m_Settings.maximum    = std::max(value.x, value.y);
                if (value.x > value.y)
                    m_Inspector->GetVariable("Range").SetVector2(glm::vec2(m_Settings.minimum, m_Settings.maximum));
            } else {
                m_Settings.minimum = range.GetFloat();
                m_Settings.maximum = 1.0f;
            }
        }
        if (m_Inspector->HasVariable("EdgeFeather"))
            m_Settings.softness = m_Inspector->GetVariable("EdgeFeather").GetFloat();
        if (m_Inspector->HasVariable("EdgeSoftness"))
            m_Settings.softness = m_Inspector->GetVariable("EdgeSoftness").GetFloat();
        if (m_Inspector->HasVariable("Direction"))
            m_Settings.angle = m_Inspector->GetVariable("Direction").GetFloat();
        if (m_Inspector->HasVariable("DirectionWidth"))
            m_Settings.angleWidth = m_Inspector->GetVariable("DirectionWidth").GetFloat();
        if (m_Inspector->HasVariable("Scale"))
            m_Settings.scale = m_Inspector->GetVariable("Scale").GetFloat();
        if (m_Inspector->HasVariable("Seed"))
            m_Settings.seed = m_Inspector->GetVariable("Seed").GetFloat();
        if (m_Inspector->HasVariable("NoiseAlgorithm"))
            m_Settings.noiseAlgorithm = m_Inspector->GetVariable("NoiseAlgorithm").GetInt();
        if (m_Inspector->HasVariable("NoiseOctaves"))
            m_Settings.noiseOctaves = m_Inspector->GetVariable("NoiseOctaves").GetInt();
        if (m_Inspector->HasVariable("NoiseLacunarity"))
            m_Settings.noiseLacunarity = m_Inspector->GetVariable("NoiseLacunarity").GetFloat();
        if (m_Inspector->HasVariable("NoisePersistence"))
            m_Settings.noisePersistence = m_Inspector->GetVariable("NoisePersistence").GetFloat();
        if (m_Inspector->HasVariable("NoiseWarp"))
            m_Settings.noiseWarp = m_Inspector->GetVariable("NoiseWarp").GetFloat();
        if (m_Inspector->HasVariable("NoiseJitter"))
            m_Settings.noiseJitter = m_Inspector->GetVariable("NoiseJitter").GetFloat();
        if (m_Inspector->HasVariable("SampleRadius"))
            m_Settings.sampleRadius = m_Inspector->GetVariable("SampleRadius").GetFloat();
        if (m_Inspector->HasVariable("CurvatureSensitivity"))
            m_Settings.curvatureScale = m_Inspector->GetVariable("CurvatureSensitivity").GetFloat();
        if (m_Inspector->HasVariable("CavitySensitivity"))
            m_Settings.cavityScale = m_Inspector->GetVariable("CavitySensitivity").GetFloat();
        if (m_Inspector->HasVariable("SeaLevel"))
            m_Settings.seaLevel = m_Inspector->GetVariable("SeaLevel").GetFloat();
        if (m_Inspector->HasVariable("SpiralArms"))
            m_Settings.spiralArms = m_Inspector->GetVariable("SpiralArms").GetFloat();
        if (m_Inspector->HasVariable("SpiralTurns"))
            m_Settings.spiralTurns = m_Inspector->GetVariable("SpiralTurns").GetFloat();
        if (m_Inspector->HasVariable("SpiralThickness"))
            m_Settings.spiralThickness = m_Inspector->GetVariable("SpiralThickness").GetFloat();
        if (m_Inspector->HasVariable("SpiralSoftness"))
            m_Settings.spiralSoftness = m_Inspector->GetVariable("SpiralSoftness").GetFloat();
        if (m_Inspector->HasVariable("SpiralRotation"))
            m_Settings.spiralRotation = m_Inspector->GetVariable("SpiralRotation").GetFloat();
        if (m_Inspector->HasVariable("SpiralInvert"))
            m_Settings.spiralInvert = m_Inspector->GetVariable("SpiralInvert").GetBool();
        if (m_Inspector->HasVariable("GridCells"))
            m_Settings.gridCells = m_Inspector->GetVariable("GridCells").GetFloat();
        if (m_Inspector->HasVariable("GridThickness"))
            m_Settings.gridThickness = m_Inspector->GetVariable("GridThickness").GetFloat();
        if (m_Inspector->HasVariable("GridSoftness"))
            m_Settings.gridSoftness = m_Inspector->GetVariable("GridSoftness").GetFloat();
        if (m_Inspector->HasVariable("GridRotation"))
            m_Settings.gridRotation = m_Inspector->GetVariable("GridRotation").GetFloat();
        if (m_Inspector->HasVariable("GridInvert"))
            m_Settings.gridInvert = m_Inspector->GetVariable("GridInvert").GetBool();
        if (m_Inspector->HasVariable("DotCells"))
            m_Settings.dotCells = m_Inspector->GetVariable("DotCells").GetFloat();
        if (m_Inspector->HasVariable("DotRadius"))
            m_Settings.dotRadius = m_Inspector->GetVariable("DotRadius").GetFloat();
        if (m_Inspector->HasVariable("DotSoftness"))
            m_Settings.dotSoftness = m_Inspector->GetVariable("DotSoftness").GetFloat();
        if (m_Inspector->HasVariable("DotRotation"))
            m_Settings.dotRotation = m_Inspector->GetVariable("DotRotation").GetFloat();
        if (m_Inspector->HasVariable("DotInvert"))
            m_Settings.dotInvert = m_Inspector->GetVariable("DotInvert").GetBool();
        if (m_Inspector->HasVariable("Center"))
            m_Settings.center = m_Inspector->GetVariable("Center").GetVector2();
        if (m_Inspector->HasVariable("UsePath"))
            m_Settings.usePath = m_Inspector->GetVariable("UsePath").GetBool();
        if (m_Inspector->HasVariable("Feature"))
            m_Settings.selectValleys = m_Inspector->GetVariable("Feature").GetInt() == 1;
        if (m_Inspector->HasVariable("Path")) {
            const auto &path          = m_Inspector->GetVariable("Path");
            m_Settings.pathPoints     = path.GetPathPoints();
            m_Settings.pathPointCount = glm::clamp(path.GetPathPointCount(), 2, CalculatedMaskSettings::MaxPathPoints);
            m_Settings.pathEnd        = m_Settings.pathPoints[m_Settings.pathPointCount - 1];
        }
        if (m_Inspector->HasVariable("MaskType")) {
            const int typeIndex = GetSelectedTypeIndex();
            m_Settings.type     = static_cast<CalculatedMaskType>(typeIndex);
            if (m_Metadata.contains("Types") && typeIndex < static_cast<int>(m_Metadata["Types"].size()))
                m_Settings.shaderMode = m_Metadata["Types"][typeIndex].value("Mode", typeIndex);
        }
    }

    bool CalculatedMaskGenerator::ShowSettings()
    {
        if (!m_MetadataLoaded || m_Inspector == nullptr)
            return false;
        bool changed                          = m_Inspector->Render();
        const std::string lastAction          = m_Inspector->GetLastAction();
        const std::string lastChangedVariable = m_Inspector->GetLastChangedVariable();
        if (lastAction == "ResetRecommended") {
            changed = LoadInspectorForType(GetSelectedTypeIndex()) || changed;
        } else if (lastChangedVariable == "MaskType") {
            changed = LoadInspectorForType(GetSelectedTypeIndex()) || changed;
        } else if (changed) {
            SyncSettingsFromInspector();
        }
        if (changed)
            Invalidate();
        return changed;
    }

    SerializerNode CalculatedMaskGenerator::Save() const
    {
        auto node = CreateSerializerNode();
        node->SetInteger("MaskType", static_cast<int32_t>(m_Settings.type));
        node->SetInteger("MaskMode", m_Settings.shaderMode);
        const int typeIndex = glm::clamp(static_cast<int>(m_Settings.type), 0, static_cast<int>(CalculatedMaskType::Count) - 1);
        if (m_Metadata.contains("Types") && typeIndex < static_cast<int>(m_Metadata["Types"].size()))
            node->SetString("MaskTypeID", m_Metadata["Types"][typeIndex].value("ID", ""));
        if (m_Inspector != nullptr)
            node->SetChildNode("Inspector", m_Inspector->SaveData());
        return node;
    }

    void CalculatedMaskGenerator::Load(SerializerNode data)
    {
        if (data == nullptr || m_Inspector == nullptr)
            return;
        int typeIndex = FindTypeIndexByID(data->GetString("MaskTypeID", ""));
        if (typeIndex < 0 && data->HasKey("MaskMode")) {
            const int shaderMode = data->GetInteger("MaskMode", m_Settings.shaderMode);
            if (m_Metadata.contains("Types") && m_Metadata["Types"].is_array()) {
                for (size_t candidateIndex = 0; candidateIndex < m_Metadata["Types"].size(); ++candidateIndex)
                    if (m_Metadata["Types"][candidateIndex].value("Mode", -1) == shaderMode)
                        typeIndex = static_cast<int>(candidateIndex);
            }
        }
        if (typeIndex < 0)
            typeIndex = glm::clamp(data->GetInteger("MaskType", static_cast<int32_t>(m_Settings.type)),
                                   0, static_cast<int>(CalculatedMaskType::Count) - 1);
        if (!LoadInspectorForType(typeIndex))
            return;
        const auto inspectorData = data->GetChildNode("Inspector");
        if (inspectorData != nullptr)
            m_Inspector->LoadData(inspectorData);
        if (!m_Inspector->HasVariable("NoiseAlgorithm"))
            m_Inspector->AddIntegerVariable("NoiseAlgorithm", m_NoiseAlgorithms.DefaultValue());
        if (!m_Inspector->HasVariable("NoiseOctaves"))
            m_Inspector->AddIntegerVariable("NoiseOctaves", 5);
        if (!m_Inspector->HasVariable("NoiseLacunarity"))
            m_Inspector->AddFloatVariable("NoiseLacunarity", 2.0f);
        if (!m_Inspector->HasVariable("NoisePersistence"))
            m_Inspector->AddFloatVariable("NoisePersistence", 0.5f);
        if (!m_Inspector->HasVariable("NoiseWarp"))
            m_Inspector->AddFloatVariable("NoiseWarp", 0.0f);
        if (!m_Inspector->HasVariable("NoiseJitter"))
            m_Inspector->AddFloatVariable("NoiseJitter", 0.75f);
        if (m_Inspector->HasVariable("MaskType"))
            m_Inspector->GetVariable("MaskType").SetInt(typeIndex);
        SyncSettingsFromInspector();
        Invalidate();
    }

    bool CalculatedMaskGenerator::Update(GeneratorData *sourceData)
    {
        if (sourceData == nullptr || !m_Dirty)
            return false;

        sourceData->Bind(0);
        m_Texture->BindForCompute(1);
        m_Shader->Bind();
        m_Shader->SetUniform1i("u_Resolution", m_Size);
        m_Shader->SetUniform1i("u_Mode", m_Settings.shaderMode);
        m_Shader->SetUniform4f(
            "u_Range",
            m_Settings.minimum,
            m_Settings.maximum,
            m_Settings.softness,
            m_AppState->mainMap.tileSize);
        m_Shader->SetUniform4f("u_Settings0", m_Settings.angle, m_Settings.angleWidth, m_Settings.scale, m_Settings.seed);
        m_Shader->SetUniform1i("u_NoiseAlgorithm", glm::clamp(m_Settings.noiseAlgorithm, 0, m_NoiseAlgorithms.MaxValue()));
        m_Shader->SetUniform1f("u_NoiseScale", m_Settings.scale);
        m_Shader->SetUniform1f("u_NoiseSeed", m_Settings.seed);
        m_Shader->SetUniform1i("u_NoiseOctaves", glm::clamp(m_Settings.noiseOctaves, 1, 16));
        m_Shader->SetUniform1f("u_NoiseLacunarity", m_Settings.noiseLacunarity);
        m_Shader->SetUniform1f("u_NoisePersistence", m_Settings.noisePersistence);
        m_Shader->SetUniform1f("u_NoiseWarp", m_Settings.noiseWarp);
        m_Shader->SetUniform1f("u_NoiseJitter", m_Settings.noiseJitter);
        m_Shader->SetUniform4f("u_Settings1", m_Settings.center.x, m_Settings.center.y, m_Settings.pathEnd.x, m_Settings.pathEnd.y);
        m_Shader->SetUniform4f("u_Settings2", m_Settings.seaLevel, m_Settings.selectValleys ? 1.0f : 0.0f, m_Settings.usePath ? 1.0f : 0.0f, m_Settings.sampleRadius);
        m_Shader->SetUniform1f("u_CurvatureScale", m_Settings.curvatureScale);
        m_Shader->SetUniform1f("u_CavityScale", m_Settings.cavityScale);
        m_Shader->SetUniform1f("u_SpiralArms", m_Settings.spiralArms);
        m_Shader->SetUniform1f("u_SpiralTurns", m_Settings.spiralTurns);
        m_Shader->SetUniform1f("u_SpiralThickness", m_Settings.spiralThickness);
        m_Shader->SetUniform1f("u_SpiralSoftness", m_Settings.spiralSoftness);
        m_Shader->SetUniform1f("u_SpiralRotation", m_Settings.spiralRotation);
        m_Shader->SetUniform1i("u_SpiralInvert", m_Settings.spiralInvert ? 1 : 0);
        m_Shader->SetUniform1f("u_GridCells", m_Settings.gridCells);
        m_Shader->SetUniform1f("u_GridThickness", m_Settings.gridThickness);
        m_Shader->SetUniform1f("u_GridSoftness", m_Settings.gridSoftness);
        m_Shader->SetUniform1f("u_GridRotation", m_Settings.gridRotation);
        m_Shader->SetUniform1i("u_GridInvert", m_Settings.gridInvert ? 1 : 0);
        m_Shader->SetUniform1f("u_DotCells", m_Settings.dotCells);
        m_Shader->SetUniform1f("u_DotRadius", m_Settings.dotRadius);
        m_Shader->SetUniform1f("u_DotSoftness", m_Settings.dotSoftness);
        m_Shader->SetUniform1f("u_DotRotation", m_Settings.dotRotation);
        m_Shader->SetUniform1i("u_DotInvert", m_Settings.dotInvert ? 1 : 0);
        m_Shader->SetUniform1i("u_PathPointCount", glm::clamp(m_Settings.pathPointCount, 2, CalculatedMaskSettings::MaxPathPoints));
        for (int pointIndex = 0; pointIndex < CalculatedMaskSettings::MaxPathPoints; ++pointIndex) {
            m_Shader->SetUniform2f("u_PathPoints[" + std::to_string(pointIndex) + "]", m_Settings.pathPoints[pointIndex]);
        }
        const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
        const auto dispatchSize  = (m_Size + workgroupSize - 1) / workgroupSize;
        m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
        m_Shader->SetMemoryBarrier();
        m_Dirty = false;
        return true;
    }

} // namespace tf3d::generators
