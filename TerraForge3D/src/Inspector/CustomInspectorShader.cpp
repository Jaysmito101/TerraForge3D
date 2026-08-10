#include "Base/Base.h"
#include "Base/Shader.h"
#include "Inspector/CustomInspector.h"
#include "Inspector/CustomInspectorSnapshot.h"

#include <algorithm>

namespace tf3d::inspector
{

    namespace
    {
        void ApplyValueToShader(tf3d::base::ShaderCore &shader,
                                const CustomInspectorValue &value,
                                const CustomInspectorDataStore &dataStore,
                                std::string_view path,
                                std::string_view uniformPrefix)
        {
            const std::string uniformName = value.IsShaderUniformConfigured()
                                                ? value.GetShaderUniformName()
                                                : std::string(uniformPrefix) + value.GetName();
            if (uniformName.empty())
                return;

            const auto stored = dataStore.At(path);
            switch (value.GetType()) {
                case CustomInspectorValueType::Int:
                    shader.SetUniform1i(uniformName, stored.Get<int32_t>());
                    break;
                case CustomInspectorValueType::Float:
                    shader.SetUniform1f(uniformName, stored.Get<float>());
                    break;
                case CustomInspectorValueType::Bool:
                    shader.SetUniform1i(uniformName, stored.Get<bool>() ? 1 : 0);
                    break;
                case CustomInspectorValueType::Vector2:
                    shader.SetUniform2f(uniformName, stored.Get<glm::vec2>());
                    break;
                case CustomInspectorValueType::Vector3:
                    shader.SetUniform3f(uniformName, stored.Get<glm::vec3>());
                    break;
                case CustomInspectorValueType::Vector4: {
                    const glm::vec4 vector = stored.Get<glm::vec4>();
                    shader.SetUniform4f(uniformName, vector.x, vector.y, vector.z, vector.w);
                    break;
                }
                case CustomInspectorValueType::FloatArray: {
                    const auto values = stored.Get<std::vector<float>>();
                    if (!values.empty())
                        shader.SetUniform1fv(uniformName, values.data(), static_cast<int>(values.size()));
                    break;
                }
                case CustomInspectorValueType::Path: {
                    const auto points    = stored.Get<std::vector<glm::vec2>>();
                    const int pointCount = std::min(static_cast<int>(points.size()), static_cast<int>(CustomInspectorMaxPathPoints));
                    for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex)
                        shader.SetUniform2f(uniformName + "[" + std::to_string(pointIndex) + "]", points[pointIndex]);
                    shader.SetUniform1i(uniformName + "Count", pointCount);
                    break;
                }
                case CustomInspectorValueType::String:
                case CustomInspectorValueType::Texture:
                case CustomInspectorValueType::Curve:
                case CustomInspectorValueType::Unknown:
                case CustomInspectorValueType::Count:
                default:
                    break;
            }
        }
    } // namespace

    void CustomInspector::ApplyToShader(tf3d::base::ShaderCore &shader, std::string_view uniformPrefix) const
    {
        for (const auto &widgetLabel : m_WidgetState.order) {
            if (!IsWidgetVisible(widgetLabel))
                continue;

            const auto widgetIterator = m_WidgetState.byName.find(widgetLabel);
            if (widgetIterator == m_WidgetState.byName.end())
                continue;
            const auto valuePath = PathForWidget(widgetLabel);
            if (valuePath.empty())
                continue;

            const auto valueIterator = m_ValueState.metadata.find(valuePath);
            if (valueIterator == m_ValueState.metadata.end())
                continue;
            const auto &uniformValue = valueIterator->second;
            ApplyValueToShader(shader, uniformValue, m_ValueState.dataStore, valuePath, uniformPrefix);
        }
    }

    void CustomInspector::ApplyToShader(const CustomInspectorSnapshot &snapshot,
                                        tf3d::base::ShaderCore &shader,
                                        std::string_view uniformPrefix) const
    {
        for (const auto &[path, value] : m_ValueState.metadata) {
            ApplyValueToShader(shader, value, snapshot.GetDataStore(), path, uniformPrefix);
        }
    }

} // namespace tf3d::inspector
