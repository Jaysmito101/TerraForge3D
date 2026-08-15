#include "Base/Base.h"
#include "Base/Shader.h"
#include "Inspector/CustomInspector.h"
#include "Inspector/CustomInspectorSnapshot.h"

namespace tf3d::inspector
{

    void CustomInspector::ApplyToShader(tf3d::base::ShaderCore &shader,
                                        const CustomInspectorValue &value,
                                        const CustomInspectorValue::ConstStoreView &stored,
                                        std::string_view uniformName,
                                        const CustomInspectorShaderOptions &options)
    {
        if (uniformName.empty()) {
            return;
        }

        switch (value.GetType()) {
            case CustomInspectorValueType::Int:
                shader.SetUniform1i(std::string(uniformName), stored.Get<int32_t>());
                break;
            case CustomInspectorValueType::Float:
                shader.SetUniform1f(std::string(uniformName), stored.Get<float>());
                break;
            case CustomInspectorValueType::Bool:
                shader.SetUniform1i(std::string(uniformName), stored.Get<bool>() ? 1 : 0);
                break;
            case CustomInspectorValueType::Vector2:
                shader.SetUniform2f(std::string(uniformName), stored.Get<glm::vec2>());
                break;
            case CustomInspectorValueType::Vector3:
                shader.SetUniform3f(std::string(uniformName), stored.Get<glm::vec3>());
                break;
            case CustomInspectorValueType::Vector4:
                shader.SetUniform4f(std::string(uniformName), stored.Get<glm::vec4>());
                break;
            case CustomInspectorValueType::FloatArray: {
                const auto values = stored.Get<std::vector<float>>();
                if (!values.empty()) {
                    shader.SetUniform1fv(std::string(uniformName), values.data(), static_cast<int>(values.size()));
                }
                break;
            }
            case CustomInspectorValueType::Texture: {
                const auto texture    = stored.Get<std::shared_ptr<tf3d::base::Texture2D>>();
                const bool hasTexture = texture != nullptr && texture->IsLoaded();
                if (hasTexture && options.textureSlot != nullptr) {
                    shader.SetUniform1i(std::string(uniformName),
                                        texture->Bind(static_cast<uint32_t>((*options.textureSlot)++)));
                }
                if (!options.presenceUniform.empty()) {
                    shader.SetUniform1i(std::string(options.presenceUniform), hasTexture ? 1 : 0);
                }
                break;
            }
            case CustomInspectorValueType::Path: {
                const auto points    = stored.Get<std::vector<glm::vec2>>();
                const int pointCount = std::min(static_cast<int>(points.size()),
                                                static_cast<int>(CustomInspectorMaxPathPoints));
                for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
                    shader.SetUniform2f(std::string(uniformName) + "[" + std::to_string(pointIndex) + "]",
                                        points[static_cast<size_t>(pointIndex)]);
                }
                shader.SetUniform1i(
                    options.pointCountUniform.empty()
                        ? std::string(uniformName) + "Count"
                        : options.pointCountUniform,
                    pointCount);
                break;
            }
            case CustomInspectorValueType::Curve: {
                if (!options.bindCurve) {
                    break;
                }
                const auto points    = stored.Get<std::vector<glm::vec2>>();
                const int pointCount = std::clamp(static_cast<int>(points.size()),
                                                  2,
                                                  static_cast<int>(CustomInspectorMaxCurvePoints));
                for (int pointIndex = 0; pointIndex < static_cast<int>(CustomInspectorMaxCurvePoints); ++pointIndex) {
                    const glm::vec2 point = pointIndex < static_cast<int>(points.size())
                                                ? points[static_cast<size_t>(pointIndex)]
                                                : glm::vec2(0.0f);
                    shader.SetUniform2f(std::string(uniformName) + "[" + std::to_string(pointIndex) + "]", point);
                }
                shader.SetUniform1i(
                    options.pointCountUniform.empty()
                        ? std::string(uniformName) + "PointCount"
                        : options.pointCountUniform,
                    pointCount);
                break;
            }
            case CustomInspectorValueType::String:
            case CustomInspectorValueType::Unknown:
            case CustomInspectorValueType::Count:
            default:
                break;
        }
    }

    void CustomInspector::ApplyToShader(tf3d::base::ShaderCore &shader, std::string_view uniformPrefix) const
    {
        for (const auto &widgetLabel : m_WidgetState.order) {
            if (!IsWidgetVisible(widgetLabel)) {
                continue;
            }

            const auto widgetIterator = m_WidgetState.byName.find(widgetLabel);
            if (widgetIterator == m_WidgetState.byName.end()) {
                continue;
            }
            const auto valuePath = PathForWidget(widgetLabel);
            if (valuePath.empty()) {
                continue;
            }

            const auto valueIterator = m_ValueState.metadata.find(valuePath);
            if (valueIterator == m_ValueState.metadata.end()) {
                continue;
            }
            const auto &uniformValue      = valueIterator->second;
            const std::string uniformName = uniformValue.IsShaderUniformConfigured()
                                                ? uniformValue.GetShaderUniformName()
                                                : std::string(uniformPrefix) + uniformValue.GetName();
            CustomInspector::ApplyToShader(shader,
                                           uniformValue,
                                           m_ValueState.dataStore.At(valuePath),
                                           uniformName);
        }
    }

    void CustomInspector::ApplyToShader(const CustomInspectorSnapshot &snapshot,
                                        tf3d::base::ShaderCore &shader,
                                        std::string_view uniformPrefix) const
    {
        for (const auto &widgetLabel : m_WidgetState.order) {
            if (!IsWidgetVisible(widgetLabel)) {
                continue;
            }

            const auto widgetIterator = m_WidgetState.byName.find(widgetLabel);
            if (widgetIterator == m_WidgetState.byName.end()) {
                continue;
            }

            const auto valuePath = PathForWidget(widgetLabel);
            if (valuePath.empty()) {
                continue;
            }

            const auto valueIterator = m_ValueState.metadata.find(valuePath);
            if (valueIterator == m_ValueState.metadata.end()) {
                continue;
            }

            const auto &uniformValue      = valueIterator->second;
            const std::string uniformName = uniformValue.IsShaderUniformConfigured()
                                                ? uniformValue.GetShaderUniformName()
                                                : std::string(uniformPrefix) + uniformValue.GetName();
            CustomInspector::ApplyToShader(shader,
                                           uniformValue,
                                           snapshot.GetDataStore().At(valuePath),
                                           uniformName);
        }
    }

} // namespace tf3d::inspector
