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
        if (m_Inspector == nullptr || !m_Inspector->Contains("MaskType"))
            return static_cast<int>(m_Settings.type);
        return glm::clamp(m_Inspector->Get("MaskType", static_cast<int32_t>(m_Settings.type)), 0, static_cast<int>(CalculatedMaskType::Count) - 1);
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
        if (const auto *range = m_Inspector->FindValue("Range")) {
            if (range->GetType() == CustomInspectorValueType::Vector2) {
                const glm::vec2 value = range->Get<glm::vec2>();
                m_Settings.minimum    = std::min(value.x, value.y);
                m_Settings.maximum    = std::max(value.x, value.y);
                if (value.x > value.y)
                    m_Inspector->Set("Range", glm::vec2(m_Settings.minimum, m_Settings.maximum));
            } else {
                m_Settings.minimum = range->Get<float>(m_Settings.minimum);
                m_Settings.maximum = 1.0f;
            }
        }
        const auto readFloat = [this](const char *name, float &target) {
            if (const auto *value = m_Inspector->FindValue(name))
                target = value->Get<float>(target);
        };
        const auto readInt = [this](const char *name, int &target) {
            if (const auto *value = m_Inspector->FindValue(name))
                target = value->Get<int32_t>(target);
        };
        const auto readBool = [this](const char *name, bool &target) {
            if (const auto *value = m_Inspector->FindValue(name))
                target = value->Get<bool>(target);
        };

        readFloat("EdgeFeather", m_Settings.softness);
        readFloat("EdgeSoftness", m_Settings.softness);
        readFloat("Direction", m_Settings.angle);
        readFloat("DirectionWidth", m_Settings.angleWidth);
        readFloat("Scale", m_Settings.scale);
        readFloat("Seed", m_Settings.seed);
        readInt("NoiseAlgorithm", m_Settings.noiseAlgorithm);
        readInt("NoiseOctaves", m_Settings.noiseOctaves);
        readFloat("NoiseLacunarity", m_Settings.noiseLacunarity);
        readFloat("NoisePersistence", m_Settings.noisePersistence);
        readFloat("NoiseWarp", m_Settings.noiseWarp);
        readFloat("NoiseJitter", m_Settings.noiseJitter);
        readFloat("SampleRadius", m_Settings.sampleRadius);
        readFloat("CurvatureSensitivity", m_Settings.curvatureScale);
        readFloat("CavitySensitivity", m_Settings.cavityScale);
        readFloat("SeaLevel", m_Settings.seaLevel);
        readFloat("SpiralArms", m_Settings.spiralArms);
        readFloat("SpiralTurns", m_Settings.spiralTurns);
        readFloat("SpiralThickness", m_Settings.spiralThickness);
        readFloat("SpiralSoftness", m_Settings.spiralSoftness);
        readFloat("SpiralRotation", m_Settings.spiralRotation);
        readBool("SpiralInvert", m_Settings.spiralInvert);
        readFloat("GridCells", m_Settings.gridCells);
        readFloat("GridThickness", m_Settings.gridThickness);
        readFloat("GridSoftness", m_Settings.gridSoftness);
        readFloat("GridRotation", m_Settings.gridRotation);
        readBool("GridInvert", m_Settings.gridInvert);
        readFloat("DotCells", m_Settings.dotCells);
        readFloat("DotRadius", m_Settings.dotRadius);
        readFloat("DotSoftness", m_Settings.dotSoftness);
        readFloat("DotRotation", m_Settings.dotRotation);
        readBool("DotInvert", m_Settings.dotInvert);
        if (const auto *value = m_Inspector->FindValue("Center"))
            m_Settings.center = value->Get<glm::vec2>(m_Settings.center);
        readBool("UsePath", m_Settings.usePath);
        if (const auto *value = m_Inspector->FindValue("Feature"))
            m_Settings.selectValleys = value->Get<int32_t>(m_Settings.selectValleys ? 1 : 0) == 1;
        if (const auto *value = m_Inspector->FindValue("Path")) {
            const auto points         = value->Get<std::vector<glm::vec2>>();
            m_Settings.pathPointCount = glm::clamp(static_cast<int>(points.size()), 2, CalculatedMaskSettings::MaxPathPoints);
            for (int index = 0; index < m_Settings.pathPointCount; ++index)
                m_Settings.pathPoints[index] = points[static_cast<size_t>(index)];
            m_Settings.pathEnd = m_Settings.pathPoints[m_Settings.pathPointCount - 1];
        }
        if (m_Inspector->Contains("MaskType")) {
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
        node->Set("MaskType", static_cast<int32_t>(m_Settings.type));
        node->Set("MaskMode", m_Settings.shaderMode);
        const int typeIndex = glm::clamp(static_cast<int>(m_Settings.type), 0, static_cast<int>(CalculatedMaskType::Count) - 1);
        if (m_Metadata.contains("Types") && typeIndex < static_cast<int>(m_Metadata["Types"].size()))
            node->Set("MaskTypeID", m_Metadata["Types"][typeIndex].value("ID", ""));
        if (m_Inspector != nullptr)
            node->Set("Inspector", m_Inspector->SaveState());
        return node;
    }

    void CalculatedMaskGenerator::Load(SerializerNode data)
    {
        if (data == nullptr || m_Inspector == nullptr)
            return;
        int typeIndex = FindTypeIndexByID(data->Get<std::string>("MaskTypeID", ""));
        if (typeIndex < 0 && data->HasKey("MaskMode")) {
            const int shaderMode = data->Get<int>("MaskMode", m_Settings.shaderMode);
            if (m_Metadata.contains("Types") && m_Metadata["Types"].is_array()) {
                for (size_t candidateIndex = 0; candidateIndex < m_Metadata["Types"].size(); ++candidateIndex)
                    if (m_Metadata["Types"][candidateIndex].value("Mode", -1) == shaderMode)
                        typeIndex = static_cast<int>(candidateIndex);
            }
        }
        if (typeIndex < 0)
            typeIndex = glm::clamp(data->Get<int>("MaskType", static_cast<int32_t>(m_Settings.type)),
                                   0, static_cast<int>(CalculatedMaskType::Count) - 1);
        if (!LoadInspectorForType(typeIndex))
            return;
        const auto inspectorData = data->Get<SerializerNode>("Inspector");
        if (inspectorData != nullptr)
            m_Inspector->LoadState(inspectorData);
        if (!m_Inspector->Contains("NoiseAlgorithm"))
            m_Inspector->Add("NoiseAlgorithm", m_NoiseAlgorithms.DefaultValue());
        if (!m_Inspector->Contains("NoiseOctaves"))
            m_Inspector->Add("NoiseOctaves", 5);
        if (!m_Inspector->Contains("NoiseLacunarity"))
            m_Inspector->Add("NoiseLacunarity", 2.0f);
        if (!m_Inspector->Contains("NoisePersistence"))
            m_Inspector->Add("NoisePersistence", 0.5f);
        if (!m_Inspector->Contains("NoiseWarp"))
            m_Inspector->Add("NoiseWarp", 0.0f);
        if (!m_Inspector->Contains("NoiseJitter"))
            m_Inspector->Add("NoiseJitter", 0.75f);
        if (m_Inspector->Contains("MaskType"))
            m_Inspector->Set("MaskType", typeIndex);
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
