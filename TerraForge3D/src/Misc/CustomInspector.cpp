#include "Misc/CustomInspector.h"
#include "Base/Base.h"
#include "UI/ImGuiComponents.h"
#include "Utils/PathEditor.h"
#include "Utils/Utils.h"

#include <algorithm>
#include <unordered_set>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

namespace tf3d::misc
{

    void RenderInspectorTooltip(const std::string &label, const std::string &description)
    {
        if (description.empty() || ImGui::IsItemActive() || !ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay))
            return;

        ImGui::SetNextWindowSizeConstraints(ImVec2(220.0f, 0.0f), ImVec2(380.0f, 1000.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
        if (ImGui::BeginTooltip()) {
            ImGui::PushTextWrapPos(350.0f);
            if (!label.empty()) {
                ImGui::TextUnformatted(label.c_str());
                ImGui::Separator();
            }
            ImGui::TextUnformatted(description.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        ImGui::PopStyleVar(2);
    }

    std::string CustomInspectorValue::CustomInspectorValueTypeToString(CustomInspectorValueType type)
    {
        switch (type) {
            case CustomInspectorValueType::Int:
                return "Int";
            case CustomInspectorValueType::Float:
                return "Float";
            case CustomInspectorValueType::Bool:
                return "Bool";
            case CustomInspectorValueType::String:
                return "String";
            case CustomInspectorValueType::Vector2:
                return "Vector2";
            case CustomInspectorValueType::Vector3:
                return "Vector3";
            case CustomInspectorValueType::Vector4:
                return "Vector4";
            case CustomInspectorValueType::Texture:
                return "Texture";
            case CustomInspectorValueType::Path:
                return "Path";
            case CustomInspectorValueType::Curve:
                return "Curve";
            default:
                return "Unknown";
        }
    }

    CustomInspectorValueType CustomInspectorValue::CustomInspectorValueTypeFromString(const std::string &type)
    {
        if (type == "Int")
            return CustomInspectorValueType::Int;
        if (type == "Float")
            return CustomInspectorValueType::Float;
        if (type == "Bool")
            return CustomInspectorValueType::Bool;
        if (type == "String")
            return CustomInspectorValueType::String;
        if (type == "Vector2")
            return CustomInspectorValueType::Vector2;
        if (type == "Vector3")
            return CustomInspectorValueType::Vector3;
        if (type == "Vector4")
            return CustomInspectorValueType::Vector4;
        if (type == "Texture")
            return CustomInspectorValueType::Texture;
        if (type == "Path")
            return CustomInspectorValueType::Path;
        if (type == "Curve")
            return CustomInspectorValueType::Curve;
        return CustomInspectorValueType::Unknown;
    }

    SerializerNode CustomInspectorValue::Save() const
    {
        SerializerNode node = CreateSerializerNode();
        node->Set("Name", m_Name);
        node->Set("Type", static_cast<int32_t>(m_Type));
        node->Set("TypeName", CustomInspectorValueTypeToString(m_Type));
        switch (m_Type) {
            case CustomInspectorValueType::Int:
                node->Set("Value", m_IntValue);
                node->Set("DefaultValue", m_DefaultIntValue);
                break;
            case CustomInspectorValueType::Float:
                node->Set("Value", m_FloatValue);
                node->Set("DefaultValue", m_DefaultFloatValue);
                break;
            case CustomInspectorValueType::Bool:
                node->Set("Value", m_BoolValue);
                node->Set("DefaultValue", m_DefaultBoolValue);
                break;
            case CustomInspectorValueType::String:
                node->Set("Value", m_StringValue);
                node->Set("DefaultValue", m_DefaultStringValue);
                break;
            case CustomInspectorValueType::Vector2:
                node->Set("Value", glm::vec2(m_VectorValue[0], m_VectorValue[1]));
                node->Set("DefaultValue", glm::vec2(m_DefaultVectorValue[0], m_DefaultVectorValue[1]));
                break;
            case CustomInspectorValueType::Vector3:
                node->Set("Value", glm::vec3(m_VectorValue[0], m_VectorValue[1], m_VectorValue[2]));
                node->Set("DefaultValue", glm::vec3(m_DefaultVectorValue[0], m_DefaultVectorValue[1], m_DefaultVectorValue[2]));
                break;
            case CustomInspectorValueType::Vector4:
                node->Set("Value", glm::vec4(m_VectorValue[0], m_VectorValue[1], m_VectorValue[2], m_VectorValue[3]));
                node->Set("DefaultValue", glm::vec4(m_DefaultVectorValue[0], m_DefaultVectorValue[1], m_DefaultVectorValue[2], m_DefaultVectorValue[3]));
                break;
            case CustomInspectorValueType::Texture:
                node->Set("Value", m_TextureValue ? m_TextureValue->GetPath() : "null");
                node->Set("DefaultValue", m_DefaultTextureValue ? m_DefaultTextureValue->GetPath() : "null");
                node->Set("TextureBitDepth", m_TextureLoadAs16Bit ? 16 : 8);
                break;
            case CustomInspectorValueType::Path:
                node->Set("PathPointCount", m_PathPointCount);
                node->Set("DefaultPathPointCount", m_DefaultPathPointCount);
                for (size_t index = 0; index < CustomInspectorMaxPathPoints; ++index) {
                    node->Set("PathValueX" + std::to_string(index), m_PathPoints[index].x);
                    node->Set("PathValueY" + std::to_string(index), m_PathPoints[index].y);
                    node->Set("DefaultPathValueX" + std::to_string(index), m_DefaultPathPoints[index].x);
                    node->Set("DefaultPathValueY" + std::to_string(index), m_DefaultPathPoints[index].y);
                }
                break;
            case CustomInspectorValueType::Curve:
                node->Set("CurvePointCount", m_CurvePointCount);
                node->Set("DefaultCurvePointCount", m_DefaultCurvePointCount);
                for (size_t index = 0; index < CustomInspectorMaxCurvePoints; ++index) {
                    node->Set("CurveValueX" + std::to_string(index), m_CurvePoints[index].x);
                    node->Set("CurveValueY" + std::to_string(index), m_CurvePoints[index].y);
                    node->Set("DefaultCurveValueX" + std::to_string(index), m_DefaultCurvePoints[index].x);
                    node->Set("DefaultCurveValueY" + std::to_string(index), m_DefaultCurvePoints[index].y);
                }
                break;
            default:
                break;
        }
        return node;
    }

    void CustomInspectorValue::Load(const SerializerNode &node)
    {
        m_Type = CustomInspectorValueTypeFromString(node->Get<std::string>("TypeName", CustomInspectorValueTypeToString(m_Type)));
        m_Name = node->Get<std::string>("Name");
        switch (m_Type) {
            case CustomInspectorValueType::Int:
                m_DefaultIntValue = node->Get<int>("DefaultValue", m_DefaultIntValue);
                m_IntValue        = node->Get<int>("Value", m_DefaultIntValue);
                break;
            case CustomInspectorValueType::Float:
                m_DefaultFloatValue = node->Get<float>("DefaultValue", m_DefaultFloatValue);
                m_FloatValue        = node->Get<float>("Value", m_DefaultFloatValue);
                break;
            case CustomInspectorValueType::Bool:
                m_DefaultBoolValue = node->Get<bool>("DefaultValue", m_DefaultBoolValue);
                m_BoolValue        = node->Get<bool>("Value", m_DefaultBoolValue);
                break;
            case CustomInspectorValueType::String:
                m_DefaultStringValue = node->Get<std::string>("DefaultValue", m_DefaultStringValue);
                m_StringValue        = node->Get<std::string>("Value", m_DefaultStringValue);
                break;
            case CustomInspectorValueType::Vector2: {
                const glm::vec2 defaultValue = node->Get<glm::vec2>(
                    "DefaultValue", glm::vec2(m_DefaultVectorValue[0], m_DefaultVectorValue[1]));
                const glm::vec2 value   = node->Get<glm::vec2>("Value", defaultValue);
                m_DefaultVectorValue[0] = defaultValue.x;
                m_DefaultVectorValue[1] = defaultValue.y;
                m_VectorValue[0]        = value.x;
                m_VectorValue[1]        = value.y;
            } break;
            case CustomInspectorValueType::Vector3: {
                const glm::vec3 defaultValue = node->Get<glm::vec3>(
                    "DefaultValue", glm::vec3(m_DefaultVectorValue[0], m_DefaultVectorValue[1], m_DefaultVectorValue[2]));
                const glm::vec3 value   = node->Get<glm::vec3>("Value", defaultValue);
                m_DefaultVectorValue[0] = defaultValue.x;
                m_DefaultVectorValue[1] = defaultValue.y;
                m_DefaultVectorValue[2] = defaultValue.z;
                m_VectorValue[0]        = value.x;
                m_VectorValue[1]        = value.y;
                m_VectorValue[2]        = value.z;
            } break;
            case CustomInspectorValueType::Vector4: {
                const glm::vec4 defaultValue = node->Get<glm::vec4>(
                    "DefaultValue", glm::vec4(m_DefaultVectorValue[0], m_DefaultVectorValue[1], m_DefaultVectorValue[2], m_DefaultVectorValue[3]));
                const glm::vec4 value   = node->Get<glm::vec4>("Value", defaultValue);
                m_DefaultVectorValue[0] = defaultValue.x;
                m_DefaultVectorValue[1] = defaultValue.y;
                m_DefaultVectorValue[2] = defaultValue.z;
                m_DefaultVectorValue[3] = defaultValue.w;
                m_VectorValue[0]        = value.x;
                m_VectorValue[1]        = value.y;
                m_VectorValue[2]        = value.z;
                m_VectorValue[3]        = value.w;
            } break;
            case CustomInspectorValueType::Texture: {
                m_TextureLoadAs16Bit   = node->Get<int>("TextureBitDepth", m_TextureLoadAs16Bit ? 16 : 8) >= 16;
                const auto defaultPath = node->Get<std::string>("DefaultValue", m_DefaultTextureValue ? m_DefaultTextureValue->GetPath() : "");
                const auto path        = node->Get<std::string>("Value", m_DefaultTextureValue ? m_DefaultTextureValue->GetPath() : "");
                auto loadTexture       = [this](const std::string &texturePath) -> std::shared_ptr<Texture2D> {
                    if (texturePath.empty() || texturePath == "null")
                        return nullptr;
                    return std::make_shared<Texture2D>(texturePath, false, false, m_TextureLoadAs16Bit);
                };
                m_DefaultTextureValue = loadTexture(defaultPath);
                m_TextureValue        = loadTexture(path);
                break;
            }
            case CustomInspectorValueType::Path:
                m_DefaultPathPointCount = glm::clamp(node->Get<int>("DefaultPathPointCount", m_DefaultPathPointCount), 1, static_cast<int32_t>(CustomInspectorMaxPathPoints));
                m_PathPointCount        = glm::clamp(node->Get<int>("PathPointCount", m_DefaultPathPointCount), 1, static_cast<int32_t>(CustomInspectorMaxPathPoints));
                for (size_t index = 0; index < CustomInspectorMaxPathPoints; ++index) {
                    m_DefaultPathPoints[index].x = node->Get<float>("DefaultPathValueX" + std::to_string(index), m_DefaultPathPoints[index].x);
                    m_DefaultPathPoints[index].y = node->Get<float>("DefaultPathValueY" + std::to_string(index), m_DefaultPathPoints[index].y);
                    m_PathPoints[index].x        = node->Get<float>("PathValueX" + std::to_string(index), m_DefaultPathPoints[index].x);
                    m_PathPoints[index].y        = node->Get<float>("PathValueY" + std::to_string(index), m_DefaultPathPoints[index].y);
                }
                break;
            case CustomInspectorValueType::Curve:
                m_DefaultCurvePointCount = glm::clamp(node->Get<int>("DefaultCurvePointCount", m_DefaultCurvePointCount), 2, static_cast<int32_t>(CustomInspectorMaxCurvePoints));
                m_CurvePointCount        = glm::clamp(node->Get<int>("CurvePointCount", m_DefaultCurvePointCount), 2, static_cast<int32_t>(CustomInspectorMaxCurvePoints));
                for (size_t index = 0; index < CustomInspectorMaxCurvePoints; ++index) {
                    m_DefaultCurvePoints[index].x = node->Get<float>("DefaultCurveValueX" + std::to_string(index), m_DefaultCurvePoints[index].x);
                    m_DefaultCurvePoints[index].y = node->Get<float>("DefaultCurveValueY" + std::to_string(index), m_DefaultCurvePoints[index].y);
                    m_CurvePoints[index].x        = node->Get<float>("CurveValueX" + std::to_string(index), m_DefaultCurvePoints[index].x);
                    m_CurvePoints[index].y        = node->Get<float>("CurveValueY" + std::to_string(index), m_DefaultCurvePoints[index].y);
                }
                break;
            default:
                break;
        }
    }

    bool CustomInspectorValue::WriteStateValue(SerializerNode target, const std::string &name) const
    {
        switch (m_Type) {
            case CustomInspectorValueType::Int:
                target->Set(name, m_IntValue);
                return true;
            case CustomInspectorValueType::Float:
                target->Set(name, m_FloatValue);
                return true;
            case CustomInspectorValueType::Bool:
                target->Set(name, m_BoolValue);
                return true;
            case CustomInspectorValueType::String:
                target->Set(name, m_StringValue);
                return true;
            case CustomInspectorValueType::Vector2:
                target->Set(name, glm::vec2(m_VectorValue[0], m_VectorValue[1]));
                return true;
            case CustomInspectorValueType::Vector3:
                target->Set(name, glm::vec3(m_VectorValue[0], m_VectorValue[1], m_VectorValue[2]));
                return true;
            case CustomInspectorValueType::Vector4:
                target->Set(name, glm::vec4(m_VectorValue[0], m_VectorValue[1], m_VectorValue[2], m_VectorValue[3]));
                return true;
            case CustomInspectorValueType::Texture:
                target->Set(name, m_TextureValue ? m_TextureValue->GetPath() : "");
                return true;
            case CustomInspectorValueType::Path:
                target->Set(name, Get<std::vector<glm::vec2>>());
                return true;
            case CustomInspectorValueType::Curve:
                target->Set(name, Get<std::vector<glm::vec2>>());
                return true;
            case CustomInspectorValueType::Unknown:
            default:
                return false;
        }
    }

    bool CustomInspectorValue::ReadStateValue(SerializerNode source, const std::string &name)
    {
        switch (m_Type) {
            case CustomInspectorValueType::Int:
                return Set(source->Get(name, m_IntValue));
            case CustomInspectorValueType::Float:
                return Set(source->Get(name, m_FloatValue));
            case CustomInspectorValueType::Bool:
                return Set(source->Get(name, m_BoolValue));
            case CustomInspectorValueType::String:
                return Set(source->Get(name, m_StringValue));
            case CustomInspectorValueType::Vector2:
                return Set(source->Get(name, GetVector2()));
            case CustomInspectorValueType::Vector3:
                return Set(source->Get(name, GetVector3()));
            case CustomInspectorValueType::Vector4:
                return Set(source->Get(name, GetVector4()));
            case CustomInspectorValueType::Texture: {
                const std::string path = source->Get(name, m_TextureValue ? m_TextureValue->GetPath() : "");
                m_TextureValue         = path.empty() ? nullptr : std::make_shared<Texture2D>(path, false, false, m_TextureLoadAs16Bit);
                return true;
            }
            case CustomInspectorValueType::Path:
            case CustomInspectorValueType::Curve:
                return Set(source->Get(name, Get<std::vector<glm::vec2>>()));
            case CustomInspectorValueType::Unknown:
            default:
                return false;
        }
    }

    CustomInspectorWidget::CustomInspectorWidget(CustomInspectorWidgetType type)
    {
        m_Type = type;
        m_ID   = GenerateId(16);
    }

    CustomInspectorWidget::~CustomInspectorWidget()
    {
    }

    std::string CustomInspectorWidget::CustomInspectorWidgetTypeToString(CustomInspectorWidgetType type)
    {
        switch (type) {
            case CustomInspectorWidgetType_Slider:
                return "Slider";
            case CustomInspectorWidgetType_Drag:
                return "Drag";
            case CustomInspectorWidgetType_Color:
                return "Color";
            case CustomInspectorWidgetType_Texture:
                return "Texture";
            case CustomInspectorWidgetType_Path:
                return "Path";
            case CustomInspectorWidgetType_Curve:
                return "Curve";
            case CustomInspectorWidgetType_Button:
                return "Button";
            case CustomInspectorWidgetType_Checkbox:
                return "Checkbox";
            case CustomInspectorWidgetType_Input:
                return "Input";
            case CustomInspectorWidgetType_Seed:
                return "Seed";
            case CustomInspectorWidgetType_Dropdown:
                return "Dropdown";
            case CustomInspectorWidgetType_Seperator:
                return "Seperator";
            case CustomInspectorWidgetType_NewLine:
                return "NewLine";
            case CustomInspectorWidgetType_Text:
                return "Text";
            case CustomInspectorWidgetType_Unknown:
            default:
                return "Unknown";
        }
    }

    CustomInspectorWidgetType CustomInspectorWidget::CustomInspectorWidgetTypeFromString(const std::string &type)
    {
        if (type == "Slider")
            return CustomInspectorWidgetType_Slider;
        if (type == "Drag")
            return CustomInspectorWidgetType_Drag;
        if (type == "Color")
            return CustomInspectorWidgetType_Color;
        if (type == "Texture")
            return CustomInspectorWidgetType_Texture;
        if (type == "Path")
            return CustomInspectorWidgetType_Path;
        if (type == "Curve")
            return CustomInspectorWidgetType_Curve;
        if (type == "Button")
            return CustomInspectorWidgetType_Button;
        if (type == "Checkbox")
            return CustomInspectorWidgetType_Checkbox;
        if (type == "Input")
            return CustomInspectorWidgetType_Input;
        if (type == "Seed")
            return CustomInspectorWidgetType_Seed;
        if (type == "Dropdown")
            return CustomInspectorWidgetType_Dropdown;
        if (type == "Seperator")
            return CustomInspectorWidgetType_Seperator;
        if (type == "NewLine")
            return CustomInspectorWidgetType_NewLine;
        if (type == "Text")
            return CustomInspectorWidgetType_Text;
        return CustomInspectorWidgetType_Unknown;
    }

    SerializerNode CustomInspectorWidget::Save() const
    {
        SerializerNode node = CreateSerializerNode();
        node->Set("Type", static_cast<int32_t>(m_Type));
        node->Set("TypeName", CustomInspectorWidgetTypeToString(m_Type));
        node->Set("TargetVariable", m_VariableName);
        node->Set("Label", m_Label);
        if (m_Type == CustomInspectorWidgetType_Seed)
            node->Set("SeedHistory", m_SeedHistory);
        node->Set("ISpeed", m_ISpeed);
        node->Set("FSeed", m_FSpeed);
        node->Set("ID", m_ID);
        node->Set("Constraints0", m_Constratins[0]);
        node->Set("Constraints1", m_Constratins[1]);
        node->Set("Constraints2", m_Constratins[2]);
        node->Set("Constraints3", m_Constratins[3]);
        if (m_UseRenderOnCondition) {
            node->Set("RenderOnConditionValue", m_RenderOnConditionValue);
            node->Set("RenderOnConditionName", m_RenderOnConditionName);
            node->Set("UseRenderOnCondition", m_UseRenderOnCondition);
            if (!m_RenderOnConditionValues.empty())
                node->Set("RenderOnConditionValues", m_RenderOnConditionValues);
        }
        if (m_FontName.size() > 0)
            node->Set("FontName", m_FontName);
        if (m_Tooltip.size() > 0)
            node->Set("Tooltip", m_Tooltip);
        return node;
    }

    void CustomInspectorWidget::Load(SerializerNode node)
    {
        m_Type         = CustomInspectorWidgetTypeFromString(node->Get<std::string>("TypeName", CustomInspectorWidgetTypeToString(m_Type)));
        m_VariableName = node->Get<std::string>("TargetVariable", m_VariableName);
        m_Label        = node->Get<std::string>("Label", m_Label);
        m_ID           = node->Get<std::string>("ID", m_ID);
        if (m_Type == CustomInspectorWidgetType_Seed)
            m_SeedHistory = node->Get<std::vector<int>>("SeedHistory", m_SeedHistory);
        m_ISpeed               = node->Get<int>("ISpeed", m_ISpeed);
        m_FSpeed               = node->Get<float>("FSeed", m_FSpeed);
        m_Constratins[0]       = node->Get<float>("Constraints0", m_Constratins[0]);
        m_Constratins[1]       = node->Get<float>("Constraints1", m_Constratins[1]);
        m_Constratins[2]       = node->Get<float>("Constraints2", m_Constratins[2]);
        m_Constratins[3]       = node->Get<float>("Constraints3", m_Constratins[3]);
        m_UseRenderOnCondition = node->Get<bool>("UseRenderOnCondition", false);
        if (m_UseRenderOnCondition) {
            m_RenderOnConditionValue  = node->Get<int>("RenderOnConditionValue", m_RenderOnConditionValue);
            m_RenderOnConditionName   = node->Get<std::string>("RenderOnConditionName", m_RenderOnConditionName);
            m_RenderOnConditionValues = node->Get<std::vector<int>>("RenderOnConditionValues", {});
            if (m_RenderOnConditionValues.empty())
                m_RenderOnConditionValues.push_back(m_RenderOnConditionValue);
        }
        m_Tooltip  = node->Get<std::string>("Tooltip", m_Tooltip);
        m_FontName = node->Get<std::string>("FontName", m_FontName);
    }

    CustomInspector::CustomInspector()
    {
        m_ID = GenerateId(16);
        AddWidget("Seperator", CustomInspectorWidget(CustomInspectorWidgetType_Seperator));
        AddWidget("NewLine", CustomInspectorWidget(CustomInspectorWidgetType_NewLine));
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
        // Ideally this should not be called
        m_Values.erase(name);
        // Suggestion:
        // Also delete all widgets referencing this variable
    }

    CustomInspectorValue &CustomInspector::AddVariable(const std::string &name, const CustomInspectorValue &value)
    {
        m_Values[name] = value;
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
        std::string name          = config.contains("Name") ? config["Name"].get<std::string>() : "Unnamed";
        std::string valueTypeName = "Float";
        if (config.contains("Type"))
            valueTypeName = config["Type"];
        auto valueType       = CustomInspectorValue::CustomInspectorValueTypeFromString(valueTypeName);
        bool hasDefaultValue = config.contains("Default");
        switch (valueType) {
            case CustomInspectorValueType::Int: {
                auto &var  = Add<int32_t>(name, hasDefaultValue ? config["Default"].get<int32_t>() : 0);
                var.m_Name = name;
                return var;
            }
            case CustomInspectorValueType::Float: {
                auto &var  = Add<float>(name, hasDefaultValue ? config["Default"].get<float>() : 0.0f);
                var.m_Name = name;
                return var;
            }
            case CustomInspectorValueType::Bool: {
                auto &var  = Add<bool>(name, hasDefaultValue ? config["Default"].get<bool>() : false);
                var.m_Name = name;
                return var;
            }
            case CustomInspectorValueType::String: {
                auto &var  = Add<std::string>(name, hasDefaultValue ? config["Default"].get<std::string>() : "");
                var.m_Name = name;
                return var;
            }
            case CustomInspectorValueType::Vector2: {
                auto &var  = Add<glm::vec2>(name, hasDefaultValue ? glm::vec2(config["Default"][0].get<float>(), config["Default"][1].get<float>()) : glm::vec2(0.0f));
                var.m_Name = name;
                return var;
            }
            case CustomInspectorValueType::Vector3: {
                auto &var  = Add<glm::vec3>(name, hasDefaultValue ? glm::vec3(config["Default"][0].get<float>(), config["Default"][1].get<float>(), config["Default"][2].get<float>()) : glm::vec3(0.0f));
                var.m_Name = name;
                return var;
            }
            case CustomInspectorValueType::Vector4: {
                auto &var  = Add<glm::vec4>(name, hasDefaultValue ? glm::vec4(config["Default"][0].get<float>(), config["Default"][1].get<float>(), config["Default"][2].get<float>(), config["Default"][3].get<float>()) : glm::vec4(0.0f));
                var.m_Name = name;
                return var;
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
                var.m_Name               = name;
                return var;
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
                auto &var  = AddPathVariable(name, points, pointCount);
                var.m_Name = name;
                return var;
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
                auto &var  = AddCurveVariable(name, points, pointCount);
                var.m_Name = name;
                return var;
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
    }

    CustomInspectorWidget &CustomInspector::AddWidget(const std::string &name, const CustomInspectorWidget &widget)
    {
        if (name == "Seperator" || name == "NewLine") {
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
        if (type == CustomInspectorWidgetType_Button)
            widget.SetActionName(variableName);
        else
            widget.SetVariableName(variableName);
        return AddWidget(label, widget);
    }

    CustomInspectorWidget &CustomInspector::AddWidgetFromString(const std::string &label, const std::string &type, const std::string &variableName)
    {
        auto widgetType = CustomInspectorWidget::CustomInspectorWidgetTypeFromString(type);
        switch (widgetType) {
            case CustomInspectorWidgetType_Seperator:
                return AddWidget("Seperator", widgetType);
            case CustomInspectorWidgetType_NewLine:
                return AddWidget("NewLine", widgetType);
            case CustomInspectorWidgetType_Unknown:
                TF3D_LOG_WARN("Skipping unknown CustomInspector widget type '{}'", type);
                return AddWidget(label, CustomInspectorWidgetType_Text);
            default:
                return AddWidget(label, widgetType, variableName);
        }
    }

    SerializerNode CustomInspector::SaveData() const
    {
        SerializerNode node = CreateSerializerNode();
        node->Set("ValueCount", static_cast<int32_t>(m_Values.size()));
        std::vector<SerializerNode> values;
        values.reserve(m_Values.size());
        for (const auto &it : m_Values) {
            auto subNode = it.second.Save();
            subNode->Set("GName", it.first);
            values.push_back(subNode);
        }
        node->Set("Values", values);
        return node;
    }

    void CustomInspector::LoadData(SerializerNode node)
    {
        std::unordered_map<std::string, bool> textureBitDepths;
        for (const auto &[name, existingValue] : m_Values) {
            if (existingValue.GetType() == CustomInspectorValueType::Texture && existingValue.m_TextureLoadAs16Bit)
                textureBitDepths[name] = true;
        }
        m_Values.clear();
        int32_t valueCount = node->Get<int>("ValueCount");
        auto subNodes      = node->Get<std::vector<SerializerNode>>("Values");
        if (subNodes.size() != valueCount)
            TF3D_LOG_WARN("Inspector data is incomplete: expected {}, found {}", valueCount, subNodes.size());
        for (auto subNode : subNodes) {
            std::string name = subNode->Get<std::string>("GName");
            CustomInspectorValue value;
            if (textureBitDepths.contains(name))
                value.m_TextureLoadAs16Bit = true;
            value.Load(subNode);
            m_Values[name] = value;
        }
    }

    SerializerNode CustomInspector::SaveState() const
    {
        SerializerNode state = CreateSerializerNode();
        std::unordered_set<std::string> savedVariables;

        auto saveValue = [&](SerializerNode target,
                             const std::string &name,
                             const CustomInspectorValue &value) {
            if (!value.WriteStateValue(target, name))
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
                const auto value = m_Values.find(widget->second.m_VariableName);
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

        bool valid     = true;
        auto loadValue = [&](const std::string &name, SerializerNode source) {
            const auto existing = m_Values.find(name);
            if (existing == m_Values.end()) {
                TF3D_LOG_WARN("Invalid CustomInspector state field '{}'", name);
                valid = false;
                return;
            }

            if (!existing->second.ReadStateValue(source, name)) {
                TF3D_LOG_WARN("Invalid CustomInspector state type for '{}'", name);
                valid = false;
            }
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
                    loadValue(field, sectionState);
                continue;
            }
            loadValue(key, node);
        }
        return valid;
    }

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
                if ((widget->m_Type == CustomInspectorWidgetType_Slider ||
                     widget->m_Type == CustomInspectorWidgetType_Drag) &&
                    (widget->m_Constratins[0] != 0.0f || widget->m_Constratins[1] != 0.0f)) {
                    schema["minimum"] = widget->m_Constratins[0];
                    schema["maximum"] = widget->m_Constratins[1];
                }
                if (widget->m_Type == CustomInspectorWidgetType_Dropdown &&
                    !widget->m_DropdownOptions.empty()) {
                    schema["type"]        = "integer";
                    schema["enum"]        = nlohmann::json::array();
                    schema["x-enumNames"] = nlohmann::json::array();
                    for (size_t index = 0; index < widget->m_DropdownOptions.size(); ++index) {
                        schema["enum"].push_back(index);
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
            target["properties"][name] = buildValueSchema(value->second, widget);
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

            for (const auto &widgetLabel : m_WidgetsOrder) {
                const auto widgetSection = m_WidgetSections.find(widgetLabel);
                if (widgetSection == m_WidgetSections.end() || widgetSection->second != sectionName)
                    continue;
                const auto widget = m_Widgets.find(widgetLabel);
                if (widget != m_Widgets.end() && !widget->second.m_VariableName.empty())
                    addVariable(sectionSchema, widget->second.m_VariableName, &widget->second);
            }
            schema["properties"][sectionName] = sectionSchema;
        }

        for (const auto &widgetLabel : m_WidgetsOrder) {
            const auto widget = m_Widgets.find(widgetLabel);
            if (widget == m_Widgets.end() || widget->second.m_VariableName.empty())
                continue;
            if (!m_WidgetSections.contains(widgetLabel))
                addVariable(schema, widget->second.m_VariableName, &widget->second);
        }
        for (const auto &[name, value] : m_Values)
            addVariable(schema, name, nullptr);
        return schema;
    }

    SerializerNode CustomInspector::Save() const
    {
        SerializerNode node = CreateSerializerNode();
        node->Set("ID", m_ID);
        if (!m_Description.empty())
            node->Set("Description", m_Description);
        node->Set("Data", SaveData());
        node->Set("WidgetsOrder", m_WidgetsOrder);
        node->Set("WidgetsCount", static_cast<int32_t>(m_Widgets.size()));
        std::vector<SerializerNode> widgets;
        widgets.reserve(m_Widgets.size());
        for (const auto &it : m_Widgets) {
            auto subNode = it.second.Save();
            subNode->Set("GName", it.first);
            widgets.push_back(subNode);
        }
        node->Set("Widgets", widgets);

        node->Set("SectionsOrder", m_SectionsOrder);
        std::vector<SerializerNode> sections;
        sections.reserve(m_Sections.size());
        for (const auto &[name, section] : m_Sections) {
            auto sectionNode = CreateSerializerNode();
            sectionNode->Set("GName", name);
            sectionNode->Set("Label", section.label);
            sectionNode->Set("Description", section.description);
            sectionNode->Set("Collapsible", section.collapsible);
            sectionNode->Set("DefaultOpen", section.defaultOpen);
            sections.push_back(sectionNode);
        }
        node->Set("Sections", sections);

        std::vector<SerializerNode> widgetSections;
        widgetSections.reserve(m_WidgetSections.size());
        for (const auto &[widget, section] : m_WidgetSections) {
            auto sectionNode = CreateSerializerNode();
            sectionNode->Set("Widget", widget);
            sectionNode->Set("Section", section);
            widgetSections.push_back(sectionNode);
        }
        node->Set("WidgetSections", widgetSections);
        return node;
    }

    void CustomInspector::Load(SerializerNode node)
    {
        LoadData(node->Get<SerializerNode>("Data"));
        m_Widgets.clear();
        m_WidgetsOrder.clear();
        m_ID           = node->Get<std::string>("ID", m_ID);
        m_Description  = node->Get<std::string>("Description", m_Description);
        int valueCount = node->Get<int>("WidgetsCount");
        m_WidgetsOrder = node->Get<std::vector<std::string>>("WidgetsOrder");
        auto subNodes  = node->Get<std::vector<SerializerNode>>("Widgets");
        if (subNodes.size() != valueCount)
            TF3D_LOG_WARN("Inspector data is incomplete: expected {}, found {}", valueCount, subNodes.size());
        for (auto subNode : subNodes) {
            std::string name = subNode->Get<std::string>("GName");
            CustomInspectorWidget widget;
            widget.Load(subNode);
            m_Widgets[name] = widget;
        }
        if (m_WidgetsOrder.empty()) {
            for (const auto &subNode : subNodes)
                m_WidgetsOrder.push_back(subNode->Get<std::string>("GName"));
        }

        m_Sections.clear();
        m_SectionsOrder     = node->Get<std::vector<std::string>>("SectionsOrder", {});
        const auto sections = node->Get<std::vector<SerializerNode>>("Sections", {});
        for (const auto &sectionNode : sections) {
            if (!sectionNode)
                continue;
            const std::string name = sectionNode->Get<std::string>("GName");
            if (name.empty())
                continue;
            auto &section       = m_Sections[name];
            section.name        = name;
            section.label       = sectionNode->Get<std::string>("Label", name);
            section.description = sectionNode->Get<std::string>("Description", "");
            section.collapsible = sectionNode->Get<bool>("Collapsible", false);
            section.defaultOpen = sectionNode->Get<bool>("DefaultOpen", true);
            if (std::find(m_SectionsOrder.begin(), m_SectionsOrder.end(), name) == m_SectionsOrder.end())
                m_SectionsOrder.push_back(name);
        }
        m_WidgetSections.clear();
        const auto widgetSections = node->Get<std::vector<SerializerNode>>("WidgetSections", {});
        for (const auto &sectionNode : widgetSections) {
            if (!sectionNode)
                continue;
            const std::string widget  = sectionNode->Get<std::string>("Widget");
            const std::string section = sectionNode->Get<std::string>("Section");
            if (!widget.empty() && !section.empty())
                m_WidgetSections[widget] = section;
        }
    }

    bool CustomInspector::LoadConfig(const nlohmann::json &config)
    {
        Clear();
        m_Description     = config.contains("Description") && config["Description"].is_string()
                                ? config["Description"].get<std::string>()
                                : "";
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
                        const auto &value             = AddVairableFromConfig(parameter);
                        const std::string widgetType  = parameter.value("Widget", "Input");
                        const std::string widgetLabel = parameter.value("Label", value.GetName());
                        auto &widget                  = AddWidgetFromString(widgetLabel, widgetType, value.GetName());
                        if (parameter.contains("Sensitivity"))
                            widget.SetSpeed(parameter["Sensitivity"].get<float>());
                        if (parameter.contains("Options"))
                            widget.SetDropdownOptions(parameter["Options"].get<std::vector<std::string>>());
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
                        if (parameter.contains("Tooltip"))
                            widget.SetTooltip(parameter["Tooltip"].get<std::string>());
                        else if (parameter.contains("Description"))
                            widget.SetTooltip(parameter["Description"].get<std::string>());
                        if (parameter.contains("Conditional")) {
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
                        auto &widget             = AddWidget(label, CustomInspectorWidgetType_Button, action);
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
        } catch (const std::exception &exception) {
            TF3D_LOG_ERROR("Failed to load inspector metadata: {}", exception.what());
            return false;
        }
        if (!hasContent)
            AddWidget("No parameters available", CustomInspectorWidgetType_Text);
        return true;
    }

    bool CustomInspector::RenderWidget(const std::string &widgetLabel)
    {
        const auto widgetIterator = m_Widgets.find(widgetLabel);
        if (widgetIterator == m_Widgets.end())
            return false;

        const auto &widget = widgetIterator->second;
        if (widget.m_UseRenderOnCondition) {
            if (!Contains(widget.m_RenderOnConditionName))
                return false;
            const auto &condition     = m_Values.at(widget.m_RenderOnConditionName);
            const auto &allowedValues = widget.m_RenderOnConditionValues;
            if (!allowedValues.empty()) {
                if (std::find(allowedValues.begin(), allowedValues.end(), condition.Get<int32_t>()) == allowedValues.end())
                    return false;
            } else if (condition.Get<int32_t>() != widget.m_RenderOnConditionValue) {
                return false;
            }
        }

        ImGui::PushID(widget.m_ID.c_str());
        if (!widget.m_FontName.empty())
            ImGui::PushFont(GetUIFont(widget.m_FontName));
        bool widgetChanged = false;
        if (widget.m_Type == CustomInspectorWidgetType_Slider)
            widgetChanged = RenderSlider(widget);
        else if (widget.m_Type == CustomInspectorWidgetType_Drag)
            widgetChanged = RenderDrag(widget);
        else if (widget.m_Type == CustomInspectorWidgetType_Color)
            widgetChanged = RenderColor(widget);
        else if (widget.m_Type == CustomInspectorWidgetType_Texture)
            widgetChanged = RenderTexture(widget);
        else if (widget.m_Type == CustomInspectorWidgetType_Path)
            widgetChanged = RenderPath(widget);
        else if (widget.m_Type == CustomInspectorWidgetType_Curve)
            widgetChanged = RenderCurve(widget);
        else if (widget.m_Type == CustomInspectorWidgetType_Button)
            widgetChanged = RenderButton(widget);
        else if (widget.m_Type == CustomInspectorWidgetType_Checkbox)
            widgetChanged = RenderCheckbox(widget);
        else if (widget.m_Type == CustomInspectorWidgetType_Input)
            widgetChanged = RenderInput(widget);
        else if (widget.m_Type == CustomInspectorWidgetType_Seed)
            widgetChanged = RenderSeed(m_Widgets[widgetLabel]);
        else if (widget.m_Type == CustomInspectorWidgetType_Dropdown)
            widgetChanged = RenderDropdown(widget);
        else if (widget.m_Type == CustomInspectorWidgetType_Seperator)
            ImGui::Separator();
        else if (widget.m_Type == CustomInspectorWidgetType_NewLine)
            ImGui::NewLine();
        else if (widget.m_Type == CustomInspectorWidgetType_Text)
            ImGui::TextWrapped("%s", widget.m_Label.c_str());

        if (widgetChanged) {
            if (widget.m_Type == CustomInspectorWidgetType_Button)
                m_LastAction = widget.m_VariableName;
            else if (!widget.m_VariableName.empty())
                m_LastChangedVariable = widget.m_VariableName;
        }
        if (!widget.m_FontName.empty())
            ImGui::PopFont();
        RenderInspectorTooltip(widget.m_Label, widget.m_Tooltip);
        if (widget.m_Type != CustomInspectorWidgetType_Seed &&
            widget.m_Type != CustomInspectorWidgetType_Button &&
            !widget.m_VariableName.empty()) {
            if (ImGui::BeginPopupContextItem(widget.m_ID.c_str())) {
                static char s_ResetButtonName[1024];
                sprintf(s_ResetButtonName, "Reset Value (%s)", widget.GetLabel().c_str());
                if (ImGui::Button(s_ResetButtonName)) {
                    m_Values[widget.m_VariableName].ResetValue();
                    widgetChanged         = true;
                    m_LastChangedVariable = widget.m_VariableName;
                }
                ImGui::EndPopup();
            }
        }
        ImGui::PopID();
        return widgetChanged;
    }

    bool CustomInspector::Render()
    {
        bool hasChanged = false;
        m_LastChangedVariable.clear();
        m_LastAction.clear();
        ImGui::PushID(m_ID.c_str());
        if (!m_Description.empty()) {
            ImGui::TextWrapped("%s", m_Description.c_str());
            ImGui::Separator();
        }

        const auto renderWidget = [&](const std::string &widgetLabel) {
            hasChanged = RenderWidget(widgetLabel) || hasChanged;
        };

        if (m_SectionsOrder.empty()) {
            for (const auto &widgetLabel : m_WidgetsOrder)
                renderWidget(widgetLabel);
        } else {
            for (const auto &widgetLabel : m_WidgetsOrder) {
                if (!m_WidgetSections.contains(widgetLabel))
                    renderWidget(widgetLabel);
            }

            for (const auto &sectionName : m_SectionsOrder) {
                const auto section = m_Sections.find(sectionName);
                if (section == m_Sections.end())
                    continue;

                ImGui::PushID(sectionName.c_str());
                bool renderSection = true;
                if (section->second.collapsible) {
                    ImGui::SetNextItemOpen(section->second.defaultOpen, ImGuiCond_Once);
                    renderSection = ImGui::CollapsingHeader(section->second.label.c_str());
                } else {
                    ImGui::TextUnformatted(section->second.label.c_str());
                    if (!section->second.description.empty())
                        RenderInspectorTooltip(section->second.label, section->second.description);
                    ImGui::Separator();
                }
                if (renderSection) {
                    for (const auto &widgetLabel : m_WidgetsOrder) {
                        const auto widgetSection = m_WidgetSections.find(widgetLabel);
                        if (widgetSection != m_WidgetSections.end() && widgetSection->second == sectionName)
                            renderWidget(widgetLabel);
                    }
                }
                ImGui::PopID();
            }
        }

        if (m_ShowResetButton && ImGui::Button("Reset to Defaults")) {
            for (auto &it : m_Values)
                it.second.ResetValue();
            hasChanged = true;
        }
        ImGui::PopID();
        return hasChanged;
    }

    bool CustomInspector::RenderSlider(const CustomInspectorWidget &widget)
    {
        bool hasChanged = false;
        auto &value     = m_Values[widget.m_VariableName];
        switch (value.GetType()) {
            case CustomInspectorValueType::Int:
                hasChanged = ImGui::SliderInt(widget.m_Label.c_str(), &value.m_IntValue, static_cast<int32_t>(widget.m_Constratins[0]), static_cast<int32_t>(widget.m_Constratins[1]));
                break;
            case CustomInspectorValueType::Float:
                hasChanged = ImGui::SliderFloat(widget.m_Label.c_str(), &value.m_FloatValue, widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType::Vector2:
                hasChanged = ImGui::SliderFloat2(widget.m_Label.c_str(), value.m_VectorValue, widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType::Vector3:
                hasChanged = ImGui::SliderFloat3(widget.m_Label.c_str(), value.m_VectorValue, widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType::Vector4:
                hasChanged = ImGui::SliderFloat4(widget.m_Label.c_str(), value.m_VectorValue, widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType::String:
            case CustomInspectorValueType::Bool:
            case CustomInspectorValueType::Texture:
                throw std::runtime_error(std::string("Invalid data type for Slider"));
            default:
                throw std::runtime_error(std::string("Invalid data type for Slider"));
        }
        return hasChanged;
    }

    bool CustomInspector::RenderDrag(const CustomInspectorWidget &widget)
    {
        bool hasChanged = false;
        auto &value     = m_Values[widget.m_VariableName];
        switch (value.GetType()) {
            case CustomInspectorValueType::Int:
                hasChanged = ImGui::DragInt(widget.m_Label.c_str(), &value.m_IntValue, widget.m_FSpeed, static_cast<int32_t>(widget.m_Constratins[0]), static_cast<int32_t>(widget.m_Constratins[1]));
                if (abs(widget.m_Constratins[0] - widget.m_Constratins[1]) < 0.001f)
                    break; // no constraints
                value.m_IntValue = std::clamp(value.m_IntValue, static_cast<int32_t>(widget.m_Constratins[0]), static_cast<int32_t>(widget.m_Constratins[1]));
                break;
            case CustomInspectorValueType::Float:
                hasChanged = ImGui::DragFloat(widget.m_Label.c_str(), &value.m_FloatValue, widget.m_FSpeed, widget.m_Constratins[0], widget.m_Constratins[1]);
                if (abs(widget.m_Constratins[0] - widget.m_Constratins[1]) < 0.001f)
                    break; // no constraints
                value.m_FloatValue = std::clamp(value.m_FloatValue, widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType::Vector2:
                hasChanged = ImGui::DragFloat2(widget.m_Label.c_str(), value.m_VectorValue, widget.m_FSpeed, widget.m_Constratins[0], widget.m_Constratins[1]);
                if (abs(widget.m_Constratins[0] - widget.m_Constratins[1]) < 0.001f)
                    break; // no constraints
                value.m_VectorValue[0] = std::clamp(value.m_VectorValue[0], widget.m_Constratins[0], widget.m_Constratins[1]);
                value.m_VectorValue[1] = std::clamp(value.m_VectorValue[1], widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType::Vector3:
                hasChanged = ImGui::DragFloat3(widget.m_Label.c_str(), value.m_VectorValue, widget.m_FSpeed, widget.m_Constratins[0], widget.m_Constratins[1]);
                if (abs(widget.m_Constratins[0] - widget.m_Constratins[1]) < 0.001f)
                    break; // no constraints
                value.m_VectorValue[0] = std::clamp(value.m_VectorValue[0], widget.m_Constratins[0], widget.m_Constratins[1]);
                value.m_VectorValue[1] = std::clamp(value.m_VectorValue[1], widget.m_Constratins[0], widget.m_Constratins[1]);
                value.m_VectorValue[2] = std::clamp(value.m_VectorValue[2], widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType::Vector4:
                hasChanged = ImGui::DragFloat4(widget.m_Label.c_str(), value.m_VectorValue, widget.m_FSpeed, widget.m_Constratins[0], widget.m_Constratins[1]);
                if (abs(widget.m_Constratins[0] - widget.m_Constratins[1]) < 0.001f)
                    break; // no constraints
                value.m_VectorValue[0] = std::clamp(value.m_VectorValue[0], widget.m_Constratins[0], widget.m_Constratins[1]);
                value.m_VectorValue[1] = std::clamp(value.m_VectorValue[1], widget.m_Constratins[0], widget.m_Constratins[1]);
                value.m_VectorValue[2] = std::clamp(value.m_VectorValue[2], widget.m_Constratins[0], widget.m_Constratins[1]);
                value.m_VectorValue[3] = std::clamp(value.m_VectorValue[3], widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType::String:
            case CustomInspectorValueType::Bool:
            case CustomInspectorValueType::Texture:
                throw std::runtime_error(std::string("Invalid data type for Drag"));
            default:
                throw std::runtime_error(std::string("Invalid data type for Drag"));
        }
        return hasChanged;
    }

    bool CustomInspector::RenderColor(const CustomInspectorWidget &widget)
    {
        bool hasChanged = false;
        auto &value     = m_Values[widget.m_VariableName];
        switch (value.GetType()) {
            case CustomInspectorValueType::Int:
                hasChanged       = ImGui::ColorEdit4(widget.m_Label.c_str(), value.m_VectorValue);
                value.m_IntValue = ImGui::ColorConvertFloat4ToU32(ImVec4(value.m_VectorValue[0], value.m_VectorValue[1], value.m_VectorValue[2], value.m_VectorValue[3]));
                break;
            case CustomInspectorValueType::Vector3:
                hasChanged = ImGui::ColorEdit3(widget.m_Label.c_str(), value.m_VectorValue);
                break;
            case CustomInspectorValueType::Vector4:
                hasChanged = ImGui::ColorEdit4(widget.m_Label.c_str(), value.m_VectorValue);
                break;
            case CustomInspectorValueType::String:
                hasChanged          = ImGui::ColorEdit4(widget.m_Label.c_str(), value.m_VectorValue);
                value.m_StringValue = ColorConvertToHexString(value.m_VectorValue[0], value.m_VectorValue[1], value.m_VectorValue[2], value.m_VectorValue[3]);
                break;
            case CustomInspectorValueType::Float:
            case CustomInspectorValueType::Bool:
            case CustomInspectorValueType::Vector2:
            case CustomInspectorValueType::Texture:
                throw std::runtime_error(std::string("Invalid data type for Color"));
            default:
                throw std::runtime_error(std::string("Invalid data type for Color"));
        }
        return hasChanged;
    }

    bool CustomInspector::RenderTexture(const CustomInspectorWidget &widget)
    {
        bool hasChanged = false;

        auto &value = m_Values[widget.m_VariableName];
        switch (value.GetType()) {
            case CustomInspectorValueType::Texture:
                break;
            case CustomInspectorValueType::Int:
            case CustomInspectorValueType::Float:
            case CustomInspectorValueType::Vector2:
            case CustomInspectorValueType::Vector3:
            case CustomInspectorValueType::Vector4:
            case CustomInspectorValueType::Bool:
                throw std::runtime_error(std::string("Invalid data type for Drag"));
            default:
                throw std::runtime_error(std::string("Invalid data type for Drag"));
        }
        ImTextureID textureID = value.m_TextureValue ? (ImTextureID)(int64_t)value.m_TextureValue->GetRendererID() : static_cast<ImTextureID>(0);
        if (ImGui::ImageButton(textureID, ImVec2(widget.m_Constratins[0], widget.m_Constratins[1]))) {
            std::string path = ShowOpenFileDialog("*.*");
            if (path.size() > 3) {
                value.m_TextureValue = std::make_shared<Texture2D>(path, false, false, value.m_TextureLoadAs16Bit);
                hasChanged           = true;
            }
        }
        return hasChanged;
    }

    bool CustomInspector::RenderDropdown(const CustomInspectorWidget &widget)
    {
        bool hasChanged = false;
        auto &value     = m_Values[widget.m_VariableName];
        switch (value.GetType()) {
            case CustomInspectorValueType::Int:
                break;
            case CustomInspectorValueType::Float:
            case CustomInspectorValueType::Vector2:
            case CustomInspectorValueType::Vector3:
            case CustomInspectorValueType::Vector4:
            case CustomInspectorValueType::String:
            case CustomInspectorValueType::Bool:
            case CustomInspectorValueType::Texture:
                throw std::runtime_error(std::string("Invalid data type for Dropdown"));
            default:
                throw std::runtime_error(std::string("Invalid data type for Dropdown"));
        }

        if (widget.m_DropdownOptions.empty())
            return false;
        value.m_IntValue = glm::clamp(value.m_IntValue, 0, static_cast<int32_t>(widget.m_DropdownOptions.size()) - 1);
        if (ImGui::BeginCombo(widget.m_Label.c_str(), widget.m_DropdownOptions[value.m_IntValue].c_str())) {
            for (int i = 0; i < static_cast<int32_t>(widget.m_DropdownOptions.size()); i++) {
                bool isSelected = (value.m_IntValue == i);
                if (ImGui::Selectable(widget.m_DropdownOptions[i].c_str(), isSelected)) {
                    value.m_IntValue = i;
                    hasChanged       = true;
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        return hasChanged;
    }

    bool CustomInspector::RenderButton(const CustomInspectorWidget &widget)
    {
        return ImGui::Button(widget.m_Label.c_str());
    }

    bool CustomInspector::RenderPath(const CustomInspectorWidget &widget)
    {
        auto &value = m_Values[widget.m_VariableName];
        if (value.GetType() != CustomInspectorValueType::Path)
            throw std::runtime_error("Invalid data type for Path");
        return utils::DrawPathEditor<CustomInspectorMaxPathPoints>(
            widget.m_Label.c_str(), value.m_PathPoints, value.m_PathPointCount);
    }

    bool CustomInspector::RenderCurve(const CustomInspectorWidget &widget)
    {
        auto &value = m_Values[widget.m_VariableName];
        if (value.GetType() != CustomInspectorValueType::Curve)
            throw std::runtime_error("Invalid data type for Curve");

        std::array<ImVec2, CustomInspectorMaxCurvePoints> points{};
        for (size_t index = 0; index < CustomInspectorMaxCurvePoints; ++index)
            points[index] = ImVec2(value.m_CurvePoints[index].x, value.m_CurvePoints[index].y);

        const float width            = std::max(ImGui::GetContentRegionAvail().x, 220.0f);
        const std::string curveLabel = widget.m_Label + "##" + widget.m_VariableName;
        const bool changed           = ImGui::Curve(curveLabel.c_str(), ImVec2(width, 180.0f),
                                                    static_cast<int>(CustomInspectorMaxCurvePoints), points.data()) != 0;
        int pointCount               = 0;
        while (pointCount < static_cast<int>(CustomInspectorMaxCurvePoints) && points[pointCount].x >= 0.0f)
            ++pointCount;
        pointCount = std::clamp(pointCount, 2, static_cast<int>(CustomInspectorMaxCurvePoints));
        for (size_t index = 0; index < CustomInspectorMaxCurvePoints; ++index)
            value.m_CurvePoints[index] = glm::vec2(points[index].x, points[index].y);
        value.m_CurvePointCount = pointCount;
        return changed;
    }

    bool CustomInspector::RenderCheckbox(const CustomInspectorWidget &widget)
    {
        bool hasChanged = false;
        auto &value     = m_Values[widget.m_VariableName];
        hasChanged      = ImGui::Checkbox(widget.m_Label.c_str(), &value.m_BoolValue);
        switch (value.GetType()) {
            case CustomInspectorValueType::Int:
                value.m_IntValue = (value.m_BoolValue ? 1 : 0);
                ;
                break;
            case CustomInspectorValueType::Float:
                value.m_FloatValue = (value.m_BoolValue ? 1.0f : 0.0f);
                break;
            case CustomInspectorValueType::Vector2:
            case CustomInspectorValueType::Vector3:
            case CustomInspectorValueType::Vector4:
                value.m_VectorValue[0] = (value.m_BoolValue ? 1.0f : 0.0f);
                value.m_VectorValue[1] = (value.m_BoolValue ? 1.0f : 0.0f);
                value.m_VectorValue[2] = (value.m_BoolValue ? 1.0f : 0.0f);
                value.m_VectorValue[3] = (value.m_BoolValue ? 1.0f : 0.0f);
                break;
            case CustomInspectorValueType::String:
                value.m_StringValue = (value.m_BoolValue ? "true" : "false");
                break;
            case CustomInspectorValueType::Bool:
                break;
            case CustomInspectorValueType::Texture:
                throw std::runtime_error(std::string("Invalid data type for Checkbox"));
            default:
                throw std::runtime_error(std::string("Invalid data type for Checkbox"));
        }
        return hasChanged;
    }

    bool CustomInspector::RenderInput(const CustomInspectorWidget &widget)
    {
        bool hasChanged = false;
        auto &value     = m_Values[widget.m_VariableName];
        switch (value.GetType()) {
            case CustomInspectorValueType::Int:
                hasChanged = ImGui::InputInt(widget.m_Label.c_str(), &value.m_IntValue, widget.m_ISpeed, widget.m_ISpeed * 10);
                break;
            case CustomInspectorValueType::Float:
                hasChanged = ImGui::InputFloat(widget.m_Label.c_str(), &value.m_FloatValue, widget.m_FSpeed, widget.m_FSpeed * 10.0f);
                break;
            case CustomInspectorValueType::Vector2:
                hasChanged = ImGui::InputFloat2(widget.m_Label.c_str(), value.m_VectorValue);
                break;
            case CustomInspectorValueType::Vector3:
                hasChanged = ImGui::InputFloat3(widget.m_Label.c_str(), value.m_VectorValue);
                break;
            case CustomInspectorValueType::Vector4:
                hasChanged = ImGui::InputFloat4(widget.m_Label.c_str(), value.m_VectorValue);
                break;
            case CustomInspectorValueType::String:
                static char s_Buffer[4096];
                std::strcpy(s_Buffer, value.m_StringValue.c_str());
                hasChanged          = ImGui::InputText(widget.m_Label.c_str(), s_Buffer, sizeof(s_Buffer));
                value.m_StringValue = s_Buffer;
                break;
            case CustomInspectorValueType::Bool:
            case CustomInspectorValueType::Texture:
                throw std::runtime_error(std::string("Invalid data type for Input"));
            default:
                throw std::runtime_error(std::string("Invalid data type for Input"));
        }
        return hasChanged;
    }

    bool CustomInspector::RenderSeed(CustomInspectorWidget &widget)
    {
        bool hasChanged = false;
        auto &value     = m_Values[widget.m_VariableName];
        switch (value.GetType()) {
            case CustomInspectorValueType::Int:
                hasChanged = ShowSeedSettings(widget.m_Label, &value.m_IntValue, widget.m_SeedHistory);
                break;
            case CustomInspectorValueType::Float:
                hasChanged         = ShowSeedSettings(widget.m_Label, &value.m_IntValue, widget.m_SeedHistory);
                value.m_FloatValue = static_cast<float>(value.m_IntValue);
                break;
            case CustomInspectorValueType::String:
                if (ImGui::Button(("Seed Value: " + value.m_StringValue + " [Click to change]").c_str())) {
                    value.m_StringValue = GenerateId(8);
                    hasChanged          = true;
                }
                break;
            case CustomInspectorValueType::Vector2:
            case CustomInspectorValueType::Vector3:
            case CustomInspectorValueType::Vector4:
            case CustomInspectorValueType::Bool:
            case CustomInspectorValueType::Texture: // todo: add seed texture here too
                throw std::runtime_error(std::string("Invalid data type for Seed"));
            default:
                throw std::runtime_error(std::string("Invalid data type for Seed"));
        }
        return hasChanged;
    }

} // namespace tf3d::misc
