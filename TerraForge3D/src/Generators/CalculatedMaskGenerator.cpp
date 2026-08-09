#include "Generators/CalculatedMaskGenerator.h"
#include "Generators/NoiseAlgorithmConfig.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Inspector/CustomInspector.h"
#include "Utils/JsonIncludeResolver.h"
#include "Utils/Utils.h"

#include <cctype>
#include <cstring>
#include <filesystem>
#include <regex>
#include <unordered_set>

namespace tf3d::generators
{

    namespace
    {
        constexpr const char *kShaderModuleMarker = "/* TF3D_CALCULATED_MASK_MODULES */";

        bool MergeParameters(const nlohmann::json &parameters,
                             nlohmann::json &merged,
                             std::string *error)
        {
            if (!parameters.is_array()) {
                if (error)
                    *error = "Params must be an array";
                return false;
            }

            merged = nlohmann::json::array();
            std::unordered_map<std::string, size_t> indices;
            for (size_t parameterIndex = 0; parameterIndex < parameters.size(); ++parameterIndex) {
                const auto &parameter = parameters[parameterIndex];
                if (!parameter.is_object() || !parameter.contains("Name") || !parameter["Name"].is_string() ||
                    parameter["Name"].get<std::string>().empty()) {
                    if (error)
                        *error = "parameter " + std::to_string(parameterIndex) + " must define a non-empty Name";
                    return false;
                }

                const std::string name = parameter["Name"].get<std::string>();
                const auto existing    = indices.find(name);
                if (existing == indices.end()) {
                    indices.emplace(name, merged.size());
                    merged.push_back(parameter);
                    continue;
                }

                auto &base = merged[existing->second];
                if (base.contains("Type") && parameter.contains("Type") && base["Type"] != parameter["Type"]) {
                    if (error)
                        *error = "parameter '" + name + "' changes Type while being overridden";
                    return false;
                }
                utils::MergeObjects(base, parameter);
            }
            return true;
        }

        void RemoveConditions(nlohmann::json &node)
        {
            if (node.is_array()) {
                for (auto &child : node)
                    RemoveConditions(child);
                return;
            }
            if (!node.is_object())
                return;

            node.erase("Conditions");
            for (auto &[key, value] : node.items())
                RemoveConditions(value);
        }

        void RemoveFieldRecursive(nlohmann::json &node, const std::string &field)
        {
            if (node.is_array()) {
                for (auto &child : node)
                    RemoveFieldRecursive(child, field);
                return;
            }
            if (!node.is_object())
                return;

            node.erase(field);
            for (auto &[key, value] : node.items())
                RemoveFieldRecursive(value, field);
        }

        std::string StripGlslComments(const std::string &source)
        {
            std::string result;
            result.reserve(source.size());
            bool inLineComment  = false;
            bool inBlockComment = false;
            for (size_t index = 0; index < source.size(); ++index) {
                const char character = source[index];
                const char next      = index + 1 < source.size() ? source[index + 1] : '\0';

                if (inLineComment) {
                    if (character == '\n') {
                        inLineComment = false;
                        result.push_back('\n');
                    } else {
                        result.push_back(' ');
                    }
                    continue;
                }

                if (inBlockComment) {
                    if (character == '*' && next == '/') {
                        result.push_back(' ');
                        result.push_back(' ');
                        ++index;
                    } else {
                        result.push_back(character == '\n' ? '\n' : ' ');
                    }
                    if (character == '*' && next == '/')
                        inBlockComment = false;
                    continue;
                }

                if (character == '/' && next == '/') {
                    result.push_back(' ');
                    result.push_back(' ');
                    ++index;
                    inLineComment = true;
                    continue;
                }
                if (character == '/' && next == '*') {
                    result.push_back(' ');
                    result.push_back(' ');
                    ++index;
                    inBlockComment = true;
                    continue;
                }

                result.push_back(character);
            }
            return result;
        }

