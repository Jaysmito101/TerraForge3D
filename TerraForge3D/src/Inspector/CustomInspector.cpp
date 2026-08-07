#include "Inspector/CustomInspector.h"
#include "Base/Base.h"
#include "Utils/Utils.h"

namespace tf3d::inspector
{

    CustomInspector::CustomInspector()
    {
        m_ID = GenerateId(16);
        AddWidget("Separator", CustomInspectorWidget(CustomInspectorWidgetType::Separator));
        AddWidget("NewLine", CustomInspectorWidget(CustomInspectorWidgetType::NewLine));
    }

    CustomInspector::~CustomInspector()
    {
    }

    CustomInspectorSection &CustomInspector::AddSection(const std::string &name,
                                                        const std::string &label,
                                                        bool collapsible,
                                                        bool defaultOpen)
    {
        auto [it, inserted] = m_Sections.emplace(name, CustomInspectorSection{});
        if (inserted) {
            it->second.name = name;
            m_SectionsOrder.push_back(name);
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
        m_CurrentSection = name;
    }

    void CustomInspector::EndSection()
    {
        m_CurrentSection.clear();
    }

    bool CustomInspector::HasSection(const std::string &name) const
    {
        return m_Sections.find(name) != m_Sections.end();
    }

    CustomInspectorSection &CustomInspector::GetSection(const std::string &name)
    {
        if (!HasSection(name))
            return AddSection(name);
        return m_Sections.at(name);
    }

    const CustomInspectorValue *CustomInspector::FindValue(const std::string &name) const
    {
        const auto value = m_Values.find(name);
        return value == m_Values.end() ? nullptr : &value->second;
    }

    bool CustomInspector::Contains(const std::string &name) const
    {
        return m_Values.find(name) != m_Values.end();
    }

    void CustomInspector::Remove(const std::string &name)
    {
        m_Values.erase(name);
    }

    CustomInspectorValue &CustomInspector::AddVariable(const std::string &name, const CustomInspectorValue &value)
    {
        m_Values[name] = value;
        if (m_Values[name].m_SerializedName.empty())
            m_Values[name].m_SerializedName = name;
        return (m_Values[name]);
    }

    CustomInspectorValue &CustomInspector::AddPathVariable(const std::string &name,
                                                           const std::array<glm::vec2, CustomInspectorMaxPathPoints> &defaultPoints, int defaultPointCount)
    {
        CustomInspectorValue value(CustomInspectorValueType::Path);
        value.m_DefaultPathPoints = value.m_PathPoints = defaultPoints;
        value.m_DefaultPathPointCount = value.m_PathPointCount = glm::clamp(defaultPointCount, 1, static_cast<int>(CustomInspectorMaxPathPoints));
        value.m_Name                                           = name;
        return AddVariable(name, value);
    }

    CustomInspectorValue &CustomInspector::AddCurveVariable(const std::string &name,
                                                            const std::array<glm::vec2, CustomInspectorMaxCurvePoints> &defaultPoints, int defaultPointCount)
    {
        CustomInspectorValue value(CustomInspectorValueType::Curve);
        value.m_CurvePoints.fill(glm::vec2(-1.0f));
        value.m_DefaultCurvePoints.fill(glm::vec2(-1.0f));
        const int pointCount = glm::clamp(defaultPointCount, 2, static_cast<int>(CustomInspectorMaxCurvePoints));
        for (int index = 0; index < pointCount; ++index) {
            value.m_CurvePoints[index]        = defaultPoints[index];
            value.m_DefaultCurvePoints[index] = defaultPoints[index];
        }
        if (value.m_CurvePoints[0].x < 0.0f) {
            value.m_CurvePoints[0] = value.m_DefaultCurvePoints[0] = glm::vec2(0.0f, 0.0f);
        }
        if (value.m_CurvePoints[1].x < 0.0f || value.m_CurvePoints[1].x <= value.m_CurvePoints[0].x)
            value.m_CurvePoints[1] = value.m_DefaultCurvePoints[1] = glm::vec2(1.0f, 1.0f);
        value.m_DefaultCurvePointCount = value.m_CurvePointCount = pointCount;
        value.m_Name                                             = name;
        return AddVariable(name, value);
    }

    CustomInspectorValue &CustomInspector::AddVairableFromConfig(const nlohmann::json &config)
    {
        std::string name                 = config.contains("Name") ? config["Name"].get<std::string>() : "Unnamed";
        const std::string serializedName = config.value("SerializedName", name);
        std::string valueTypeName        = "Float";
        if (config.contains("Type"))
            valueTypeName = config["Type"];
        auto valueType            = CustomInspectorValue::CustomInspectorValueTypeFromString(valueTypeName);
        bool hasDefaultValue      = config.contains("Default");
        const auto configureValue = [&](CustomInspectorValue &value) -> CustomInspectorValue & {
            value.m_Name           = name;
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

    bool CustomInspector::HasWidget(const std::string &name)
    {
        return m_Widgets.find(name) != m_Widgets.end();
    }

    CustomInspectorWidget &CustomInspector::GetWidget(const std::string &name)
    {
        return m_Widgets[name];
    }

    void CustomInspector::RemoveWidget(const std::string &name)
    {
        m_Widgets.erase(name);
        m_WidgetsOrder.erase(std::remove(m_WidgetsOrder.begin(), m_WidgetsOrder.end(), name), m_WidgetsOrder.end());
        m_WidgetSections.erase(name);
    }

    CustomInspectorWidget &CustomInspector::AddWidget(const std::string &name, const CustomInspectorWidget &widget)
    {
        if (name == "Separator" || name == "NewLine") {
            if (!HasWidget(name))
                m_WidgetsOrder.push_back(name);
            m_Widgets[name] = widget;
            return m_Widgets[name];
        }
        m_Widgets[name] = widget;
        m_WidgetsOrder.push_back(name);
        if (!m_CurrentSection.empty())
            m_WidgetSections[name] = m_CurrentSection;
        return m_Widgets[name];
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

} // namespace tf3d::inspector
