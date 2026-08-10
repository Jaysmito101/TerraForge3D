#include "Inspector/CustomInspector.h"
#include "Base/Base.h"
#include "Utils/Utils.h"

#include <algorithm>
#include <optional>

namespace tf3d::inspector
{

    CustomInspector::CustomInspector()
    {
        m_InteractionState.id = GenerateId(16);
        AddWidget("Separator", CustomInspectorWidget(CustomInspectorWidgetType::Separator));
        AddWidget("NewLine", CustomInspectorWidget(CustomInspectorWidgetType::NewLine));
    }

    CustomInspector::~CustomInspector() = default;

    void CustomInspector::Reset()
    {
        for (auto &entry : m_ValueState.metadata)
            entry.second.Store().Reset();
        m_PresetState.selectedIndex            = 0;
        m_InteractionState.lastChangedVariable = "Preset";
        m_InteractionState.lastAction.clear();
    }

    void CustomInspector::Clear()
    {
        m_ValueState.metadata.clear();
        m_ValueState.dataStore.Clear();
        m_WidgetState.byName.clear();
        m_WidgetState.order.clear();
        m_SectionState.byName.clear();
        m_SectionState.order.clear();
        m_SectionState.widgetSections.clear();
        m_SectionState.customData.clear();
        m_SectionState.selectionValues.clear();
        m_PresetState.entries.clear();
        m_SectionState.currentName.clear();
        m_ConfigState.description.clear();
        m_ConfigState.schemaMetadata = nlohmann::json::object();
        m_InteractionState.lastChangedVariable.clear();
        m_InteractionState.lastAction.clear();
        m_PresetState.selectedIndex = 0;
    }

    InspectorScope CustomInspector::Root()
    {
        return InspectorScope(*this, {});
    }

    ConstInspectorScope CustomInspector::Root() const
    {
        return ConstInspectorScope(*this, {});
    }

    CustomInspectorSnapshot CustomInspector::Clone() const
    {
        return CustomInspectorSnapshot(*this);
    }

    CustomInspectorSnapshot::CustomInspectorSnapshot(const CustomInspector &inspector)
        : m_DataStore(inspector.GetDataStore().Clone())
    {
    }

    SnapshotScope CustomInspectorSnapshot::Root() const
    {
        return SnapshotScope(*this, {});
    }

    CustomInspectorSection &CustomInspector::AddSection(const std::string &name,
                                                        const std::string &label,
                                                        bool collapsible,
                                                        bool defaultOpen)
    {
        auto [it, inserted] = m_SectionState.byName.emplace(name, CustomInspectorSection{});
        if (inserted) {
            it->second.name = name;
            m_SectionState.order.push_back(name);
        }
        it->second.label       = label.empty() ? name : label;
        it->second.collapsible = collapsible;
        it->second.defaultOpen = defaultOpen;
        return it->second;
    }

    void CustomInspector::BeginSection(const std::string &name)
    {
        if (!HasSection(name))
            AddSection(name);
        m_SectionState.currentName = name;
    }

    void CustomInspector::EndSection()
    {
        m_SectionState.currentName.clear();
    }

    bool CustomInspector::HasSection(const std::string &name) const
    {
        return m_SectionState.byName.find(name) != m_SectionState.byName.end();
    }

    CustomInspectorSection &CustomInspector::GetSection(const std::string &name)
    {
        if (!HasSection(name))
            return AddSection(name);
        return m_SectionState.byName.at(name);
    }

    CustomInspectorValue *CustomInspector::FindExactValue(std::string_view path)
    {
        const auto value = m_ValueState.metadata.find(std::string(path));
        return value == m_ValueState.metadata.end() ? nullptr : &value->second;
    }

    const CustomInspectorValue *CustomInspector::FindExactValue(std::string_view path) const
    {
        const auto value = m_ValueState.metadata.find(std::string(path));
        return value == m_ValueState.metadata.end() ? nullptr : &value->second;
    }

    bool CustomInspector::RemoveExactValue(std::string_view path)
    {
        const std::string key(path);
        const bool removed = m_ValueState.metadata.erase(key) != 0;
        m_ValueState.dataStore.Remove(key);
        return removed;
    }