        bool ValidateShaderModule(const std::string &source,
                                  const std::string &path,
                                  std::string *error)
        {
            auto fail = [&](const std::string &message) {
                if (error)
                    *error = message;
                return false;
            };

            const std::string stripped = StripGlslComments(source);
            if (stripped.find("#version") != std::string::npos)
                return fail("must not declare #version");
            if (stripped.find("#include") != std::string::npos)
                return fail("must not contain #include; shared shader code is supplied by the host");
            if (stripped.find("uniform") != std::string::npos || stripped.find("layout") != std::string::npos ||
                stripped.find("buffer") != std::string::npos)
                return fail("must not declare uniforms, layouts, or buffers");

            const std::regex functionPattern(
                R"(\b(?:void|bool|int|uint|float|double|vec[234]|ivec[234]|uvec[234]|bvec[234]|mat[234]|MaskContext)\s+[A-Za-z_][A-Za-z0-9_]*\s*\([^;{}]*\)\s*\{)");
            const std::regex evaluatePattern(
                R"(\bfloat\s+evaluate\s*\(\s*inout\s+MaskContext\s+[A-Za-z_][A-Za-z0-9_]*\s*\)\s*\{)");

            size_t functionCount = 0;
            for (std::sregex_iterator iterator(stripped.begin(), stripped.end(), functionPattern), end; iterator != end; ++iterator)
                ++functionCount;

            size_t evaluateCount = 0;
            for (std::sregex_iterator iterator(stripped.begin(), stripped.end(), evaluatePattern), end; iterator != end; ++iterator)
                ++evaluateCount;

            if (functionCount != 1)
                return fail("must define exactly one function; found " + std::to_string(functionCount));
            if (evaluateCount != 1)
                return fail("must define exactly one 'float evaluate(inout MaskContext ...)' function");

            (void)path;
            return true;
        }

        bool ResolveShaderPath(const std::filesystem::path &shaderRoot,
                               const std::string &relativePath,
                               std::filesystem::path &resolvedPath,
                               std::string *error)
        {
            auto fail = [&](const std::string &message) {
                if (error)
                    *error = message;
                return false;
            };

            const std::filesystem::path relative(relativePath);
            if (relative.empty() || relative.is_absolute() || relative.extension() != ".glsl")
                return fail("shader path must be a relative .glsl path");

            std::error_code errorCode;
            const auto root = std::filesystem::weakly_canonical(shaderRoot, errorCode);
            if (errorCode)
                return fail("could not resolve shader root");

            errorCode.clear();
            const auto candidate = std::filesystem::weakly_canonical(root / relative, errorCode);
            if (errorCode || !std::filesystem::is_regular_file(candidate))
                return fail("shader file does not exist");

            const auto relativeCandidate = candidate.lexically_relative(root);
            const auto relativeText      = relativeCandidate.generic_string();
            if (relativeCandidate.empty() || relativeText == ".." || relativeText.starts_with("../"))
                return fail("shader path escapes the shader root");

            resolvedPath = candidate;
            return true;
        }

        std::string AssembleShader(const std::string &baseSource,
                                   const std::vector<CalculatedMaskGenerator::AlgorithmDefinition> &algorithms)
        {
            std::string generated;
            for (size_t index = 0; index < algorithms.size(); ++index) {
                generated += "\n#define evaluate tf3d_calculated_mask_evaluate_" + std::to_string(index) + "\n";
                generated += algorithms[index].shaderSource;
                if (!generated.empty() && generated.back() != '\n')
                    generated.push_back('\n');
                generated += "#undef evaluate\n";
            }

            generated += "\nfloat tf3d_calculated_mask_dispatch(inout MaskContext context)\n{\n    switch (u_Mode)\n    {\n";
            for (size_t index = 0; index < algorithms.size(); ++index) {
                generated += "        case " + std::to_string(index) + ": return tf3d_calculated_mask_evaluate_" +
                             std::to_string(index) + "(context);\n";
            }
            generated += "        default: return 1.0;\n    }\n}\n";

            const size_t markerPosition = baseSource.find(kShaderModuleMarker);
            if (markerPosition == std::string::npos)
                return {};
            std::string result = baseSource;
            result.replace(markerPosition, std::strlen(kShaderModuleMarker), generated);
            return result;
        }
    } // namespace

