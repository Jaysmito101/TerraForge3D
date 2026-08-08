#include "Base/Base.h"
#include "Data/ApplicationState.h"
#include "Inspector/CustomInspector.h"
#include "Utils/JsonIncludeResolver.h"
#include "Utils/Utils.h"

#include <unordered_set>

namespace tf3d::inspector
{

    bool CustomInspector::LoadConfig(ApplicationState *appState, std::string_view inspectorName)
    {
        if (appState == nullptr) {
            TF3D_LOG_ERROR("Cannot load inspector metadata '{}' without application state", inspectorName);
            return false;
        }
        if (inspectorName.empty()) {
            TF3D_LOG_ERROR("Cannot load inspector metadata with an empty name");
            return false;
        }

        const std::string configPath = appState->constants.dataDir + PATH_SEPARATOR + "inspectors" +
                                       PATH_SEPARATOR + std::string(inspectorName) + ".json";
        const utils::JsonIncludeResolver resolver;
        std::string resolveError;
        const auto config = resolver.ResolveFile(configPath, &resolveError);
        if (!config) {
            TF3D_LOG_ERROR("Could not load inspector metadata '{}': {}", configPath, resolveError);
            return false;
        }
        if (!LoadConfig(*config)) {
            TF3D_LOG_ERROR("Could not load inspector metadata '{}'", configPath);
            return false;
        }
        return true;
    }