    CustomInspectorValue &CustomInspector::AddVariable(const std::string &name, const CustomInspectorValue &value)
    {
        const std::string valueKey = m_SectionState.currentName.empty() ? name : m_SectionState.currentName + "." + name;
        m_ValueState.dataStore.Ensure(valueKey, value.GetType());
        m_ValueState.metadata[valueKey] = value;
        m_ValueState.metadata[valueKey].BindDataStore(&m_ValueState.dataStore, valueKey);
        return m_ValueState.metadata[valueKey];
    }

    CustomInspectorValue &CustomInspector::AddPathVariable(const std::string &name,
                                                           const std::array<glm::vec2, CustomInspectorMaxPathPoints> &defaultPoints, int defaultPointCount)
    {
        CustomInspectorValue value(CustomInspectorValueType::Path);
        auto &stored         = AddVariable(name, value);
        const int pointCount = glm::clamp(defaultPointCount, 1, static_cast<int>(CustomInspectorMaxPathPoints));
        std::vector<glm::vec2> points;
        points.reserve(static_cast<size_t>(pointCount));
        for (int index = 0; index < pointCount; ++index)
            points.push_back(defaultPoints[static_cast<size_t>(index)]);
        stored.Store().SetDefault(std::move(points));
        return stored;
    }

    CustomInspectorValue &CustomInspector::AddCurveVariable(const std::string &name,
                                                            const std::array<glm::vec2, CustomInspectorMaxCurvePoints> &defaultPoints, int defaultPointCount)
    {
        CustomInspectorValue value(CustomInspectorValueType::Curve);
        const int pointCount = glm::clamp(defaultPointCount, 2, static_cast<int>(CustomInspectorMaxCurvePoints));
        std::vector<glm::vec2> points;
        points.reserve(static_cast<size_t>(pointCount));
        for (int index = 0; index < pointCount; ++index) {
            points.push_back(defaultPoints[static_cast<size_t>(index)]);
        }
        if (points[0].x < 0.0f) {
            points[0] = glm::vec2(0.0f, 0.0f);
        }
        if (points[1].x < 0.0f || points[1].x <= points[0].x)
            points[1] = glm::vec2(1.0f, 1.0f);
        auto &stored = AddVariable(name, value);
        if (auto *curve = stored.Store().Edit<CustomInspectorCurveData>(); curve != nullptr)
            curve->points.fill(glm::vec2(-1.0f));
        stored.Store().SetDefault(std::move(points));
        return stored;
    }