    CalculatedMaskGenerator::CalculatedMaskGenerator(ApplicationState *state, std::string defaultTypeID)
        : m_AppState(state), m_DefaultTypeID(std::move(defaultTypeID))
    {
        if (m_AppState == nullptr) {
            TF3D_LOG_ERROR("Cannot construct calculated mask generator without application state.");
            return;
        }

        std::string catalogError;
        if (!m_NoiseAlgorithms.LoadFromFile(NoiseAlgorithmCatalog::IndexPath(m_AppState->constants.shadersDir), &catalogError))
            TF3D_LOG_ERROR("{}", catalogError);

        m_Texture   = std::make_shared<GeneratorTexture>(m_Size, m_Size, GeneratorTextureStorage::R16);
        m_Inspector = std::make_shared<CustomInspector>();
        m_Inspector->SetShowResetButton(false);

        const bool configLoaded = LoadMetadata();
        bool shaderSourceLoaded = false;
        const auto shaderSource = m_AppState->resourceManager->LoadShaderSource(
            "masks/mask_gen", false, &shaderSourceLoaded);
        if (shaderSourceLoaded) {
            BuildShader(shaderSource);
        } else {
            TF3D_LOG_ERROR("Calculated mask preview shader source could not be loaded.");
        }

        if (configLoaded && !m_Algorithms.empty()) {
            const int defaultIndex = FindTypeIndexByID(m_DefaultTypeID) >= 0
                                         ? FindTypeIndexByID(m_DefaultTypeID)
                                         : 0;
            m_MetadataLoaded       = RebuildInspector(defaultIndex, false);
        }

        if (!m_MetadataLoaded)
            TF3D_LOG_ERROR("Calculated mask metadata could not be loaded; calculated mask settings are unavailable.");
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
        if (m_Algorithms.empty())
            return -1;
        const int fallback = m_SelectedAlgorithmIndex >= 0 && m_SelectedAlgorithmIndex < static_cast<int>(m_Algorithms.size())
                                 ? m_SelectedAlgorithmIndex
                                 : 0;
        if (m_Inspector == nullptr || !m_Inspector->Contains("MaskType"))
            return fallback;

        const int selectedMode = m_Inspector->Get<int32_t>("MaskType", fallback);
        return selectedMode >= 0 && selectedMode < static_cast<int>(m_Algorithms.size()) ? selectedMode : fallback;
    }

    int CalculatedMaskGenerator::GetShaderModeForType(int typeIndex) const
    {
        return typeIndex >= 0 && typeIndex < static_cast<int>(m_Algorithms.size())
                   ? m_Algorithms[typeIndex].runtimeMode
                   : -1;
    }

    bool CalculatedMaskGenerator::LoadMetadata()
    {
        if (m_AppState == nullptr)
            return false;

        const auto inspectorPath = tf3d::inspector::CustomInspector::GetConfigPath(
            std::filesystem::path(m_AppState->constants.dataDir), "CalculatedMask");
        utils::JsonIncludeResolverOptions resolverOptions;
        resolverOptions.rootDirectory  = std::filesystem::path(m_AppState->constants.dataDir) / "inspectors";
        resolverOptions.pathMode       = utils::JsonIncludePathMode::RelativeToIncludingFile;
        resolverOptions.restrictToRoot = true;
        const utils::JsonIncludeResolver resolver(resolverOptions);
        std::string resolveError;
        const auto document = resolver.ResolveFile(inspectorPath, &resolveError);
        if (!document) {
            TF3D_LOG_ERROR("Could not load calculated mask inspector '{}': {}", inspectorPath.string(), resolveError);
            return false;
        }
        if (!document->is_object() || !document->contains("Sections") || !(*document)["Sections"].is_array()) {
            TF3D_LOG_ERROR("Calculated mask inspector '{}' must contain a Sections array.", inspectorPath.string());
            return false;
        }

        m_InspectorDocument = *document;
        m_CommonSections.clear();
        m_Algorithms.clear();
        std::unordered_set<std::string> algorithmIDs;

        for (size_t sectionIndex = 0; sectionIndex < m_InspectorDocument["Sections"].size(); ++sectionIndex) {
            const auto &section = m_InspectorDocument["Sections"][sectionIndex];
            if (!section.is_object()) {
                TF3D_LOG_WARN("Skipping calculated mask inspector section {}: expected an object.", sectionIndex);
                continue;
            }

            const auto customDataIterator = section.find("CustomData") != section.end()
                                                ? section.find("CustomData")
                                                : section.find("customData");
            const bool hasCustomData      = customDataIterator != section.end();
            if (!hasCustomData) {
                m_CommonSections.push_back(section);
                continue;
            }

            const auto &customData = *customDataIterator;
            if (!customData.is_object()) {
                TF3D_LOG_ERROR("Skipping calculated mask section {}: CustomData must be an object.", sectionIndex);
                continue;
            }
            const std::string id         = customData.value("ID", "");
            const std::string shaderPath = customData.value("Shader", "");
            if (!utils::IsPascalIdentifier(id) || !algorithmIDs.insert(utils::CanonicalID(id)).second) {
                TF3D_LOG_ERROR("Skipping calculated mask section {}: CustomData.ID must be a unique PascalCase identifier.", sectionIndex);
                continue;
            }
            if (shaderPath.empty()) {
                TF3D_LOG_ERROR("Skipping calculated mask algorithm '{}': CustomData.Shader is required.", id);
                continue;
            }

            AlgorithmDefinition algorithm;
            algorithm.id          = id;
            algorithm.label       = section.value("Label", section.value("Name", id));
            algorithm.description = section.value("Description", "");
            algorithm.shaderPath  = shaderPath;
            algorithm.section     = section;

            const auto parameters = section.value("Params", nlohmann::json::array());
            std::string parameterError;
            nlohmann::json mergedParameters;
            if (!MergeParameters(parameters, mergedParameters, &parameterError)) {
                TF3D_LOG_ERROR("Skipping calculated mask algorithm '{}': {}", id, parameterError);
                continue;
            }
            RemoveConditions(mergedParameters);
            algorithm.section["Params"] = std::move(mergedParameters);
            m_Algorithms.push_back(std::move(algorithm));
        }

        return !m_CommonSections.empty();
    }

