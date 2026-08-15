#include "Generators/Masks/BaseMaskGenerator.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Inspector/CustomInspector.h"
#include "Utils/Utils.h"

#include <filesystem>
#include <regex>
#include <unordered_set>

namespace tf3d::generators
{

    namespace
    {
        constexpr const char *kShaderUniformMarker = "/* TF3D_CALCULATED_MASK_UNIFORMS */";
        constexpr const char *kShaderModuleMarker  = "/* TF3D_CALCULATED_MASK_MODULES */";
        constexpr const char *kNoneTypeID          = "None";

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
                                   const std::string &uniformDeclarations,
                                   const std::vector<BaseMaskGenerator::AlgorithmDefinition> &algorithms)
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

            std::string result = baseSource;
            if (!utils::ReplaceAll(result, kShaderUniformMarker, uniformDeclarations))
                return {};
            if (!utils::ReplaceAll(result, kShaderModuleMarker, generated))
                return {};
            return result;
        }
    } // namespace

    BaseMaskGenerator::BaseMaskGenerator(tf3d::data::ApplicationState *state,
                                         std::string defaultTypeID)
        : m_AppState(state), m_DefaultTypeID(std::move(defaultTypeID))
    {
        if (m_AppState == nullptr) {
            TF3D_LOG_ERROR("Cannot construct calculated mask generator without application state.");
            return;
        }

        m_Inspector = std::make_shared<inspector::CustomInspector>();
        m_Inspector->SetShowResetButton(false);

        const bool configLoaded = LoadMetadata();
        bool shaderSourceLoaded = false;
        const auto shaderSource = m_AppState->resourceManager->LoadShaderSource(
            "masks/mask_gen", false, &shaderSourceLoaded);
        if (shaderSourceLoaded) {
            BuildShaderSource(shaderSource);
        } else {
            TF3D_LOG_ERROR("Calculated mask preview shader source could not be loaded.");
        }

        if (configLoaded && !m_Algorithms.empty()) {
            const int defaultIndex   = FindTypeIndexByID(m_DefaultTypeID) >= 0
                                           ? FindTypeIndexByID(m_DefaultTypeID)
                                           : 0;
            m_SelectedAlgorithmIndex = defaultIndex;
            m_Inspector->Root().Scope("Mask").Set("MaskType", m_Algorithms[defaultIndex].selectionValue);
            m_MetadataLoaded = true;
        }

        if (!m_MetadataLoaded)
            TF3D_LOG_ERROR("Calculated mask metadata could not be loaded; calculated mask settings are unavailable.");
    }

    BaseMaskGenerator::~BaseMaskGenerator()
    {
    }

    int BaseMaskGenerator::GetSelectedTypeIndex() const
    {
        if (m_Algorithms.empty()) {
            return -1;
        }
        const int defaultIndex = m_SelectedAlgorithmIndex >= 0 && m_SelectedAlgorithmIndex < static_cast<int>(m_Algorithms.size())
                                     ? m_SelectedAlgorithmIndex
                                     : 0;
        if (m_Inspector == nullptr || !m_Inspector->Root().Scope("Mask").Contains("MaskType"))
            return defaultIndex;

        const int selectedValue = m_Inspector->Root().Scope("Mask").Get<int32_t>(
            "MaskType", m_Algorithms[defaultIndex].selectionValue);
        for (size_t index = 0; index < m_Algorithms.size(); ++index) {
            if (m_Algorithms[index].selectionValue == selectedValue)
                return static_cast<int>(index);
        }
        return defaultIndex;
    }

    int BaseMaskGenerator::GetShaderModeForType(int typeIndex) const
    {
        return typeIndex >= 0 && typeIndex < static_cast<int>(m_Algorithms.size())
                   ? m_Algorithms[typeIndex].runtimeMode
                   : -1;
    }

    bool BaseMaskGenerator::LoadMetadata()
    {
        if (m_AppState == nullptr || m_Inspector == nullptr)
            return false;
        m_Algorithms.clear();
        std::unordered_set<std::string> algorithmIDs;

        if (!m_Inspector->LoadConfig(m_AppState, "CalculatedMask"))
            return false;
        m_Inspector->SetShowResetButton(false);

        for (const auto &sectionName : m_Inspector->GetSectionsOrder()) {
            const auto shaderPath = m_Inspector->GetSectionCustomDataString(sectionName, "Shader");
            if (!shaderPath || shaderPath->empty())
                continue;
            if (!utils::IsPascalIdentifier(sectionName) || !algorithmIDs.insert(utils::CanonicalID(sectionName)).second)
                continue;

            const auto &section = m_Inspector->GetSection(sectionName);
            AlgorithmDefinition algorithm;
            algorithm.id             = sectionName;
            algorithm.label          = section.label.empty() ? sectionName : section.label;
            algorithm.description    = section.description;
            algorithm.shaderPath     = *shaderPath;
            algorithm.selectionValue = m_Inspector->GetSectionSelectionValue(sectionName)
                                           .value_or(static_cast<int32_t>(m_Algorithms.size()));
            m_Algorithms.push_back(std::move(algorithm));
        }

        return !m_Algorithms.empty();
    }

    bool BaseMaskGenerator::BuildShaderSource(const std::string &baseShaderSource)
    {
        if (baseShaderSource.find(kShaderUniformMarker) == std::string::npos) {
            TF3D_LOG_ERROR("Calculated mask shader is missing the inspector uniform declaration marker.");
            return false;
        }
        if (baseShaderSource.find(kShaderModuleMarker) == std::string::npos) {
            TF3D_LOG_ERROR("Calculated mask shader is missing the module assembly marker.");
            return false;
        }

        std::string uniformError;
        const auto uniformDeclarations = m_Inspector->GetShaderUniformDeclarations(&uniformError);
        if (!uniformDeclarations) {
            TF3D_LOG_ERROR("Cannot generate calculated-mask uniform declarations: {}", uniformError);
            return false;
        }

        const auto shaderRoot = std::filesystem::path(m_AppState->constants.shadersDir);
        const auto candidates = std::move(m_Algorithms);
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

        if (m_Algorithms.empty()) {
            TF3D_LOG_ERROR("No valid base mask algorithms remain; base mask shader source was not created.");
            return false;
        }

        for (size_t index = 0; index < m_Algorithms.size(); ++index) {
            m_Algorithms[index].runtimeMode = static_cast<int>(index);
        }

        const std::string finalSource = AssembleShader(baseShaderSource, *uniformDeclarations, m_Algorithms);
        if (finalSource.empty()) {
            TF3D_LOG_ERROR("Base mask shader assembly failed; calculated base generation is unavailable.");
            m_Algorithms.clear();
            return false;
        }

        AlgorithmDefinition none;
        none.id             = kNoneTypeID;
        none.label          = "None";
        none.description    = "Starts from a black base without a calculated terrain signal.";
        none.selectionValue = -1;
        none.runtimeMode    = -1;
        m_Algorithms.insert(m_Algorithms.begin(), std::move(none));

        std::vector<std::string> labels;
        std::vector<int32_t> values;
        labels.reserve(m_Algorithms.size());
        values.reserve(m_Algorithms.size());
        for (const auto &algorithm : m_Algorithms) {
            labels.push_back(algorithm.label.empty() ? algorithm.id : algorithm.label);
            values.push_back(algorithm.selectionValue);
        }
        if (!m_Inspector->Root().Scope("Mask").SetDropdownOptions("MaskType", labels, values)) {
            TF3D_LOG_ERROR("Base mask inspector is missing its MaskType dropdown.");
            m_Algorithms.clear();
            return false;
        }

        m_ShaderSource = finalSource;
        return true;
    }

    int BaseMaskGenerator::FindTypeIndexByID(const std::string &id) const
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

    int BaseMaskGenerator::FindTypeIndexByMode(int mode) const
    {
        for (size_t index = 0; index < m_Algorithms.size(); ++index) {
            if (m_Algorithms[index].runtimeMode == mode)
                return static_cast<int>(index);
        }
        return -1;
    }

    bool BaseMaskGenerator::IsNone() const
    {
        const int typeIndex = GetSelectedTypeIndex();
        return typeIndex >= 0 && typeIndex < static_cast<int>(m_Algorithms.size()) &&
               m_Algorithms[typeIndex].id == kNoneTypeID;
    }

    bool BaseMaskGenerator::SetTypeID(std::string_view typeID)
    {
        const int typeIndex = FindTypeIndexByID(std::string(typeID));
        if (typeIndex < 0 || m_Inspector == nullptr)
            return false;

        const bool changed = m_SelectedAlgorithmIndex != typeIndex ||
                             GetSelectedTypeIndex() != typeIndex;
        m_SelectedAlgorithmIndex = typeIndex;
        m_Inspector->Root().Scope("Mask").Set("MaskType", m_Algorithms[typeIndex].selectionValue);
        return changed;
    }

    bool BaseMaskGenerator::ShowSettings()
    {
        if (!m_MetadataLoaded || m_Inspector == nullptr || m_Algorithms.empty())
            return false;

        const int previousIndex        = GetSelectedTypeIndex();
        bool changed                   = m_Inspector->Render();
        const std::string lastAction   = m_Inspector->GetLastAction();
        const std::string lastVariable = m_Inspector->GetLastChangedVariable();

        if (lastAction == "ResetRecommended") {
            const int selectedIndex = previousIndex >= 0 ? previousIndex : GetSelectedTypeIndex();
            if (selectedIndex >= 0 && selectedIndex < static_cast<int>(m_Algorithms.size())) {
                m_Inspector->ResetVisible();
                m_Inspector->Root().Scope("Mask").Set("MaskType", m_Algorithms[selectedIndex].selectionValue);
                m_SelectedAlgorithmIndex = selectedIndex;
                changed                  = true;
            }
        } else if (lastVariable == "MaskType") {
            const int selectedIndex = GetSelectedTypeIndex();
            if (selectedIndex != previousIndex) {
                m_SelectedAlgorithmIndex = selectedIndex;
                changed                  = true;
            }
        }

        return changed;
    }

    BaseMaskGenerator::State BaseMaskGenerator::GetState() const
    {
        State state{};
        state.runtimeMode = GetShaderModeForType(GetSelectedTypeIndex());
        if (m_Inspector != nullptr) {
            state.values.emplace(m_Inspector->Clone());
        }
        return state;
    }

    SerializerNode BaseMaskGenerator::Save() const
    {
        auto node           = CreateSerializerNode();
        const int typeIndex = GetSelectedTypeIndex();
        if (typeIndex >= 0 && typeIndex < static_cast<int>(m_Algorithms.size()))
            node->Set("MaskTypeID", m_Algorithms[typeIndex].id);

        if (m_Inspector != nullptr) {
            const auto state = m_Inspector->SaveState({"MaskType"});
            if (state != nullptr)
                node->Set("Inspector", state);
        }
        return node;
    }

    void BaseMaskGenerator::Load(SerializerNode data)
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

        const auto inspectorData = data->Get<SerializerNode>("Inspector");
        if (inspectorData != nullptr)
            m_Inspector->LoadState(inspectorData);
        m_SelectedAlgorithmIndex = typeIndex;
        m_Inspector->Root().Scope("Mask").Set("MaskType", m_Algorithms[typeIndex].selectionValue);
    }

    void BaseMaskGenerator::ApplyToShader(const State &state, tf3d::base::ComputeShader &shader) const
    {
        if (m_Inspector != nullptr && state.values.has_value()) {
            m_Inspector->ApplyToShader(*state.values, shader);
        }
        shader.SetUniform1i("u_Mode", state.runtimeMode);
    }

} // namespace tf3d::generators