    CustomInspectorValue &CustomInspector::AddVairableFromConfig(const nlohmann::json &config)
    {
        std::string name                 = config.contains("Name") ? config["Name"].get<std::string>() : "Unnamed";
        const std::string serializedName = config.value("SerializedName", "");
        std::string valueTypeName        = "Float";
        if (config.contains("Type"))
            valueTypeName = config["Type"];
        auto valueType            = CustomInspectorValue::CustomInspectorValueTypeFromString(valueTypeName);
        bool hasDefaultValue      = config.contains("Default");
        const auto configureValue = [&](CustomInspectorValue &value) -> CustomInspectorValue & {
            value.m_SerializedName = serializedName;
            return value;
        };
        switch (valueType) {
            case CustomInspectorValueType::Int: {
                auto &var = Add<int32_t>(name, hasDefaultValue ? config["Default"].get<int32_t>() : 0);
                return configureValue(var);
            }
            case CustomInspectorValueType::Float: {
                auto &var = Add<float>(name, hasDefaultValue ? config["Default"].get<float>() : 0.0f);
                return configureValue(var);
            }
            case CustomInspectorValueType::Bool: {
                auto &var = Add<bool>(name, hasDefaultValue ? config["Default"].get<bool>() : false);
                return configureValue(var);
            }
            case CustomInspectorValueType::String: {
                auto &var = Add<std::string>(name, hasDefaultValue ? config["Default"].get<std::string>() : "");
                return configureValue(var);
            }
            case CustomInspectorValueType::Vector2: {
                auto &var = Add<glm::vec2>(name, hasDefaultValue ? glm::vec2(config["Default"][0].get<float>(), config["Default"][1].get<float>()) : glm::vec2(0.0f));
                return configureValue(var);
            }
            case CustomInspectorValueType::Vector3: {
                auto &var = Add<glm::vec3>(name, hasDefaultValue ? glm::vec3(config["Default"][0].get<float>(), config["Default"][1].get<float>(), config["Default"][2].get<float>()) : glm::vec3(0.0f));
                return configureValue(var);
            }
            case CustomInspectorValueType::Vector4: {
                auto &var = Add<glm::vec4>(name, hasDefaultValue ? glm::vec4(config["Default"][0].get<float>(), config["Default"][1].get<float>(), config["Default"][2].get<float>(), config["Default"][3].get<float>()) : glm::vec4(0.0f));
                return configureValue(var);
            }
            case CustomInspectorValueType::FloatArray: {
                std::vector<float> values;
                if (hasDefaultValue && config["Default"].is_array())
                    values = config["Default"].get<std::vector<float>>();
                if (values.empty())
                    values.resize(std::max(config.value("Count", 1), 1), 0.0f);
                auto &var = Add<std::vector<float>>(name, std::move(values));
                return configureValue(var);
            }
            case CustomInspectorValueType::Texture: {
                const bool loadAs16Bit                    = config.value("BitDepth", 8) >= 16;
                std::shared_ptr<Texture2D> defaultTexture = nullptr;
                if (hasDefaultValue && config["Default"].is_string()) {
                    const std::string path = config["Default"].get<std::string>();
                    if (!path.empty() && path != "null")
                        defaultTexture = std::make_shared<Texture2D>(path, true, false, loadAs16Bit);
                }
                auto &var                = Add<std::shared_ptr<Texture2D>>(name, defaultTexture);
                var.m_TextureLoadAs16Bit = loadAs16Bit;
                return configureValue(var);
            }
            case CustomInspectorValueType::Path: {
                std::array<glm::vec2, CustomInspectorMaxPathPoints> points{};
                int pointCount = config.value("PointCount", 2);
                if (hasDefaultValue && config["Default"].is_array()) {
                    pointCount = config.value("PointCount", static_cast<int>(config["Default"].size()));
                    for (size_t index = 0; index < config["Default"].size() && index < CustomInspectorMaxPathPoints; ++index) {
                        const auto &point = config["Default"][index];
                        if (!point.is_array() || point.size() < 2)
                            continue;
                        points[index] = glm::vec2(point[0].get<float>(), point[1].get<float>());
                    }
                }
                auto &var = AddPathVariable(name, points, pointCount);
                return configureValue(var);
            }
            case CustomInspectorValueType::Curve: {
                std::array<glm::vec2, CustomInspectorMaxCurvePoints> points;
                points.fill(glm::vec2(-1.0f));
                int pointCount = config.value("PointCount", 2);
                if (hasDefaultValue && config["Default"].is_array()) {
                    pointCount = config.value("PointCount", static_cast<int>(config["Default"].size()));
                    for (size_t index = 0; index < config["Default"].size() && index < CustomInspectorMaxCurvePoints; ++index) {
                        const auto &point = config["Default"][index];
                        if (!point.is_array() || point.size() < 2)
                            continue;
                        points[index] = glm::vec2(point[0].get<float>(), point[1].get<float>());
                    }
                } else {
                    points[0] = glm::vec2(0.0f, 0.0f);
                    points[1] = glm::vec2(1.0f, 1.0f);
                }
                auto &var = AddCurveVariable(name, points, pointCount);
                return configureValue(var);
            }
            default:
                throw std::runtime_error("Unknown value type");
        }
        throw std::runtime_error("Unknown value type");
    }

    std::string CustomInspector::PathForWidget(std::string_view widgetLabel) const
    {
        const auto widget = m_WidgetState.byName.find(std::string(widgetLabel));
        if (widget == m_WidgetState.byName.end() || widget->second.m_VariableName.empty())
            return {};

        const auto section = m_SectionState.widgetSections.find(widget->first);
        if (section == m_SectionState.widgetSections.end() || section->second.empty())
            return widget->second.m_VariableName;
        return section->second + "." + widget->second.m_VariableName;
    }

