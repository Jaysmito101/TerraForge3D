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
            const auto values        = value.Store().Get<std::vector<float>>();
            const auto defaultValues = value.Store().GetDefault<std::vector<float>>();
            if (!defaultValues.empty() && values.size() != defaultValues.size()) {
                TF3D_LOG_WARN("Invalid CustomInspector value '{}' must contain {} elements", name, defaultValues.size());
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
                                           ? static_cast<float>(value.Store().Get<int32_t>())
                                           : value.Store().Get<float>();
            if (!std::isfinite(numericValue)) {
                TF3D_LOG_WARN("Invalid CustomInspector value '{}' must be finite", name);
                return false;
            }
        }

        for (const auto &[widgetName, widget] : m_WidgetState.byName) {
            if (PathForWidget(widgetName) != name ||
                (widget.m_Type != CustomInspectorWidgetType::Slider &&
                 widget.m_Type != CustomInspectorWidgetType::Drag &&
                 widget.m_Type != CustomInspectorWidgetType::Octaves))
                continue;

            const float minimum = widget.m_Constraints[0];
            const float maximum = widget.m_Constraints[1];
            if (value.GetType() == CustomInspectorValueType::FloatArray) {
                for (const auto numericValue : value.Store().Get<std::vector<float>>()) {
                    if (!validateRange(numericValue, minimum, maximum))
                        return false;
                }
            } else {
                const float numericValue = value.GetType() == CustomInspectorValueType::Int
                                               ? static_cast<float>(value.Store().Get<int32_t>())
                                               : value.Store().Get<float>();
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

        for (const auto &sectionName : m_SectionState.order) {
            const auto section = m_SectionState.byName.find(sectionName);
            if (section == m_SectionState.byName.end())
                continue;

            SerializerNode sectionState = CreateSerializerNode();
            for (const auto &widgetLabel : m_WidgetState.order) {
                const auto widgetSection = m_SectionState.widgetSections.find(widgetLabel);
                if (widgetSection == m_SectionState.widgetSections.end() || widgetSection->second != sectionName)
                    continue;
                const auto widget = m_WidgetState.byName.find(widgetLabel);
                if (widget == m_WidgetState.byName.end() || widget->second.m_VariableName.empty())
                    continue;
                const auto value = m_ValueState.metadata.find(PathForWidget(widgetLabel));
                if (value == m_ValueState.metadata.end() || !savedVariables.insert(value->first).second)
                    continue;
                saveValue(sectionState, value->first, value->second);
            }
            state->Set(sectionName, sectionState);
        }

        for (const auto &[name, value] : m_ValueState.metadata) {
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

        bool valid                          = true;
        CustomInspectorDataStore candidates = m_ValueState.dataStore;
        const auto findValueName            = [&](const std::string &serializedName,
                                       const std::string &sectionName) -> std::string {
            if (sectionName.empty()) {
                const auto direct = m_ValueState.metadata.find(serializedName);
                if (direct != m_ValueState.metadata.end() && direct->second.GetSerializedName() == serializedName)
                    return direct->first;
            }

            for (const auto &widgetLabel : m_WidgetState.order) {
                const auto widgetSection = m_SectionState.widgetSections.find(widgetLabel);
                if (!sectionName.empty() &&
                    (widgetSection == m_SectionState.widgetSections.end() || widgetSection->second != sectionName))
                    continue;
                const auto widget = m_WidgetState.byName.find(widgetLabel);
                if (widget == m_WidgetState.byName.end() || widget->second.m_VariableName.empty())
                    continue;
                const auto value = m_ValueState.metadata.find(PathForWidget(widgetLabel));
                if (value != m_ValueState.metadata.end() && value->second.GetSerializedName() == serializedName)
                    return value->first;
            }
            return {};
        };

        auto loadValue = [&](const std::string &name,
                             const std::string &sectionName,
                             SerializerNode source) {
            const std::string valueName = findValueName(name, sectionName);
            const auto existing         = m_ValueState.metadata.find(valueName);
            if (existing == m_ValueState.metadata.end()) {
                TF3D_LOG_WARN("Invalid CustomInspector state field '{}'", name);
                valid = false;
                return;
            }

            CustomInspectorValue candidate = existing->second;
            candidate.BindDataStore(&candidates, existing->first);
            if (!candidate.ReadStateValue(source, name)) {
                TF3D_LOG_WARN("Invalid CustomInspector state type for '{}'", name);
                valid = false;
                return;
            }
            if (!ValidateValue(valueName, candidate)) {
                valid = false;
                return;
            }
        };

        for (const auto &key : node->GetKeys()) {
            const auto section = m_SectionState.byName.find(key);
            if (section != m_SectionState.byName.end()) {
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
        if (valid) {
            m_ValueState.dataStore      = std::move(candidates);
            m_PresetState.selectedIndex = -1;
        }
        return valid;
    }

    void CustomInspector::ResetVisible()
    {
        std::unordered_set<std::string> resetValues;
        for (const auto &widgetLabel : m_WidgetState.order) {
            if (!IsWidgetVisible(widgetLabel))
                continue;
            const auto widget = m_WidgetState.byName.find(widgetLabel);
            if (widget == m_WidgetState.byName.end() || widget->second.m_VariableName.empty())
                continue;
            const auto valuePath = PathForWidget(widgetLabel);
            if (resetValues.insert(valuePath).second) {
                const auto value = m_ValueState.metadata.find(valuePath);
                if (value != m_ValueState.metadata.end())
                    value->second.Store().Reset();
            }
        }
        m_PresetState.selectedIndex            = 0;
        m_InteractionState.lastChangedVariable = "Preset";
        m_InteractionState.lastAction.clear();
    }

} // namespace tf3d::inspector
