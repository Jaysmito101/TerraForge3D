#include "Generators/BiomeBaseShapeGenerator.h"

#include "Data/ApplicationState.h"
#include "Profiler.h"
#include "Utils/Utils.h"

#include <string_view>

namespace tf3d::generators
{

    namespace
    {
        constexpr std::string_view kUniformsMarker = "/* TF3D_BASE_SHAPE_UNIFORMS */";
        constexpr std::string_view kSourceMarker   = "/* TF3D_BASE_SHAPE_SOURCE */";
    } // namespace

    bool BiomeBaseShapeGenerator::ShowSettings()
    {
        ImGui::PushID(m_ID.c_str());
        if (!m_Description.empty() && m_Inspector.GetDescription().empty()) {
            ImGui::TextWrapped("%s", m_Description.c_str());
            ImGui::Separator();
        }
        if (m_Inspector.Render()) {
            m_State.Replace(State{m_Inspector.Clone()});
        }
        ImGui::PopID();

        return RequireUpdation();
    }

    void BiomeBaseShapeGenerator::Update(const Snapshot *state,
                                         const GenerationContext *context,
                                         GeneratorData *buffer)
    {
        if (state == nullptr || context == nullptr || !m_Shader || m_AppState == nullptr || buffer == nullptr) {
            return;
        }

        TF3D_PROFILE_SCOPE_CHILD_LAZY(std::string("base-shape/") + m_Name);

        m_Shader->Bind();
        buffer->Bind(0);
        m_Inspector.ApplyToShader(state->value.values, *m_Shader);
        m_Shader->SetUniform1i("u_Resolution", context->tileResolution);
        m_Shader->SetUniform1i("u_UseSeedTexture", context->seedTexture != nullptr ? 1 : 0);
        if (context->seedTexture != nullptr) {
            m_Shader->SetUniform1i("u_SeedTexture", context->seedTexture->Bind(1));
        }

        const int32_t workgroupSize = context->gpuWorkgroupSize > 0 ? context->gpuWorkgroupSize : 1;
        const auto dispatchSize     = (context->tileResolution + workgroupSize - 1) / workgroupSize;
        TF3D_PROFILE_GPU_SCOPE_CHILD("gpu");
        TF3D_PROFILE_VALUE_DOMAIN("generation/base-shape/dispatch", dispatchSize, dispatchSize, 1,
                                  PerformanceMonitor::Domain::Generation);
        m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
        m_Shader->SetMemoryBarrier();

        m_State.MarkProcessed(state->revision);
    }

    bool BiomeBaseShapeGenerator::Load(SerializerNode data)
    {
        if (data == nullptr) {
            return false;
        }

        const auto inspectorState = data->Get<SerializerNode>("Inspector");
        if (inspectorState == nullptr) {
            return false;
        }

        if (!m_Inspector.LoadState(inspectorState)) {
            return false;
        }

        m_State.Replace(State{m_Inspector.Clone()});
        return true;
    }

    SerializerNode BiomeBaseShapeGenerator::Save() const
    {
        auto node = CreateSerializerNode();
        node->Set("Inspector", m_Inspector.SaveState());
        return node;
    }

    std::string BiomeBaseShapeGenerator::BuildShaderSource(const std::string &templateSource,
                                                           const std::string &uniformDeclarations)
    {
        if (templateSource.empty()) {
            return {};
        }

        bool includeSuccess      = false;
        const std::string source = m_AppState->resourceManager->PreprocessShaderSource(m_Source, m_ShaderPath, &includeSuccess);
        if (!includeSuccess) {
            TF3D_LOG_ERROR("Failed to preprocess base-shape shader '{}'", m_ShaderPath);
            return {};
        }

        std::string result = templateSource;
        if (!utils::ReplaceAll(result, kUniformsMarker, uniformDeclarations) ||
            !utils::ReplaceAll(result, kSourceMarker, source)) {
            TF3D_LOG_ERROR("Base-shape template is missing one or more required assembly markers.");
            return {};
        }
        return result;
    }

    bool BiomeBaseShapeGenerator::Initialize(const nlohmann::json &config,
                                             const std::string &source,
                                             const std::string &shaderPath)
    {
        if (m_AppState == nullptr || m_AppState->resourceManager == nullptr) {
            TF3D_LOG_ERROR("Failed to load base-shape generator: application resources are unavailable.");
            return false;
        }
        if (!config.is_object()) {
            TF3D_LOG_ERROR("Failed to load base-shape generator: metadata is not an object.");
            return false;
        }
        if (config.value("ID", "").empty()) {
            TF3D_LOG_ERROR("Failed to load base-shape generator: metadata requires a non-empty ID.");
            return false;
        }
        if (source.empty() || shaderPath.empty()) {
            TF3D_LOG_ERROR("Failed to load base-shape generator '{}': shader source and path are required.",
                           config.value("ID", ""));
            return false;
        }

        m_ID          = config.value("ID", "");
        m_Description = config.value("Description", "");
        m_Source      = source;
        m_ShaderPath  = shaderPath;
        m_Name        = config.value("Name", "Unnamed");

        if (!m_Inspector.LoadConfig(config)) {
            return false;
        }

        std::string uniformError       = "";
        const auto uniformDeclarations = m_Inspector.GetShaderUniformDeclarations(&uniformError);
        if (!uniformDeclarations) {
            TF3D_LOG_ERROR("Cannot generate base-shape uniform declarations: {}", uniformError);
            return false;
        }

        bool templateLoaded              = false;
        const std::string templateSource = m_AppState->resourceManager->LoadShaderSource(
            "generation/base_shape/base_shape", false, &templateLoaded);
        if (!templateLoaded) {
            TF3D_LOG_ERROR("Failed to load the base-shape shader template.");
            return false;
        }

        const std::string finalSource = BuildShaderSource(templateSource, *uniformDeclarations);
        if (finalSource.empty()) {
            TF3D_LOG_ERROR("Failed to assemble base-shape shader '{}'.", m_ID);
            return false;
        }

        m_Shader = m_AppState->resourceManager->GetComputeShader("BaseShapeGen_" + m_ID, finalSource);
        if (!m_Shader.has_value()) {
            TF3D_LOG_ERROR("Failed to compile base-shape shader '{}'.", m_ID);
            return false;
        }

        m_State.Replace(State{m_Inspector.Clone()});
        return true;
    }

} // namespace tf3d::generators