    CustomInspectorValue &CustomInspector::ValueForWidget(const std::string &widgetLabel)
    {
        const auto valuePath = PathForWidget(widgetLabel);
        const auto value     = m_ValueState.metadata.find(valuePath);
        if (value == m_ValueState.metadata.end())
            throw std::runtime_error("CustomInspector widget has no matching value: " + widgetLabel);
        return value->second;
    }

    bool CustomInspector::HasWidget(const std::string &name)
    {
        return m_WidgetState.byName.find(name) != m_WidgetState.byName.end();
    }

    CustomInspectorWidget &CustomInspector::GetWidget(const std::string &name)
    {
        return m_WidgetState.byName[name];
    }

    void CustomInspector::RemoveWidget(const std::string &name)
    {
        m_WidgetState.byName.erase(name);
        m_WidgetState.order.erase(std::remove(m_WidgetState.order.begin(), m_WidgetState.order.end(), name), m_WidgetState.order.end());
        m_SectionState.widgetSections.erase(name);
    }

    CustomInspectorWidget &CustomInspector::AddWidget(const std::string &name, const CustomInspectorWidget &widget)
    {
        if (name == "Separator" || name == "NewLine") {
            if (!HasWidget(name))
                m_WidgetState.order.push_back(name);
            m_WidgetState.byName[name] = widget;
            return m_WidgetState.byName[name];
        }
        m_WidgetState.byName[name] = widget;
        m_WidgetState.order.push_back(name);
        if (!m_SectionState.currentName.empty())
            m_SectionState.widgetSections[name] = m_SectionState.currentName;
        return m_WidgetState.byName[name];
    }

    CustomInspectorWidget &CustomInspector::AddWidget(const std::string &label,
                                                      CustomInspectorWidgetType type,
                                                      const std::string &variableName)
    {
        CustomInspectorWidget widget(type);
        widget.SetLabel(label);
        if (type == CustomInspectorWidgetType::Button)
            widget.SetActionName(variableName);
        else
            widget.SetVariableName(variableName);
        return AddWidget(label, widget);
    }

    CustomInspectorWidget &CustomInspector::AddWidgetFromString(const std::string &label, const std::string &type, const std::string &variableName)
    {
        auto widgetType = CustomInspectorWidget::CustomInspectorWidgetTypeFromString(type);
        switch (widgetType) {
            case CustomInspectorWidgetType::Separator:
                return AddWidget("Separator", widgetType);
            case CustomInspectorWidgetType::NewLine:
                return AddWidget("NewLine", widgetType);
            case CustomInspectorWidgetType::Unknown:
                TF3D_LOG_WARN("Skipping unknown CustomInspector widget type '{}'", type);
                return AddWidget(label, CustomInspectorWidgetType::Text);
            default:
                return AddWidget(label, widgetType, variableName);
        }
    }

    bool CustomInspector::SetDropdownOptionsAt(std::string_view path,
                                               const std::vector<std::string> &options,
                                               const std::vector<int32_t> &values)
    {
        for (auto &[label, widget] : m_WidgetState.byName) {
            if (PathForWidget(label) != path)
                continue;
            if (widget.m_Type != CustomInspectorWidgetType::Dropdown)
                return false;
            widget.SetDropdownOptions(options, values);
            return true;
        }
        return false;
    }

    std::optional<std::string> CustomInspector::GetSectionCustomDataString(std::string_view sectionName,
                                                                           std::string_view key) const
    {
        const auto section = m_SectionState.customData.find(std::string(sectionName));
        if (section == m_SectionState.customData.end() || !section->second.is_object())
            return std::nullopt;
        const auto value = section->second.find(std::string(key));
        if (value == section->second.end() || !value->is_string())
            return std::nullopt;
        return value->get<std::string>();
    }

    std::optional<int32_t> CustomInspector::GetSectionSelectionValue(std::string_view sectionName) const
    {
        const auto value = m_SectionState.selectionValues.find(std::string(sectionName));
        return value == m_SectionState.selectionValues.end()
                   ? std::nullopt
                   : std::optional<int32_t>(value->second);
    }

    const CustomInspectorValue *CustomInspector::FindConditionValue(std::string_view name,
                                                                    std::string_view sectionName) const
    {
        const std::string path = name.find('.') != std::string_view::npos
                                     ? std::string(name)
                                 : sectionName.empty() ? std::string(name)
                                                       : std::string(sectionName) + "." + std::string(name);
        return FindExactValue(path);
    }

