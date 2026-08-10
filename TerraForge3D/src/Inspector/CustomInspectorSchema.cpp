#include "Base/Base.h"
#include "Inspector/CustomInspector.h"

#include <algorithm>
#include <unordered_set>

namespace tf3d::inspector
{

    nlohmann::json CustomInspector::BuildSchema() const
    {
        const auto vectorSchema = [](int dimensions) {
            nlohmann::json schema = {
                {"type", "object"},
                {"additionalProperties", false},
                {"properties", nlohmann::json::object()},
                {"required", nlohmann::json::array()}};
            const char *names[] = {"X", "Y", "Z", "W"};
            for (int index = 0; index < dimensions; ++index) {
                schema["properties"][names[index]] = {{"type", "number"}};
                schema["required"].push_back(names[index]);
            }
            return schema;
        };

        const auto buildValueSchema = [&](const CustomInspectorValue &value,
                                          const CustomInspectorWidget *widget) {
            nlohmann::json schema = nlohmann::json::object();
            switch (value.GetType()) {
                case CustomInspectorValueType::Int:
                    schema["type"] = "integer";
                    break;
                case CustomInspectorValueType::Float:
                    schema["type"] = "number";
                    break;
                case CustomInspectorValueType::Bool:
                    schema["type"] = "boolean";
                    break;
                case CustomInspectorValueType::String:
                case CustomInspectorValueType::Texture:
                    schema["type"] = "string";
                    break;
                case CustomInspectorValueType::Vector2:
                    schema = vectorSchema(2);
                    break;
                case CustomInspectorValueType::Vector3:
                    schema = vectorSchema(3);
                    break;
                case CustomInspectorValueType::Vector4:
                    schema = vectorSchema(4);
                    break;
                case CustomInspectorValueType::FloatArray:
                    schema = {
                        {"type", "array"},
                        {"items", {{"type", "number"}}}};
                    break;
                case CustomInspectorValueType::Path:
                case CustomInspectorValueType::Curve:
                    schema = {
                        {"type", "array"},
                        {"items", vectorSchema(2)}};
                    break;
                case CustomInspectorValueType::Unknown:
                default:
                    schema["type"] = "string";
                    break;
            }

            if (widget != nullptr) {
                if (!widget->m_Label.empty())
                    schema["title"] = widget->m_Label;
                if (!widget->m_Tooltip.empty())
                    schema["description"] = widget->m_Tooltip;
                if ((widget->m_Type == CustomInspectorWidgetType::Slider ||
                     widget->m_Type == CustomInspectorWidgetType::Drag) &&
                    (widget->m_Constraints[0] != 0.0f || widget->m_Constraints[1] != 0.0f)) {
                    schema["minimum"] = widget->m_Constraints[0];
                    schema["maximum"] = widget->m_Constraints[1];
                }
                if (widget->m_Type == CustomInspectorWidgetType::Octaves &&
                    value.GetType() == CustomInspectorValueType::FloatArray &&
                    (widget->m_Constraints[0] != 0.0f || widget->m_Constraints[1] != 0.0f)) {
                    schema["items"]["minimum"] = widget->m_Constraints[0];
                    schema["items"]["maximum"] = widget->m_Constraints[1];
                }
                if (widget->m_Type == CustomInspectorWidgetType::Dropdown &&
                    !widget->m_DropdownOptions.empty()) {
                    schema["type"]        = "integer";
                    schema["enum"]        = nlohmann::json::array();
                    schema["x-enumNames"] = nlohmann::json::array();
                    for (size_t index = 0; index < widget->m_DropdownOptions.size(); ++index) {
                        const int32_t value = widget->m_DropdownValues.size() == widget->m_DropdownOptions.size()
                                                  ? widget->m_DropdownValues[index]
                                                  : static_cast<int32_t>(index);
                        schema["enum"].push_back(value);
                        schema["x-enumNames"].push_back(widget->m_DropdownOptions[index]);
                    }
                }
            }
            return schema;
        };

        nlohmann::json schema = {
            {"type", "object"},
            {"additionalProperties", false},
            {"properties", nlohmann::json::object()}};
        std::unordered_set<std::string> emitted;

        auto addVariable = [&](nlohmann::json &target,
                               const std::string &name,
                               const CustomInspectorWidget *widget) {
            const auto value = m_Values.find(name);
            if (value == m_Values.end() || !emitted.insert(name).second)
                return;
            target["properties"][value->second.GetSerializedName()] = buildValueSchema(value->second, widget);
        };

        for (const auto &sectionName : m_SectionsOrder) {
            const auto section = m_Sections.find(sectionName);
            if (section == m_Sections.end())
                continue;
            nlohmann::json sectionSchema = {
                {"type", "object"},
                {"additionalProperties", false},
                {"properties", nlohmann::json::object()}};
            if (!section->second.label.empty())
                sectionSchema["title"] = section->second.label;
            if (!section->second.description.empty()) {
                sectionSchema["description"] = section->second.description;
            } else if (!section->second.label.empty()) {
                sectionSchema["description"] = section->second.label;
            }
            const auto customData = m_SectionCustomData.find(sectionName);
            if (customData != m_SectionCustomData.end() && !customData->second.is_null() && !customData->second.empty())
                sectionSchema["x-customData"] = customData->second;

            for (const auto &widgetLabel : m_WidgetsOrder) {
                const auto widgetSection = m_WidgetSections.find(widgetLabel);
                if (widgetSection == m_WidgetSections.end() || widgetSection->second != sectionName)
                    continue;
                const auto widget = m_Widgets.find(widgetLabel);
                if (widget != m_Widgets.end() && !widget->second.m_VariableName.empty())
                    addVariable(sectionSchema, PathForWidget(widgetLabel), &widget->second);
            }
            schema["properties"][sectionName] = sectionSchema;
        }

        for (const auto &widgetLabel : m_WidgetsOrder) {
            const auto widget = m_Widgets.find(widgetLabel);
            if (widget == m_Widgets.end() || widget->second.m_VariableName.empty())
                continue;
            if (!m_WidgetSections.contains(widgetLabel))
                addVariable(schema, PathForWidget(widgetLabel), &widget->second);
        }
        for (const auto &[name, value] : m_Values)
            addVariable(schema, name, nullptr);

        if (!m_Presets.empty()) {
            schema["Presets"] = nlohmann::json::array();
            schema["Presets"].push_back({{"Name", "Default"},
                                         {"Label", "Default"},
                                         {"Description", "Reset all inspector values to their schema defaults."},
                                         {"Values", nlohmann::json::object()}});
            for (const auto &preset : m_Presets) {
                nlohmann::json presetSchema = {
                    {"Name", preset.name},
                    {"Label", preset.label},
                    {"Description", preset.description},
                    {"Values", preset.values}};
                schema["Presets"].push_back(std::move(presetSchema));
            }
        }

        const auto mergeSchema = [](nlohmann::json &target, const nlohmann::json &source, const auto &merge) -> void {
            if (!target.is_object() || !source.is_object()) {
                target = source;
                return;
            }
            for (const auto &[key, value] : source.items()) {
                if (target.contains(key))
                    merge(target[key], value, merge);
                else
                    target[key] = value;
            }
        };
        mergeSchema(schema, m_SchemaMetadata, mergeSchema);
        return schema;
    }

} // namespace tf3d::inspector