    bool CalculatedMaskGenerator::BuildShader(const std::string &baseShaderSource)
    {
        if (baseShaderSource.find(kShaderModuleMarker) == std::string::npos) {
            TF3D_LOG_ERROR("Calculated mask shader is missing the module assembly marker.");
            return false;
        }

        const std::string sourceWithNoiseDefines = m_NoiseAlgorithms.IsValid()
                                                       ? m_NoiseAlgorithms.InjectShaderDefines(baseShaderSource)
                                                       : baseShaderSource;
        const auto shaderRoot                    = std::filesystem::path(m_AppState->constants.shadersDir);
        const auto candidates                    = std::move(m_Algorithms);
        m_Algorithms.clear();

        for (auto candidate : candidates) {
            std::filesystem::path modulePath;
            std::string pathError;
            if (!ResolveShaderPath(shaderRoot, candidate.shaderPath, modulePath, &pathError)) {
                TF3D_LOG_ERROR("Skipping calculated mask algorithm '{}': {} ({})", candidate.id, pathError, candidate.shaderPath);
                continue;
            }

            bool loaded            = false;
            candidate.shaderSource = ReadShaderSourceFile(modulePath.string(), &loaded);
            if (!loaded) {
                TF3D_LOG_ERROR("Skipping calculated mask algorithm '{}': could not read shader '{}'", candidate.id, modulePath.string());
                continue;
            }

            std::string validationError;
            if (!ValidateShaderModule(candidate.shaderSource, modulePath.string(), &validationError)) {
                TF3D_LOG_ERROR("Skipping calculated mask algorithm '{}' ({}): {}", candidate.id, modulePath.string(), validationError);
                continue;
            }

            m_Algorithms.push_back(std::move(candidate));
        }

        const auto assignRuntimeModes = [&]() {
            for (size_t index = 0; index < m_Algorithms.size(); ++index)
                m_Algorithms[index].runtimeMode = static_cast<int>(index);
        };
        assignRuntimeModes();

        m_Shader.reset();
        const std::string finalSource = AssembleShader(sourceWithNoiseDefines, m_Algorithms);
        if (!finalSource.empty())
            m_Shader = m_AppState->resourceManager->GetComputeShader("CalculatedMaskPreview", finalSource);

        if (!m_Shader.has_value()) {
            TF3D_LOG_ERROR("Calculated mask shader assembly failed; checking modules individually.");

            const auto candidates = std::move(m_Algorithms);
            m_Algorithms.clear();
            for (auto candidate : candidates) {
                const std::vector<AlgorithmDefinition> probeAlgorithms{candidate};
                const std::string probeSource = AssembleShader(sourceWithNoiseDefines, probeAlgorithms);
                if (probeSource.empty()) {
                    TF3D_LOG_ERROR("Skipping calculated mask algorithm '{}': failed to assemble its shader probe.", candidate.id);
                    continue;
                }

                const auto probeShader = m_AppState->resourceManager->GetComputeShader(
                    "CalculatedMaskModule_" + candidate.id, probeSource);
                if (!probeShader.has_value()) {
                    TF3D_LOG_ERROR("Skipping calculated mask algorithm '{}': shader compilation failed.", candidate.id);
                    continue;
                }

                m_Algorithms.push_back(std::move(candidate));
            }

            assignRuntimeModes();
            const std::string filteredSource = AssembleShader(sourceWithNoiseDefines, m_Algorithms);
            if (!filteredSource.empty())
                m_Shader = m_AppState->resourceManager->GetComputeShader("CalculatedMaskPreview", filteredSource);

            if (!m_Shader.has_value()) {
                TF3D_LOG_ERROR("Calculated mask shader assembly still failed; using the neutral fallback mask.");
                const std::string fallbackSource = AssembleShader(sourceWithNoiseDefines, {});
                if (!fallbackSource.empty())
                    m_Shader = m_AppState->resourceManager->GetComputeShader("CalculatedMaskFallback", fallbackSource);
            }
        }
        return m_Shader.has_value();
    }