    bool CustomInspector::IsConditionSatisfied(const std::vector<CustomInspectorRenderCondition> &conditions,
                                               std::string_view sectionName) const
    {
        for (const auto &condition : conditions) {
            const auto value = FindConditionValue(condition.name, sectionName);
            if (value == nullptr)
                return false;
            if (!condition.values.empty()) {
                if (std::find(condition.values.begin(), condition.values.end(), value->Store().Get<int32_t>()) == condition.values.end())
                    return false;
            } else if (value->Store().Get<int32_t>() != 1) {
                return false;
            }
        }
        return true;
    }

    bool CustomInspector::IsSectionVisible(std::string_view sectionName) const
    {
        const auto section = m_SectionState.byName.find(std::string(sectionName));
        return section != m_SectionState.byName.end() && IsConditionSatisfied(section->second.renderConditions, sectionName);
    }

    bool CustomInspector::IsWidgetVisible(std::string_view widgetLabel) const
    {
        const auto widget = m_WidgetState.byName.find(std::string(widgetLabel));
        if (widget == m_WidgetState.byName.end())
            return false;
        const auto section = m_SectionState.widgetSections.find(widget->first);
        if (section != m_SectionState.widgetSections.end() && !IsSectionVisible(section->second))
            return false;
        return IsConditionSatisfied(widget->second.m_RenderOnConditions,
                                    section == m_SectionState.widgetSections.end() ? std::string_view{} : std::string_view(section->second));
    }

    void CustomInspector::ConfigureSectionSelector(const nlohmann::json &config)
    {
        if (!config.contains("SectionSelector"))
            return;
        if (!config["SectionSelector"].is_object()) {
            TF3D_LOG_ERROR("Inspector metadata field 'SectionSelector' must be an object");
            return;
        }

        const auto &selector        = config["SectionSelector"];
        const std::string path      = selector.value("Parameter", "");
        const bool deriveFromName   = !selector.contains("CustomDataKey");
        const std::string customKey = selector.value("CustomDataKey", "");
        if (path.empty()) {
            TF3D_LOG_ERROR("Inspector SectionSelector requires Parameter");
            return;
        }
        if (!deriveFromName && customKey.empty()) {
            TF3D_LOG_ERROR("Inspector SectionSelector CustomDataKey cannot be empty");
            return;
        }

        bool selectorFound = false;
        for (const auto &widgetLabel : m_WidgetState.order) {
            const auto widget = m_WidgetState.byName.find(widgetLabel);
            if (widget == m_WidgetState.byName.end() || PathForWidget(widgetLabel) != path)
                continue;
            if (selectorFound) {
                TF3D_LOG_ERROR("Inspector SectionSelector parameter '{}' is ambiguous", path);
                return;
            }
            selectorFound = true;
        }
        if (!selectorFound) {
            TF3D_LOG_ERROR("Inspector SectionSelector could not find parameter '{}'.", path);
            return;
        }

        std::vector<std::string> labels;
        std::vector<int32_t> values;
        for (const auto &sectionName : m_SectionState.order) {
            const auto data = m_SectionState.customData.find(sectionName);
            if (data == m_SectionState.customData.end() || !data->second.is_object())
                continue;

            std::string selectionID = sectionName;
            if (!deriveFromName) {
                const auto selectionValue = data->second.find(customKey);
                if (selectionValue == data->second.end() || !selectionValue->is_string())
                    continue;
                selectionID = selectionValue->get<std::string>();
            }

            const int32_t value = static_cast<int32_t>(values.size());
            const auto section  = m_SectionState.byName.find(sectionName);
            labels.push_back(section != m_SectionState.byName.end() && !section->second.label.empty()
                                 ? section->second.label
                                 : selectionID);
            values.push_back(value);
            m_SectionState.selectionValues[sectionName] = value;
            if (section != m_SectionState.byName.end())
                section->second.renderConditions.push_back({path, {value}});
        }

        if (!SetDropdownOptionsAt(path, labels, values))
            TF3D_LOG_ERROR("Inspector SectionSelector parameter '{}' is not a Dropdown", path);
    }

} // namespace tf3d::inspector
