#include "Base/Base.h"
#include "Inspector/CustomInspector.h"
#include "Utils/Utils.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace tf3d::inspector
{

    bool CustomInspector::ValidateValue(const std::string &name, const CustomInspectorValue &value) const
    {
        if (value.GetType() != CustomInspectorValueType::Int &&
            value.GetType() != CustomInspectorValueType::Float &&
            value.GetType() != CustomInspectorValueType::FloatArray)
            return true;

        const auto validateRange = [&](float numericValue, float minimum, float maximum) {
            if (!std::isfinite(numericValue)) {
                TF3D_LOG_WARN("Invalid CustomInspector value '{}' must be finite", name);
                return false;
            }
            if (minimum != 0.0f || maximum != 0.0f) {
                if (numericValue < minimum || numericValue > maximum) {
                    TF3D_LOG_WARN(
                        "Invalid CustomInspector value '{}' must be between {} and {}",
                        name,
                        minimum,
                        maximum);
                    return false;
                }
            }
            return true;
        };

        if (value.GetType() == CustomInspectorValueType::FloatArray) {
            const auto values = value.Get<std::vector<float>>();
            if (!value.m_DefaultFloatArrayValue.empty() && values.size() != value.m_DefaultFloatArrayValue.size()) {
                TF3D_LOG_WARN("Invalid CustomInspector value '{}' must contain {} elements", name, value.m_DefaultFloatArrayValue.size());
                return false;
            }
            for (const auto numericValue : values) {
                if (!std::isfinite(numericValue)) {
                    TF3D_LOG_WARN("Invalid CustomInspector value '{}' must be finite", name);
                    return false;
                }
            }
        } else {
            const float numericValue = value.GetType() == CustomInspectorValueType::Int
                                           ? static_cast<float>(value.Get<int32_t>())
                                           : value.Get<float>();
            if (!std::isfinite(numericValue)) {
                TF3D_LOG_WARN("Invalid CustomInspector value '{}' must be finite", name);
                return false;
            }
        }

        for (const auto &[widgetName, widget] : m_Widgets) {
            if (PathForWidget(widgetName) != name ||
                (widget.m_Type != CustomInspectorWidgetType::Slider &&
                 widget.m_Type != CustomInspectorWidgetType::Drag &&
                 widget.m_Type != CustomInspectorWidgetType::Octaves))
                continue;

            const float minimum = widget.m_Constratins[0];
            const float maximum = widget.m_Constratins[1];
            if (value.GetType() == CustomInspectorValueType::FloatArray) {
                for (const auto numericValue : value.Get<std::vector<float>>()) {
                    if (!validateRange(numericValue, minimum, maximum))
                        return false;
                }
            } else {
                const float numericValue = value.GetType() == CustomInspectorValueType::Int
                                               ? static_cast<float>(value.Get<int32_t>())
                                               : value.Get<float>();
                if (!validateRange(numericValue, minimum, maximum))
                    return false;
            }
        }
        return true;
    }

    SerializerNode CustomInspector::SaveState() const
    {
        return SaveState({});
    }

    SerializerNode CustomInspector::SaveState(std::initializer_list<std::string_view> excludedValues) const
    {
        SerializerNode state = CreateSerializerNode();
        std::unordered_set<std::string> savedVariables;
        std::unordered_set<std::string> excluded;
        for (const auto value : excludedValues)
            excluded.emplace(value);

        auto saveValue = [&](SerializerNode target,
                             const std::string &name,
                             const CustomInspectorValue &value) {
            if (excluded.contains(name) || excluded.contains(value.GetName()) || excluded.contains(value.GetSerializedName()))
                return;
            if (!value.WriteStateValue(target, value.GetSerializedName()))
                TF3D_LOG_WARN("Skipping unsupported CustomInspector state field '{}'", name);
        };

        for (const auto &sectionName : m_SectionsOrder) {
            const auto section = m_Sections.find(sectionName);
            if (section == m_Sections.end())
                continue;

            SerializerNode sectionState = CreateSerializerNode();
            for (const auto &widgetLabel : m_WidgetsOrder) {
                const auto widgetSection = m_WidgetSections.find(widgetLabel);
                if (widgetSection == m_WidgetSections.end() || widgetSection->second != sectionName)
                    continue;
                const auto widget = m_Widgets.find(widgetLabel);
                if (widget == m_Widgets.end() || widget->second.m_VariableName.empty())
                    continue;
                const auto value = m_Values.find(PathForWidget(widgetLabel));
                if (value == m_Values.end() || !savedVariables.insert(value->first).second)
                    continue;
                saveValue(sectionState, value->first, value->second);
            }
            state->Set(sectionName, sectionState);
        }

        for (const auto &[name, value] : m_Values) {
            if (savedVariables.insert(name).second)
                saveValue(state, name, value);
        }
        return state;
    }

    bool CustomInspector::LoadState(SerializerNode node)
    {
        if (!node) {
            TF3D_LOG_ERROR("Cannot load CustomInspector state from an empty serializer node");
            return false;
        }

        bool valid               = true;
        const auto findValueName = [&](const std::string &serializedName,
                                       const std::string &sectionName) -> std::string {
            if (sectionName.empty()) {
                const auto direct = m_Values.find(serializedName);
                if (direct != m_Values.end() && direct->second.GetSerializedName() == serializedName)
                    return direct->first;
            }

            for (const auto &widgetLabel : m_WidgetsOrder) {
                const auto widgetSection = m_WidgetSections.find(widgetLabel);
                if (!sectionName.empty() &&
                    (widgetSection == m_WidgetSections.end() || widgetSection->second != sectionName))
                    continue;
                const auto widget = m_Widgets.find(widgetLabel);
                if (widget == m_Widgets.end() || widget->second.m_VariableName.empty())
                    continue;
                const auto value = m_Values.find(PathForWidget(widgetLabel));
                if (value != m_Values.end() && value->second.GetSerializedName() == serializedName)
                    return value->first;
            }
            return {};
        };

        auto loadValue = [&](const std::string &name,
                             const std::string &sectionName,
                             SerializerNode source) {
            const std::string valueName = findValueName(name, sectionName);
            const auto existing         = m_Values.find(valueName);
            if (existing == m_Values.end()) {
                TF3D_LOG_WARN("Invalid CustomInspector state field '{}'", name);
                valid = false;
                return;
            }

            CustomInspectorValue candidate = existing->second;
            if (!candidate.ReadStateValue(source, name)) {
                TF3D_LOG_WARN("Invalid CustomInspector state type for '{}'", name);
                valid = false;
                return;
            }
            if (!ValidateValue(valueName, candidate)) {
                valid = false;
                return;
            }
            existing->second = std::move(candidate);
        };

        for (const auto &key : node->GetKeys()) {
            const auto section = m_Sections.find(key);
            if (section != m_Sections.end()) {
                const SerializerNode sectionState = node->Get<SerializerNode>(key);
                if (!sectionState) {
                    TF3D_LOG_WARN("Invalid CustomInspector section '{}': expected an object", key);
                    valid = false;
                    continue;
                }
                for (const auto &field : sectionState->GetKeys())
                    loadValue(field, key, sectionState);
                continue;
            }
            loadValue(key, {}, node);
        }
        m_SelectedPreset = -1;
        return valid;
    }

    void CustomInspector::ResetVisible()
    {
        std::unordered_set<std::string> resetValues;
        for (const auto &widgetLabel : m_WidgetsOrder) {
            if (!IsWidgetVisible(widgetLabel))
                continue;
            const auto widget = m_Widgets.find(widgetLabel);
            if (widget == m_Widgets.end() || widget->second.m_VariableName.empty())
                continue;
            const auto valuePath = PathForWidget(widgetLabel);
            if (resetValues.insert(valuePath).second) {
                const auto value = m_Values.find(valuePath);
                if (value != m_Values.end())
                    value->second.ResetValue();
            }
        }
        m_SelectedPreset      = 0;
        m_LastChangedVariable = "Preset";
        m_LastAction.clear();
    }

} // namespace tf3d::inspector