    nlohmann::json CalculatedMaskGenerator::BuildInspectorConfig(int typeIndex) const
    {
        nlohmann::json config = m_InspectorDocument;
        config["Sections"]    = nlohmann::json::array();
        for (const auto &section : m_CommonSections)
            config["Sections"].push_back(section);
        if (typeIndex >= 0 && typeIndex < static_cast<int>(m_Algorithms.size()))
            config["Sections"].push_back(m_Algorithms[typeIndex].section);
        return config;
    }

    nlohmann::json CalculatedMaskGenerator::StripTransientInspectorState(nlohmann::json state) const
    {
        RemoveFieldRecursive(state, "MaskType");
        return state;
    }

    bool CalculatedMaskGenerator::StoreActiveInspectorState()
    {
        if (m_Inspector == nullptr || m_SelectedAlgorithmIndex < 0 || m_SelectedAlgorithmIndex >= static_cast<int>(m_Algorithms.size()))
            return false;
        const auto state = m_Inspector->SaveState();
        if (state == nullptr)
            return false;
        m_AlgorithmStates[m_Algorithms[m_SelectedAlgorithmIndex].id] = StripTransientInspectorState(state->ToJson());
        return true;
    }

    void CalculatedMaskGenerator::ConfigureAlgorithmSelector()
    {
        if (m_Inspector == nullptr || !m_Inspector->HasWidget("Mask type"))
            return;

        std::vector<std::string> labels;
        std::vector<int32_t> values;
        labels.reserve(m_Algorithms.size());
        values.reserve(m_Algorithms.size());
        for (size_t index = 0; index < m_Algorithms.size(); ++index) {
            labels.push_back(m_Algorithms[index].label.empty() ? m_Algorithms[index].id : m_Algorithms[index].label);
            values.push_back(static_cast<int32_t>(index));
        }
        m_Inspector->GetWidget("Mask type").SetDropdownOptions(labels, values);
    }

    bool CalculatedMaskGenerator::RebuildInspector(int typeIndex, bool preserveCurrentState)
    {
        if (m_Inspector == nullptr || typeIndex < 0 || typeIndex >= static_cast<int>(m_Algorithms.size()))
            return false;
        if (preserveCurrentState)
            StoreActiveInspectorState();

        auto config = BuildInspectorConfig(typeIndex);
        if (m_NoiseAlgorithms.IsValid() && !ApplyNoiseAlgorithmMetadata(config, m_NoiseAlgorithms))
            return false;
        if (!m_Inspector->LoadConfig(config))
            return false;

        m_Inspector->SetShowResetButton(false);
        ConfigureAlgorithmSelector();
        const auto savedState = m_AlgorithmStates.find(m_Algorithms[typeIndex].id);
        if (savedState != m_AlgorithmStates.end())
            m_Inspector->LoadState(CreateSerializerNodeFromJson(savedState->second));
        m_Inspector->Set("MaskType", typeIndex);
        m_SelectedAlgorithmIndex = typeIndex;
        return true;
    }