    bool CustomInspector::LoadConfig(const nlohmann::json &config)
    {
        Clear();
        m_Description = config.contains("Description") && config["Description"].is_string()
                            ? config["Description"].get<std::string>()
                            : "";
        if (config.contains("Schema")) {
            if (!config["Schema"].is_object()) {
                TF3D_LOG_ERROR("Inspector metadata field 'Schema' must be an object");
                return false;
            }
            m_SchemaMetadata = config["Schema"];
        }
        m_ShowResetButton = true;
        if (config.contains("ShowResetButton")) {
            if (!config["ShowResetButton"].is_boolean()) {
                TF3D_LOG_ERROR("Inspector metadata field 'ShowResetButton' must be a boolean");
                return false;
            }
            m_ShowResetButton = config["ShowResetButton"].get<bool>();
        }
        bool hasContent = false;
        try {
            const auto loadGroup = [&](const nlohmann::json &group) {
                if (group.contains("Params") && group["Params"].is_array()) {
                    hasContent = true;
                    for (const auto &parameter : group["Params"]) {
                        const auto &value = AddVairableFromConfig(parameter);
                        if (parameter.contains("Visible") && parameter["Visible"].is_boolean() && !parameter["Visible"].get<bool>())
                            continue;
                        const std::string widgetType  = parameter.value("Widget", "Input");
                        const std::string widgetLabel = parameter.value("Label", value.GetName());
                        auto &widget                  = AddWidgetFromString(widgetLabel, widgetType, value.GetName());
                        if (parameter.contains("Sensitivity"))
                            widget.SetSpeed(parameter["Sensitivity"].get<float>());
                        if (parameter.contains("Options")) {
                            const auto options = parameter["Options"].get<std::vector<std::string>>();
                            std::vector<int32_t> optionValues;
                            if (parameter.contains("OptionValues") && parameter["OptionValues"].is_array())
                                optionValues = parameter["OptionValues"].get<std::vector<int32_t>>();
                            widget.SetDropdownOptions(options, optionValues);
                        }
                        if (parameter.contains("Constraints")) {
                            const auto &constraints = parameter["Constraints"];
                            if (constraints.is_array() && constraints.size() >= 2) {
                                const float c0 = constraints[0].get<float>();
                                const float c1 = constraints[1].get<float>();
                                const float c2 = constraints.size() > 2 ? constraints[2].get<float>() : 0.0f;
                                const float c3 = constraints.size() > 3 ? constraints[3].get<float>() : 0.0f;
                                widget.SetConstraints(c0, c1, c2, c3);
                            }
                        }
                        if (parameter.contains("ShaderUniform")) {
                            if (parameter["ShaderUniform"].is_string())
                                widget.SetShaderUniformName(parameter["ShaderUniform"].get<std::string>());
                            else
                                TF3D_LOG_WARN("Inspector parameter '{}' has an invalid ShaderUniform; expected a string", value.GetName());
                        }
                        if (parameter.contains("Tooltip"))
                            widget.SetTooltip(parameter["Tooltip"].get<std::string>());
                        else if (parameter.contains("Description"))
                            widget.SetTooltip(parameter["Description"].get<std::string>());
                        if (parameter.contains("Conditions") && parameter["Conditions"].is_array()) {
                            widget.ClearCondition();
                            for (const auto &condition : parameter["Conditions"]) {
                                if (!condition.is_object())
                                    continue;
                                const std::string conditionName = condition.value("Name", condition.value("Conditional", ""));
                                if (conditionName.empty())
                                    continue;
                                if (condition.contains("Values") && condition["Values"].is_array())
                                    widget.AddRenderOnCondition(conditionName, condition["Values"].get<std::vector<int32_t>>());
                                else if (condition.contains("ConditionalValues") && condition["ConditionalValues"].is_array())
                                    widget.AddRenderOnCondition(conditionName, condition["ConditionalValues"].get<std::vector<int32_t>>());
                                else
                                    widget.AddRenderOnCondition(conditionName, {condition.value("Value", condition.value("ConditionalValue", 1))});
                            }
                        } else if (parameter.contains("Conditional")) {
                            const std::string conditionName = parameter["Conditional"].get<std::string>();
                            if (parameter.contains("ConditionalValues") && parameter["ConditionalValues"].is_array())
                                widget.SetRenderOnConditions(conditionName, parameter["ConditionalValues"].get<std::vector<int32_t>>());
                            else
                                widget.SetRenderOnCondition(conditionName, parameter.value("ConditionalValue", 1));
                        }
                    }
                }
                if (group.contains("Buttons") && group["Buttons"].is_array()) {
                    hasContent = true;
                    for (const auto &button : group["Buttons"]) {
                        if (!button.is_object())
                            continue;
                        const std::string action = button.value("Action", button.value("Name", "Action"));
                        const std::string label  = button.value("Label", action);
                        auto &widget             = AddWidget(label, CustomInspectorWidgetType::Button, action);
                        if (button.contains("Tooltip"))
                            widget.SetTooltip(button["Tooltip"].get<std::string>());
                        else if (button.contains("Description"))
                            widget.SetTooltip(button["Description"].get<std::string>());
                    }
                }
            };

            if (config.is_object())
                loadGroup(config);

            if (config.contains("Sections") && config["Sections"].is_array()) {
                for (const auto &sectionConfig : config["Sections"]) {
                    if (!sectionConfig.is_object())
                        continue;
                    const std::string name = sectionConfig.value("Name", "Section");
                    auto &section          = AddSection(
                        name,
                        sectionConfig.value("Label", name),
                        sectionConfig.value("Collapsible", false),
                        sectionConfig.value("DefaultOpen", true));
                    section.description = sectionConfig.value("Description", "");
                    BeginSection(name);
                    loadGroup(sectionConfig);
                    EndSection();
                }
            }

            if (config.contains("WidgetOrder") && config["WidgetOrder"].is_array()) {
                std::vector<std::string> orderedWidgets;
                std::unordered_set<std::string> emittedWidgets;
                const auto appendWidget = [&](const std::string &identifier) {
                    if (m_Widgets.contains(identifier) && emittedWidgets.insert(identifier).second) {
                        orderedWidgets.push_back(identifier);
                        return;
                    }
                    for (const auto &[label, widget] : m_Widgets) {
                        if (widget.m_VariableName == identifier && emittedWidgets.insert(label).second) {
                            orderedWidgets.push_back(label);
                            return;
                        }
                    }
                };
                for (const auto &identifier : config["WidgetOrder"]) {
                    if (identifier.is_string())
                        appendWidget(identifier.get<std::string>());
                }
                for (const auto &label : m_WidgetsOrder) {
                    if (emittedWidgets.insert(label).second)
                        orderedWidgets.push_back(label);
                }
                m_WidgetsOrder = std::move(orderedWidgets);
            }

            if (config.contains("Presets")) {
                const auto &presets = config["Presets"];
                if (!presets.is_array()) {
                    TF3D_LOG_ERROR("Inspector metadata field 'Presets' must be an array");
                    return false;
                }

                for (const auto &presetConfig : presets) {
                    if (!presetConfig.is_object()) {
                        TF3D_LOG_WARN("Skipping CustomInspector preset: expected an object");
                        continue;
                    }
                    if (!presetConfig.contains("Name") || !presetConfig["Name"].is_string()) {
                        TF3D_LOG_WARN("Skipping CustomInspector preset without a string Name");
                        continue;
                    }
                    const std::string name = presetConfig["Name"].get<std::string>();
                    if (name.empty() || name == "Default") {
                        TF3D_LOG_WARN("Skipping CustomInspector preset with reserved or empty Name '{}'", name);
                        continue;
                    }
                    if (std::any_of(m_Presets.begin(), m_Presets.end(), [&](const Preset &preset) {
                            return preset.name == name;
                        })) {
                        TF3D_LOG_WARN("Skipping duplicate CustomInspector preset '{}'", name);
                        continue;
                    }
                    if (!presetConfig.contains("Values") || !presetConfig["Values"].is_object()) {
                        TF3D_LOG_WARN("Skipping CustomInspector preset '{}': Values must be an object", name);
                        continue;
                    }

                    Preset preset;
                    preset.name        = name;
                    preset.label       = name;
                    preset.description = "Apply the " + name + " inspector preset.";
                    if (presetConfig.contains("Label")) {
                        if (!presetConfig["Label"].is_string()) {
                            TF3D_LOG_WARN("Skipping CustomInspector preset '{}': Label must be a string", name);
                            continue;
                        }
                        preset.label = presetConfig["Label"].get<std::string>();
                    }
                    if (presetConfig.contains("Description")) {
                        if (!presetConfig["Description"].is_string()) {
                            TF3D_LOG_WARN("Skipping CustomInspector preset '{}': Description must be a string", name);
                            continue;
                        }
                        preset.description = presetConfig["Description"].get<std::string>();
                    }
                    if (preset.label.empty())
                        preset.label = name;
                    preset.values = presetConfig["Values"];
                    if (!ApplyPresetValues(preset.values, preset.name, false))
                        continue;
                    m_Presets.push_back(std::move(preset));
                }
            }
        } catch (const std::exception &exception) {
            TF3D_LOG_ERROR("Failed to load inspector metadata: {}", exception.what());
            return false;
        }
        if (!hasContent)
            AddWidget("No parameters available", CustomInspectorWidgetType::Text);
        return true;
    }

} // namespace tf3d::inspector
