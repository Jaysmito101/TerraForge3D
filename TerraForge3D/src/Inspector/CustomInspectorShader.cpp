#include "Base/Base.h"
#include "Base/Shader.h"
#include "Inspector/CustomInspector.h"

namespace tf3d::inspector
{

    void CustomInspector::ApplyToShader(tf3d::base::ShaderCore &shader, std::string_view uniformPrefix) const
    {
        for (const auto &widgetLabel : m_WidgetsOrder) {
            if (!IsWidgetVisible(widgetLabel))
                continue;

            const auto widgetIterator = m_Widgets.find(widgetLabel);
            if (widgetIterator == m_Widgets.end())
                continue;
            const auto valuePath = PathForWidget(widgetLabel);
            if (valuePath.empty())
                continue;

            const auto valueIterator = m_Values.find(valuePath);
            if (valueIterator == m_Values.end())
                continue;
            const auto &uniformValue      = valueIterator->second;
            const std::string uniformName = uniformValue.IsShaderUniformConfigured()
                                                ? uniformValue.GetShaderUniformName()
                                                : std::string(uniformPrefix) + uniformValue.GetName();
            if (uniformName.empty())
                continue;

            switch (uniformValue.GetType()) {
                case CustomInspectorValueType::Int:
                    shader.SetUniform1i(uniformName, uniformValue.template Get<int32_t>());
                    break;
                case CustomInspectorValueType::Float:
                    shader.SetUniform1f(uniformName, uniformValue.template Get<float>());
                    break;
                case CustomInspectorValueType::Bool:
                    shader.SetUniform1i(uniformName, uniformValue.template Get<bool>() ? 1 : 0);
                    break;
                case CustomInspectorValueType::Vector2:
                    shader.SetUniform2f(uniformName, uniformValue.template Get<glm::vec2>());
                    break;
                case CustomInspectorValueType::Vector3:
                    shader.SetUniform3f(uniformName, uniformValue.template Get<glm::vec3>());
                    break;
                case CustomInspectorValueType::Vector4: {
                    const glm::vec4 vector = uniformValue.template Get<glm::vec4>();
                    shader.SetUniform4f(uniformName, vector.x, vector.y, vector.z, vector.w);
                    break;
                }
                case CustomInspectorValueType::FloatArray: {
                    const auto values = uniformValue.template Get<std::vector<float>>();
                    if (!values.empty())
                        shader.SetUniform1fv(uniformName, values.data(), static_cast<int>(values.size()));
                    break;
                }
                case CustomInspectorValueType::Path: {
                    const auto points    = uniformValue.template Get<std::vector<glm::vec2>>();
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
    }

} // namespace tf3d::inspector