    int CalculatedMaskGenerator::FindTypeIndexByID(const std::string &id) const
    {
        const std::string canonicalID = utils::CanonicalID(id);
        if (canonicalID.empty())
            return -1;
        for (size_t index = 0; index < m_Algorithms.size(); ++index) {
            if (utils::CanonicalID(m_Algorithms[index].id) == canonicalID)
                return static_cast<int>(index);
        }
        return -1;
    }

    int CalculatedMaskGenerator::FindTypeIndexByMode(int mode) const
    {
        for (size_t index = 0; index < m_Algorithms.size(); ++index) {
            if (m_Algorithms[index].runtimeMode == mode)
                return static_cast<int>(index);
        }
        return -1;
    }

    bool CalculatedMaskGenerator::ShowSettings()
    {
        if (!m_MetadataLoaded || m_Inspector == nullptr || m_Algorithms.empty())
            return false;

        const int previousIndex        = m_SelectedAlgorithmIndex;
        bool changed                   = m_Inspector->Render();
        const std::string lastAction   = m_Inspector->GetLastAction();
        const std::string lastVariable = m_Inspector->GetLastChangedVariable();

        if (lastAction == "ResetRecommended") {
            const int selectedIndex = previousIndex >= 0 ? previousIndex : GetSelectedTypeIndex();
            if (selectedIndex >= 0 && selectedIndex < static_cast<int>(m_Algorithms.size()))
                m_AlgorithmStates.erase(m_Algorithms[selectedIndex].id);
            changed = RebuildInspector(selectedIndex, false) || changed;
        } else if (lastVariable == "MaskType") {
            const int selectedIndex = GetSelectedTypeIndex();
            if (selectedIndex != previousIndex)
                changed = RebuildInspector(selectedIndex, true) || changed;
        }

        if (changed)
            Invalidate();
        return changed;
    }

    SerializerNode CalculatedMaskGenerator::Save() const
    {
        auto node           = CreateSerializerNode();
        const int typeIndex = GetSelectedTypeIndex();
        if (typeIndex >= 0 && typeIndex < static_cast<int>(m_Algorithms.size()))
            node->Set("MaskTypeID", m_Algorithms[typeIndex].id);

        if (m_Inspector != nullptr) {
            const auto state = m_Inspector->SaveState();
            if (state != nullptr)
                node->Set("Inspector", CreateSerializerNodeFromJson(StripTransientInspectorState(state->ToJson())));
        }
        return node;
    }

    void CalculatedMaskGenerator::Load(SerializerNode data)
    {
        if (data == nullptr || m_Inspector == nullptr || m_Algorithms.empty())
            return;

        int typeIndex = FindTypeIndexByID(data->Get<std::string>("MaskTypeID", ""));
        if (typeIndex < 0 && data->HasKey("MaskMode"))
            typeIndex = FindTypeIndexByMode(data->Get<int>("MaskMode", 0));
        if (typeIndex < 0)
            typeIndex = FindTypeIndexByID(m_DefaultTypeID);
        if (typeIndex < 0)
            typeIndex = 0;

        m_AlgorithmStates.clear();
        if (!RebuildInspector(typeIndex, false))
            return;

        const auto inspectorData = data->Get<SerializerNode>("Inspector");
        if (inspectorData != nullptr) {
            nlohmann::json inspectorState = StripTransientInspectorState(inspectorData->ToJson());
            m_Inspector->LoadState(CreateSerializerNodeFromJson(inspectorState));
        }
        m_Inspector->Set("MaskType", typeIndex);
        Invalidate();
    }

    bool CalculatedMaskGenerator::Update(GeneratorData *sourceData)
    {
        if (sourceData == nullptr || !m_Dirty || m_Inspector == nullptr || !m_Shader.has_value())
            return false;

        sourceData->Bind(0);
        m_Texture->BindForCompute(1);
        m_Shader->Bind();
        m_Inspector->ApplyToShader(*m_Shader);
        m_Shader->SetUniform1i("u_Mode", std::max(GetShaderModeForType(GetSelectedTypeIndex()), 0));
        m_Shader->SetUniform1i("u_Resolution", m_Size);
        m_Shader->SetUniform1f("u_TileSize", m_AppState->mainMap.tileSize);
        const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
        const auto dispatchSize  = (m_Size + workgroupSize - 1) / workgroupSize;
        m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
        m_Shader->SetMemoryBarrier();
        m_Dirty = false;
        return true;
    }

} // namespace tf3d::generators
