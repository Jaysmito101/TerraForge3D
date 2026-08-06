#include "Misc/CustomInspector.h"
#include "Base/Base.h"
#include "UI/ImGuiComponents.h"
#include "Utils/PathEditor.h"
#include "Utils/Utils.h"

#include <algorithm>

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
            case CustomInspectorValueType_Int:
                return "Int";
            case CustomInspectorValueType_Float:
                return "Float";
            case CustomInspectorValueType_Bool:
                return "Bool";
            case CustomInspectorValueType_String:
                return "String";
            case CustomInspectorValueType_Vector2:
                return "Vector2";
            case CustomInspectorValueType_Vector3:
                return "Vector3";
            case CustomInspectorValueType_Vector4:
                return "Vector4";
            case CustomInspectorValueType_Texture:
                return "Texture";
            case CustomInspectorValueType_Path:
                return "Path";
            case CustomInspectorValueType_Curve:
                return "Curve";
            default:
                return "Unknown";
        }
    }

    CustomInspectorValueType CustomInspectorValue::CustomInspectorValueTypeFromString(const std::string &type)
    {
        if (type == "Int")
            return CustomInspectorValueType_Int;
        if (type == "Float")
            return CustomInspectorValueType_Float;
        if (type == "Bool")
            return CustomInspectorValueType_Bool;
        if (type == "String")
            return CustomInspectorValueType_String;
        if (type == "Vector2")
            return CustomInspectorValueType_Vector2;
        if (type == "Vector3")
            return CustomInspectorValueType_Vector3;
        if (type == "Vector4")
            return CustomInspectorValueType_Vector4;
        if (type == "Texture")
            return CustomInspectorValueType_Texture;
        if (type == "Path")
            return CustomInspectorValueType_Path;
        if (type == "Curve")
            return CustomInspectorValueType_Curve;
        return CustomInspectorValueType_Unknown;
    }

    SerializerNode CustomInspectorValue::Save() const
    {
        SerializerNode node = CreateSerializerNode();
        node->SetString("Name", m_Name);
        node->SetInteger("Type", static_cast<int32_t>(m_Type));
        node->SetString("TypeName", CustomInspectorValueTypeToString(m_Type));
        switch (m_Type) {
            case CustomInspectorValueType_Int:
                node->SetInteger("Value", m_IntValue);
                node->SetInteger("DefaultValue", m_DefaultIntValue);
                break;
            case CustomInspectorValueType_Float:
                node->SetFloat("Value", m_FloatValue);
                node->SetFloat("DefaultValue", m_DefaultFloatValue);
                break;
            case CustomInspectorValueType_Bool:
                node->SetString("Value", m_BoolValue ? "true" : "false");
                node->SetString("DefaultValue", m_DefaultBoolValue ? "true" : "false");
                break;
            case CustomInspectorValueType_String:
                node->SetString("Value", m_StringValue);
                node->SetString("DefaultValue", m_DefaultStringValue);
                break;
            case CustomInspectorValueType_Vector2:
                node->SetFloat("ValueX", m_VectorValue[0]);
                node->SetFloat("ValueY", m_VectorValue[1]);
                node->SetFloat("DefaultValueX", m_DefaultVectorValue[0]);
                node->SetFloat("DefaultValueY", m_DefaultVectorValue[1]);
                break;
            case CustomInspectorValueType_Vector3:
                node->SetFloat("ValueX", m_VectorValue[0]);
                node->SetFloat("ValueY", m_VectorValue[1]);
                node->SetFloat("ValueZ", m_VectorValue[2]);
                node->SetFloat("DefaultValueX", m_DefaultVectorValue[0]);
                node->SetFloat("DefaultValueY", m_DefaultVectorValue[1]);
                node->SetFloat("DefaultValueZ", m_DefaultVectorValue[2]);
                break;
            case CustomInspectorValueType_Vector4:
                node->SetFloat("ValueX", m_VectorValue[0]);
                node->SetFloat("ValueY", m_VectorValue[1]);
                node->SetFloat("ValueZ", m_VectorValue[2]);
                node->SetFloat("ValueW", m_VectorValue[3]);
                node->SetFloat("DefaultValueX", m_DefaultVectorValue[0]);
                node->SetFloat("DefaultValueY", m_DefaultVectorValue[1]);
                node->SetFloat("DefaultValueZ", m_DefaultVectorValue[2]);
                node->SetFloat("DefaultValueW", m_DefaultVectorValue[3]);
                break;
            case CustomInspectorValueType_Texture:
                node->SetFile("Value", m_TextureValue ? m_TextureValue->GetPath() : "null");
                node->SetFile("DefaultValue", m_DefaultTextureValue ? m_DefaultTextureValue->GetPath() : "null");
                node->SetInteger("TextureBitDepth", m_TextureLoadAs16Bit ? 16 : 8);
                break;
            case CustomInspectorValueType_Path:
                node->SetInteger("PathPointCount", m_PathPointCount);
                node->SetInteger("DefaultPathPointCount", m_DefaultPathPointCount);
                for (size_t index = 0; index < CustomInspectorMaxPathPoints; ++index) {
                    node->SetFloat("PathValueX" + std::to_string(index), m_PathPoints[index].x);
                    node->SetFloat("PathValueY" + std::to_string(index), m_PathPoints[index].y);
                    node->SetFloat("DefaultPathValueX" + std::to_string(index), m_DefaultPathPoints[index].x);
                    node->SetFloat("DefaultPathValueY" + std::to_string(index), m_DefaultPathPoints[index].y);
                }
                break;
            case CustomInspectorValueType_Curve:
                node->SetInteger("CurvePointCount", m_CurvePointCount);
                node->SetInteger("DefaultCurvePointCount", m_DefaultCurvePointCount);
                for (size_t index = 0; index < CustomInspectorMaxCurvePoints; ++index) {
                    node->SetFloat("CurveValueX" + std::to_string(index), m_CurvePoints[index].x);
                    node->SetFloat("CurveValueY" + std::to_string(index), m_CurvePoints[index].y);
                    node->SetFloat("DefaultCurveValueX" + std::to_string(index), m_DefaultCurvePoints[index].x);
                    node->SetFloat("DefaultCurveValueY" + std::to_string(index), m_DefaultCurvePoints[index].y);
                }
                break;
            default:
                break;
        }
        return node;
    }

    void CustomInspectorValue::Load(const SerializerNode &node)
    {
        m_Type = CustomInspectorValueTypeFromString(node->GetString("TypeName", CustomInspectorValueTypeToString(m_Type)));
        m_Name = node->GetString("Name");
        switch (m_Type) {
            case CustomInspectorValueType_Int:
                m_DefaultIntValue = node->GetInteger("DefaultValue", m_DefaultIntValue);
                m_IntValue        = node->GetInteger("Value", m_DefaultIntValue);
                break;
            case CustomInspectorValueType_Float:
                m_DefaultFloatValue = node->GetFloat("DefaultValue", m_DefaultFloatValue);
                m_FloatValue        = node->GetFloat("Value", m_DefaultFloatValue);
                break;
            case CustomInspectorValueType_Bool:
                m_DefaultBoolValue = node->GetString("DefaultValue", m_DefaultBoolValue ? "true" : "false") == "true";
                m_BoolValue        = node->GetString("Value", m_DefaultBoolValue ? "true" : "false") == "true";
                break;
            case CustomInspectorValueType_String:
                m_DefaultStringValue = node->GetString("DefaultValue", m_DefaultStringValue);
                m_StringValue        = node->GetString("Value", m_DefaultStringValue);
                break;
            case CustomInspectorValueType_Vector2:
                m_DefaultVectorValue[0] = node->GetFloat("DefaultValueX", m_DefaultVectorValue[0]);
                m_DefaultVectorValue[1] = node->GetFloat("DefaultValueY", m_DefaultVectorValue[1]);
                m_VectorValue[0]        = node->GetFloat("ValueX", m_DefaultVectorValue[0]);
                m_VectorValue[1]        = node->GetFloat("ValueY", m_DefaultVectorValue[1]);
                break;
            case CustomInspectorValueType_Vector3:
                m_DefaultVectorValue[0] = node->GetFloat("DefaultValueX", m_DefaultVectorValue[0]);
                m_DefaultVectorValue[1] = node->GetFloat("DefaultValueY", m_DefaultVectorValue[1]);
                m_DefaultVectorValue[2] = node->GetFloat("DefaultValueZ", m_DefaultVectorValue[2]);
                m_VectorValue[0]        = node->GetFloat("ValueX", m_DefaultVectorValue[0]);
                m_VectorValue[1]        = node->GetFloat("ValueY", m_DefaultVectorValue[1]);
                m_VectorValue[2]        = node->GetFloat("ValueZ", m_DefaultVectorValue[2]);
                break;
            case CustomInspectorValueType_Vector4:
                m_DefaultVectorValue[0] = node->GetFloat("DefaultValueX", m_DefaultVectorValue[0]);
                m_DefaultVectorValue[1] = node->GetFloat("DefaultValueY", m_DefaultVectorValue[1]);
                m_DefaultVectorValue[2] = node->GetFloat("DefaultValueZ", m_DefaultVectorValue[2]);
                m_DefaultVectorValue[3] = node->GetFloat("DefaultValueW", m_DefaultVectorValue[3]);
                m_VectorValue[0]        = node->GetFloat("ValueX", m_DefaultVectorValue[0]);
                m_VectorValue[1]        = node->GetFloat("ValueY", m_DefaultVectorValue[1]);
                m_VectorValue[2]        = node->GetFloat("ValueZ", m_DefaultVectorValue[2]);
                m_VectorValue[3]        = node->GetFloat("ValueW", m_DefaultVectorValue[3]);
                break;
            case CustomInspectorValueType_Texture: {
                m_TextureLoadAs16Bit    = node->GetInteger("TextureBitDepth", m_TextureLoadAs16Bit ? 16 : 8) >= 16;
                const auto &defaultPath = node->GetFile("DefaultValue", m_DefaultTextureValue ? m_DefaultTextureValue->GetPath() : "");
                const auto &path        = node->GetFile("Value", m_DefaultTextureValue ? m_DefaultTextureValue->GetPath() : "");
                auto loadTexture        = [this](const std::string &texturePath) -> std::shared_ptr<Texture2D> {
                    if (texturePath.empty() || texturePath == "null")
                        return nullptr;
                    return std::make_shared<Texture2D>(texturePath, false, false, m_TextureLoadAs16Bit);
                };
                m_DefaultTextureValue = loadTexture(defaultPath);
                m_TextureValue        = loadTexture(path);
                break;
            }
            case CustomInspectorValueType_Path:
                m_DefaultPathPointCount = glm::clamp(node->GetInteger("DefaultPathPointCount", m_DefaultPathPointCount), 1, static_cast<int32_t>(CustomInspectorMaxPathPoints));
                m_PathPointCount        = glm::clamp(node->GetInteger("PathPointCount", m_DefaultPathPointCount), 1, static_cast<int32_t>(CustomInspectorMaxPathPoints));
                for (size_t index = 0; index < CustomInspectorMaxPathPoints; ++index) {
                    m_DefaultPathPoints[index].x = node->GetFloat("DefaultPathValueX" + std::to_string(index), m_DefaultPathPoints[index].x);
                    m_DefaultPathPoints[index].y = node->GetFloat("DefaultPathValueY" + std::to_string(index), m_DefaultPathPoints[index].y);
                    m_PathPoints[index].x        = node->GetFloat("PathValueX" + std::to_string(index), m_DefaultPathPoints[index].x);
                    m_PathPoints[index].y        = node->GetFloat("PathValueY" + std::to_string(index), m_DefaultPathPoints[index].y);
                }
                break;
            case CustomInspectorValueType_Curve:
                m_DefaultCurvePointCount = glm::clamp(node->GetInteger("DefaultCurvePointCount", m_DefaultCurvePointCount), 2, static_cast<int32_t>(CustomInspectorMaxCurvePoints));
                m_CurvePointCount        = glm::clamp(node->GetInteger("CurvePointCount", m_DefaultCurvePointCount), 2, static_cast<int32_t>(CustomInspectorMaxCurvePoints));
                for (size_t index = 0; index < CustomInspectorMaxCurvePoints; ++index) {
                    m_DefaultCurvePoints[index].x = node->GetFloat("DefaultCurveValueX" + std::to_string(index), m_DefaultCurvePoints[index].x);
                    m_DefaultCurvePoints[index].y = node->GetFloat("DefaultCurveValueY" + std::to_string(index), m_DefaultCurvePoints[index].y);
                    m_CurvePoints[index].x        = node->GetFloat("CurveValueX" + std::to_string(index), m_DefaultCurvePoints[index].x);
                    m_CurvePoints[index].y        = node->GetFloat("CurveValueY" + std::to_string(index), m_DefaultCurvePoints[index].y);
                }
                break;
            default:
                break;
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
        node->SetInteger("Type", static_cast<int32_t>(m_Type));
        node->SetString("TypeName", CustomInspectorWidgetTypeToString(m_Type));
        node->SetString("TargetVariable", m_VariableName);
        node->SetString("Label", m_Label);
        if (m_Type == CustomInspectorWidgetType_Seed)
            node->SetIntegerArray("SeedHistory", m_SeedHistory);
        node->SetInteger("ISpeed", m_ISpeed);
        node->SetFloat("FSeed", m_FSpeed);
        node->SetString("ID", m_ID);
        node->SetFloat("Contraints_0", m_Constratins[0]);
        node->SetFloat("Contraints_1", m_Constratins[1]);
        node->SetFloat("Contraints_2", m_Constratins[2]);
        node->SetFloat("Contraints_3", m_Constratins[3]);
        if (m_UseRenderOnCondition) {
            node->SetInteger("RenderOnConditionValue", m_RenderOnConditionValue);
            node->SetString("RenderOnConditionName", m_RenderOnConditionName);
            node->SetInteger("UseRenderOnCondition", m_UseRenderOnCondition ? 1 : 0);
            if (!m_RenderOnConditionValues.empty())
                node->SetIntegerArray("RenderOnConditionValues", m_RenderOnConditionValues);
        }
        if (m_FontName.size() > 0)
            node->SetString("FontName", m_FontName);
        if (m_Tooltip.size() > 0)
            node->SetString("Tooltip", m_Tooltip);
        return node;
    }

    void CustomInspectorWidget::Load(SerializerNode node)
    {
        m_Type         = CustomInspectorWidgetTypeFromString(node->GetString("TypeName", CustomInspectorWidgetTypeToString(m_Type)));
        m_VariableName = node->GetString("TargetVariable", m_VariableName);
        m_Label        = node->GetString("Label", m_Label);
        m_ID           = node->GetString("ID", m_ID);
        if (m_Type == CustomInspectorWidgetType_Seed)
            m_SeedHistory = node->GetIntegerArray("SeedHistory", m_SeedHistory);
        m_ISpeed               = node->GetInteger("ISpeed", m_ISpeed);
        m_FSpeed               = node->GetFloat("FSeed", m_FSpeed);
        m_Constratins[0]       = node->GetFloat("Contraints_0", m_Constratins[0]);
        m_Constratins[1]       = node->GetFloat("Contraints_1", m_Constratins[1]);
        m_Constratins[2]       = node->GetFloat("Contraints_2", m_Constratins[2]);
        m_Constratins[3]       = node->GetFloat("Contraints_3", m_Constratins[3]);
        m_UseRenderOnCondition = node->GetInteger("UseRenderOnCondition", 0) == 1;
        if (m_UseRenderOnCondition) {
            m_RenderOnConditionValue  = node->GetInteger("RenderOnConditionValue", m_RenderOnConditionValue);
            m_RenderOnConditionName   = node->GetString("RenderOnConditionName", m_RenderOnConditionName);
            m_RenderOnConditionValues = node->GetIntegerArray("RenderOnConditionValues", {});
            if (m_RenderOnConditionValues.empty())
                m_RenderOnConditionValues.push_back(m_RenderOnConditionValue);
        }
        m_Tooltip  = node->GetString("Tooltip", m_Tooltip);
        m_FontName = node->GetString("FontName", m_FontName);
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

    CustomInspectorValue &CustomInspector::GetVariable(const std::string &name)
    {
        return m_Values[name];
    }

    bool CustomInspector::HasVariable(const std::string &name)
    {
        return m_Values.find(name) != m_Values.end();
    }

    void CustomInspector::RemoveVariable(const std::string &name)
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

    CustomInspectorValue &CustomInspector::AddStringVariable(const std::string &name, const std::string &defaultValue)
    {
        CustomInspectorValue value(CustomInspectorValueType_String);
        value.m_StringValue = value.m_DefaultStringValue = defaultValue;
        return AddVariable(name, value);
    }

    CustomInspectorValue &CustomInspector::AddIntegerVariable(const std::string &name, int defaultValue)
    {
        CustomInspectorValue value(CustomInspectorValueType_Int);
        value.m_IntValue = value.m_DefaultIntValue = defaultValue;
        return AddVariable(name, value);
    }

    CustomInspectorValue &CustomInspector::AddFloatVariable(const std::string &name, float defaultValue)
    {
        CustomInspectorValue value(CustomInspectorValueType_Float);
        value.m_FloatValue = value.m_DefaultFloatValue = defaultValue;
        return AddVariable(name, value);
    }

    CustomInspectorValue &CustomInspector::AddBoolVariable(const std::string &name, bool defaultValue)
    {
        CustomInspectorValue value(CustomInspectorValueType_Bool);
        value.m_BoolValue = value.m_DefaultBoolValue = defaultValue;
        return AddVariable(name, value);
    }

    CustomInspectorValue &CustomInspector::AddVector2Variable(const std::string &name, glm::vec2 defaultValue)
    {
        CustomInspectorValue value(CustomInspectorValueType_Vector2);
        value.m_DefaultVectorValue[0] = value.m_VectorValue[0] = defaultValue.x;
        value.m_DefaultVectorValue[1] = value.m_VectorValue[1] = defaultValue.y;
        return AddVariable(name, value);
    }

    CustomInspectorValue &CustomInspector::AddVector3Variable(const std::string &name, glm::vec3 defaultValue)
    {
        CustomInspectorValue value(CustomInspectorValueType_Vector3);
        value.m_DefaultVectorValue[0] = value.m_VectorValue[0] = defaultValue.x;
        value.m_DefaultVectorValue[1] = value.m_VectorValue[1] = defaultValue.y;
        value.m_DefaultVectorValue[2] = value.m_VectorValue[2] = defaultValue.z;
        return AddVariable(name, value);
    }

    CustomInspectorValue &CustomInspector::AddVector4Variable(const std::string &name, glm::vec4 defaultValue)
    {
        CustomInspectorValue value(CustomInspectorValueType_Vector4);
        value.m_DefaultVectorValue[0] = value.m_VectorValue[0] = defaultValue.x;
        value.m_DefaultVectorValue[1] = value.m_VectorValue[1] = defaultValue.y;
        value.m_DefaultVectorValue[2] = value.m_VectorValue[2] = defaultValue.z;
        value.m_DefaultVectorValue[3] = value.m_VectorValue[3] = defaultValue.w;
        return AddVariable(name, value);
    }

    CustomInspectorValue &CustomInspector::AddTextureVariable(const std::string &name, std::shared_ptr<Texture2D> defaultValue)
    {
        CustomInspectorValue value(CustomInspectorValueType_Texture);
        value.m_TextureValue = value.m_DefaultTextureValue = defaultValue;
        return AddVariable(name, value);
    }

    CustomInspectorValue &CustomInspector::AddPathVariable(const std::string &name,
                                                           const std::array<glm::vec2, CustomInspectorMaxPathPoints> &defaultPoints, int defaultPointCount)
    {
        CustomInspectorValue value(CustomInspectorValueType_Path);
        value.m_DefaultPathPoints = value.m_PathPoints = defaultPoints;
        value.m_DefaultPathPointCount = value.m_PathPointCount = glm::clamp(defaultPointCount, 1, static_cast<int>(CustomInspectorMaxPathPoints));
        value.m_Name                                           = name;
        return AddVariable(name, value);
    }

    CustomInspectorValue &CustomInspector::AddCurveVariable(const std::string &name,
                                                            const std::array<glm::vec2, CustomInspectorMaxCurvePoints> &defaultPoints, int defaultPointCount)
    {
        CustomInspectorValue value(CustomInspectorValueType_Curve);
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
            case CustomInspectorValueType_Int: {
                auto &var  = AddIntegerVariable(name, hasDefaultValue ? config["Default"].get<int32_t>() : 0);
                var.m_Name = name;
                return var;
            }
            case CustomInspectorValueType_Float: {
                auto &var  = AddFloatVariable(name, hasDefaultValue ? config["Default"].get<float>() : 0.0f);
                var.m_Name = name;
                return var;
            }
            case CustomInspectorValueType_Bool: {
                auto &var  = AddBoolVariable(name, hasDefaultValue ? config["Default"].get<bool>() : false);
                var.m_Name = name;
                return var;
            }
            case CustomInspectorValueType_String: {
                auto &var  = AddStringVariable(name, hasDefaultValue ? config["Default"].get<std::string>() : "");
                var.m_Name = name;
                return var;
            }
            case CustomInspectorValueType_Vector2: {
                auto &var  = AddVector2Variable(name, hasDefaultValue ? glm::vec2(config["Default"][0].get<float>(), config["Default"][1].get<float>()) : glm::vec2(0.0f));
                var.m_Name = name;
                return var;
            }
            case CustomInspectorValueType_Vector3: {
                auto &var  = AddVector3Variable(name, hasDefaultValue ? glm::vec3(config["Default"][0].get<float>(), config["Default"][1].get<float>(), config["Default"][2].get<float>()) : glm::vec3(0.0f));
                var.m_Name = name;
                return var;
            }
            case CustomInspectorValueType_Vector4: {
                auto &var  = AddVector4Variable(name, hasDefaultValue ? glm::vec4(config["Default"][0].get<float>(), config["Default"][1].get<float>(), config["Default"][2].get<float>(), config["Default"][3].get<float>()) : glm::vec4(0.0f));
                var.m_Name = name;
                return var;
            }
            case CustomInspectorValueType_Texture: {
                const bool loadAs16Bit                    = config.value("BitDepth", 8) >= 16;
                std::shared_ptr<Texture2D> defaultTexture = nullptr;
                if (hasDefaultValue && config["Default"].is_string()) {
                    const std::string path = config["Default"].get<std::string>();
                    if (!path.empty() && path != "null")
                        defaultTexture = std::make_shared<Texture2D>(path, true, false, loadAs16Bit);
                }
                auto &var                = AddTextureVariable(name, defaultTexture);
                var.m_TextureLoadAs16Bit = loadAs16Bit;
                var.m_Name               = name;
                return var;
            }
            case CustomInspectorValueType_Path: {
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
            case CustomInspectorValueType_Curve: {
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
        if (name == "Seperator" || name == "NewLine")
            return GetWidget(name);
        m_Widgets[name] = widget;
        m_WidgetsOrder.push_back(name);
        return m_Widgets[name];
    }

    CustomInspectorWidget &CustomInspector::AddSliderWidget(const std::string &label, const std::string &variableName, float min, float max)
    {
        CustomInspectorWidget widget(CustomInspectorWidgetType_Slider);
        widget.SetLabel(label);
        widget.SetVariableName(variableName);
        widget.m_Constratins[0] = min;
        widget.m_Constratins[1] = max;
        return AddWidget(label, widget);
    }

    CustomInspectorWidget &CustomInspector::AddDragWidget(const std::string &label, const std::string &variableName, float min, float max, float speed)
    {
        CustomInspectorWidget widget(CustomInspectorWidgetType_Drag);
        widget.SetLabel(label);
        widget.SetVariableName(variableName);
        widget.m_Constratins[0] = min;
        widget.m_Constratins[1] = max;
        widget.m_FSpeed         = speed;
        widget.m_ISpeed         = static_cast<int32_t>(speed);
        return AddWidget(label, widget);
    }

    CustomInspectorWidget &CustomInspector::AddColorWidget(const std::string &label, const std::string &variableName)
    {
        CustomInspectorWidget widget(CustomInspectorWidgetType_Color);
        widget.SetLabel(label);
        widget.SetVariableName(variableName);
        return AddWidget(label, widget);
    }

    CustomInspectorWidget &CustomInspector::AddTextureWidget(const std::string &label, const std::string &variableName, float widht, float height)
    {
        CustomInspectorWidget widget(CustomInspectorWidgetType_Texture);
        widget.SetLabel(label);
        widget.SetVariableName(variableName);
        widget.m_Constratins[0] = widht;
        widget.m_Constratins[1] = height;
        return AddWidget(label, widget);
    }

    CustomInspectorWidget &CustomInspector::AddButtonWidget(const std::string &label, const std::string &actionName)
    {
        CustomInspectorWidget widget(CustomInspectorWidgetType_Button);
        widget.SetLabel(label);
        widget.SetActionName(actionName);
        return AddWidget(label, widget);
    }

    CustomInspectorWidget &CustomInspector::AddCheckboxWidget(const std::string &label, const std::string &variableName)
    {
        CustomInspectorWidget widget(CustomInspectorWidgetType_Checkbox);
        widget.SetLabel(label);
        widget.SetVariableName(variableName);
        return AddWidget(label, widget);
    }

    CustomInspectorWidget &CustomInspector::AddInputWidget(const std::string &label, const std::string &variableName)
    {
        CustomInspectorWidget widget(CustomInspectorWidgetType_Input);
        widget.SetLabel(label);
        widget.SetVariableName(variableName);
        return AddWidget(label, widget);
    }

    CustomInspectorWidget &CustomInspector::AddSeedWidget(const std::string &label, const std::string &variableName)
    {
        CustomInspectorWidget widget(CustomInspectorWidgetType_Seed);
        widget.SetLabel(label);
        widget.SetVariableName(variableName);
        return AddWidget(label, widget);
    }

    CustomInspectorWidget &CustomInspector::AddDropdownWidget(const std::string &label, const std::string &variableName, const std::vector<std::string> &options)
    {
        CustomInspectorWidget widget(CustomInspectorWidgetType_Dropdown);
        widget.SetLabel(label);
        widget.SetVariableName(variableName);
        widget.m_DropdownOptions = options;
        return AddWidget(label, widget);
    }

    CustomInspectorWidget &CustomInspector::AddPathWidget(const std::string &label, const std::string &variableName)
    {
        CustomInspectorWidget widget(CustomInspectorWidgetType_Path);
        widget.SetLabel(label);
        widget.SetVariableName(variableName);
        return AddWidget(label, widget);
    }

    CustomInspectorWidget &CustomInspector::AddCurveWidget(const std::string &label, const std::string &variableName)
    {
        CustomInspectorWidget widget(CustomInspectorWidgetType_Curve);
        widget.SetLabel(label);
        widget.SetVariableName(variableName);
        return AddWidget(label, widget);
    }

    CustomInspectorWidget &CustomInspector::AddSeperatorWidget()
    {
        return AddWidget("Seperator", CustomInspectorWidget(CustomInspectorWidgetType_Seperator));
    }

    CustomInspectorWidget &CustomInspector::AddNewLineWidget()
    {
        return AddWidget("NewLine", CustomInspectorWidget(CustomInspectorWidgetType_NewLine));
    }

    CustomInspectorWidget &CustomInspector::AddTextWidget(const std::string &label, const std::string &font)
    {
        CustomInspectorWidget widget(CustomInspectorWidgetType_Text);
        widget.SetLabel(label);
        widget.m_FontName = font;
        return AddWidget(label, widget);
    }

    CustomInspectorWidget &CustomInspector::AddWidgetFromString(const std::string &label, const std::string &type, const std::string &variableName)
    {
        auto widgetType = CustomInspectorWidget::CustomInspectorWidgetTypeFromString(type);
        switch (widgetType) {
            case CustomInspectorWidgetType_Slider:
                return AddSliderWidget(label, variableName);
            case CustomInspectorWidgetType_Drag:
                return AddDragWidget(label, variableName);
            case CustomInspectorWidgetType_Color:
                return AddColorWidget(label, variableName);
            case CustomInspectorWidgetType_Texture:
                return AddTextureWidget(label, variableName);
            case CustomInspectorWidgetType_Path:
                return AddPathWidget(label, variableName);
            case CustomInspectorWidgetType_Curve:
                return AddCurveWidget(label, variableName);
            case CustomInspectorWidgetType_Button:
                return AddButtonWidget(label, variableName);
            case CustomInspectorWidgetType_Checkbox:
                return AddCheckboxWidget(label, variableName);
            case CustomInspectorWidgetType_Input:
                return AddInputWidget(label, variableName);
            case CustomInspectorWidgetType_Seed:
                return AddSeedWidget(label, variableName);
            case CustomInspectorWidgetType_Dropdown:
                return AddDropdownWidget(label, variableName, {});
            case CustomInspectorWidgetType_Seperator:
                return AddSeperatorWidget();
            case CustomInspectorWidgetType_NewLine:
                return AddNewLineWidget();
            case CustomInspectorWidgetType_Text:
                return AddTextWidget(label, variableName);
            case CustomInspectorWidgetType_Unknown:
            default:
                throw std::runtime_error(std::string("Invalid data type for Drag"));
        }
    }

    SerializerNode CustomInspector::SaveData() const
    {
        SerializerNode node = CreateSerializerNode();
        node->SetInteger("ValueCount", static_cast<int32_t>(m_Values.size()));
        node->CreateNodeArray("Values");
        for (const auto &it : m_Values) {
            auto subNode = it.second.Save();
            subNode->SetString("GName", it.first);
            node->PushToNodeArray("Values", subNode);
        }
        return node;
    }

    void CustomInspector::LoadData(SerializerNode node)
    {
        std::unordered_map<std::string, bool> textureBitDepths;
        for (const auto &[name, existingValue] : m_Values) {
            if (existingValue.GetType() == CustomInspectorValueType_Texture && existingValue.m_TextureLoadAs16Bit)
                textureBitDepths[name] = true;
        }
        m_Values.clear();
        int32_t valueCount = node->GetInteger("ValueCount");
        auto subNodes      = node->GetNodeArray("Values");
        if (subNodes.size() != valueCount)
            TF3D_LOG_WARN("Inspector data is incomplete: expected {}, found {}", valueCount, subNodes.size());
        for (auto subNode : subNodes) {
            std::string name = subNode->GetString("GName");
            CustomInspectorValue value;
            if (textureBitDepths.contains(name))
                value.m_TextureLoadAs16Bit = true;
            value.Load(subNode);
            m_Values[name] = value;
        }
    }

    SerializerNode CustomInspector::Save() const
    {
        SerializerNode node = CreateSerializerNode();
        node->SetString("ID", m_ID);
        if (!m_Description.empty())
            node->SetString("Description", m_Description);
        node->SetChildNode("Data", SaveData());
        node->SetStringArray("WidgetsOrder", m_WidgetsOrder);
        node->SetInteger("WidgetsCount", static_cast<int32_t>(m_Widgets.size()));
        node->CreateNodeArray("Widgets");
        for (const auto &it : m_Widgets) {
            auto subNode = it.second.Save();
            subNode->SetString("GName", it.first);
            node->PushToNodeArray("Widgets", subNode);
        }
        return node;
    }

    void CustomInspector::Load(SerializerNode node)
    {
        LoadData(node->GetChildNode("Data"));
        m_Widgets.clear();
        m_WidgetsOrder.clear();
        m_ID           = node->GetString("ID", m_ID);
        m_Description  = node->GetString("Description", m_Description);
        int valueCount = node->GetInteger("WidgetsCount");
        m_WidgetsOrder = node->GetStringArray("WidgetsOrder");
        auto subNodes  = node->GetNodeArray("Widgets");
        if (subNodes.size() != valueCount)
            TF3D_LOG_WARN("Inspector data is incomplete: expected {}, found {}", valueCount, subNodes.size());
        for (auto subNode : subNodes) {
            std::string name = subNode->GetString("GName");
            CustomInspectorWidget widget;
            widget.Load(subNode);
            m_Widgets[name] = widget;
        }
        if (m_WidgetsOrder.empty()) {
            for (const auto &subNode : subNodes)
                m_WidgetsOrder.push_back(subNode->GetString("GName"));
        }
    }

    bool CustomInspector::LoadConfig(const nlohmann::json &config)
    {
        Clear();
        m_Description = config.value("Description", "");
        if (!config.contains("Params") || !config["Params"].is_array()) {
            AddTextWidget("No parameters available");
            return true;
        }

        try {
            for (const auto &parameter : config["Params"]) {
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
            if (config.contains("Buttons") && config["Buttons"].is_array()) {
                for (const auto &button : config["Buttons"]) {
                    if (!button.is_object())
                        continue;
                    const std::string action = button.value("Action", button.value("Name", "Action"));
                    const std::string label  = button.value("Label", action);
                    auto &widget             = AddButtonWidget(label, action);
                    if (button.contains("Tooltip"))
                        widget.SetTooltip(button["Tooltip"].get<std::string>());
                    else if (button.contains("Description"))
                        widget.SetTooltip(button["Description"].get<std::string>());
                }
            }
        } catch (const std::exception &exception) {
            TF3D_LOG_ERROR("Failed to load inspector metadata: {}", exception.what());
            return false;
        }
        return true;
    }

    CustomInspectorWidget &CustomInspector::SetWidgetDropdownOptions(const std::string &label, const std::vector<std::string> &options)
    {
        auto &widget             = m_Widgets[label];
        widget.m_DropdownOptions = options;
        return widget;
    }

    CustomInspectorWidget &CustomInspector::SetWidgetConstraints(const std::string &label, float a, float b, float c, float d)
    {
        auto &widget            = m_Widgets[label];
        widget.m_Constratins[0] = a;
        widget.m_Constratins[1] = b;
        widget.m_Constratins[2] = c;
        widget.m_Constratins[3] = d;
        return widget;
    }

    CustomInspectorWidget &CustomInspector::SetWidgetTooltip(const std::string &label, const std::string &value)
    {
        auto &widget     = m_Widgets[label];
        widget.m_Tooltip = value;
        return widget;
    }

    CustomInspectorWidget &CustomInspector::SetWidgetFont(const std::string &label, const std::string &value)
    {
        auto &widget      = m_Widgets[label];
        widget.m_FontName = value;
        return widget;
    }

    CustomInspectorWidget &CustomInspector::SetWidgetSpeed(const std::string &label, float speed)
    {
        auto &widget    = m_Widgets[label];
        widget.m_FSpeed = speed;
        widget.m_ISpeed = static_cast<int32_t>(speed);
        return widget;
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
        for (const auto &widgetLabel : m_WidgetsOrder) {
            const auto &widget = m_Widgets[widgetLabel];
            if (widget.m_UseRenderOnCondition) {
                if (!HasVariable(widget.m_RenderOnConditionName))
                    continue;
                const auto &condition     = m_Values.at(widget.m_RenderOnConditionName);
                const auto &allowedValues = widget.m_RenderOnConditionValues;
                if (!allowedValues.empty()) {
                    if (std::find(allowedValues.begin(), allowedValues.end(), condition.GetInt()) == allowedValues.end())
                        continue;
                } else if (condition.GetInt() != widget.m_RenderOnConditionValue)
                    continue;
            }
            ImGui::PushID(widget.m_ID.c_str());
            if (widget.m_FontName.size() > 0)
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
                hasChanged = true;
                if (widget.m_Type == CustomInspectorWidgetType_Button)
                    m_LastAction = widget.m_VariableName;
                else if (!widget.m_VariableName.empty())
                    m_LastChangedVariable = widget.m_VariableName;
            }
            if (widget.m_FontName.size() > 0)
                ImGui::PopFont();
            RenderInspectorTooltip(widget.m_Label, widget.m_Tooltip);
            if (widget.m_Type != CustomInspectorWidgetType_Seed &&
                widget.m_Type != CustomInspectorWidgetType_Button &&
                !widget.m_VariableName.empty()) {
                if (ImGui::BeginPopupContextItem(widget.m_ID.c_str())) {
                    static char s_ResetButtonName[1024];
                    sprintf(s_ResetButtonName, "Reset Value (%s)", widget.GetLabel().c_str());
                    // BUG: This doesn't work for some reason!
                    if (ImGui::Button(s_ResetButtonName)) {
                        m_Values[widget.m_VariableName].ResetValue();
                        hasChanged            = true;
                        m_LastChangedVariable = widget.m_VariableName;
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::PopID();
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
            case CustomInspectorValueType_Int:
                hasChanged = ImGui::SliderInt(widget.m_Label.c_str(), &value.m_IntValue, static_cast<int32_t>(widget.m_Constratins[0]), static_cast<int32_t>(widget.m_Constratins[1]));
                break;
            case CustomInspectorValueType_Float:
                hasChanged = ImGui::SliderFloat(widget.m_Label.c_str(), &value.m_FloatValue, widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType_Vector2:
                hasChanged = ImGui::SliderFloat2(widget.m_Label.c_str(), value.m_VectorValue, widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType_Vector3:
                hasChanged = ImGui::SliderFloat3(widget.m_Label.c_str(), value.m_VectorValue, widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType_Vector4:
                hasChanged = ImGui::SliderFloat4(widget.m_Label.c_str(), value.m_VectorValue, widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType_String:
            case CustomInspectorValueType_Bool:
            case CustomInspectorValueType_Texture:
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
            case CustomInspectorValueType_Int:
                hasChanged = ImGui::DragInt(widget.m_Label.c_str(), &value.m_IntValue, widget.m_FSpeed, static_cast<int32_t>(widget.m_Constratins[0]), static_cast<int32_t>(widget.m_Constratins[1]));
                if (abs(widget.m_Constratins[0] - widget.m_Constratins[1]) < 0.001f)
                    break; // no constraints
                value.m_IntValue = std::clamp(value.m_IntValue, static_cast<int32_t>(widget.m_Constratins[0]), static_cast<int32_t>(widget.m_Constratins[1]));
                break;
            case CustomInspectorValueType_Float:
                hasChanged = ImGui::DragFloat(widget.m_Label.c_str(), &value.m_FloatValue, widget.m_FSpeed, widget.m_Constratins[0], widget.m_Constratins[1]);
                if (abs(widget.m_Constratins[0] - widget.m_Constratins[1]) < 0.001f)
                    break; // no constraints
                value.m_FloatValue = std::clamp(value.m_FloatValue, widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType_Vector2:
                hasChanged = ImGui::DragFloat2(widget.m_Label.c_str(), value.m_VectorValue, widget.m_FSpeed, widget.m_Constratins[0], widget.m_Constratins[1]);
                if (abs(widget.m_Constratins[0] - widget.m_Constratins[1]) < 0.001f)
                    break; // no constraints
                value.m_VectorValue[0] = std::clamp(value.m_VectorValue[0], widget.m_Constratins[0], widget.m_Constratins[1]);
                value.m_VectorValue[1] = std::clamp(value.m_VectorValue[1], widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType_Vector3:
                hasChanged = ImGui::DragFloat3(widget.m_Label.c_str(), value.m_VectorValue, widget.m_FSpeed, widget.m_Constratins[0], widget.m_Constratins[1]);
                if (abs(widget.m_Constratins[0] - widget.m_Constratins[1]) < 0.001f)
                    break; // no constraints
                value.m_VectorValue[0] = std::clamp(value.m_VectorValue[0], widget.m_Constratins[0], widget.m_Constratins[1]);
                value.m_VectorValue[1] = std::clamp(value.m_VectorValue[1], widget.m_Constratins[0], widget.m_Constratins[1]);
                value.m_VectorValue[2] = std::clamp(value.m_VectorValue[2], widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType_Vector4:
                hasChanged = ImGui::DragFloat4(widget.m_Label.c_str(), value.m_VectorValue, widget.m_FSpeed, widget.m_Constratins[0], widget.m_Constratins[1]);
                if (abs(widget.m_Constratins[0] - widget.m_Constratins[1]) < 0.001f)
                    break; // no constraints
                value.m_VectorValue[0] = std::clamp(value.m_VectorValue[0], widget.m_Constratins[0], widget.m_Constratins[1]);
                value.m_VectorValue[1] = std::clamp(value.m_VectorValue[1], widget.m_Constratins[0], widget.m_Constratins[1]);
                value.m_VectorValue[2] = std::clamp(value.m_VectorValue[2], widget.m_Constratins[0], widget.m_Constratins[1]);
                value.m_VectorValue[3] = std::clamp(value.m_VectorValue[3], widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType_String:
            case CustomInspectorValueType_Bool:
            case CustomInspectorValueType_Texture:
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
            case CustomInspectorValueType_Int:
                hasChanged       = ImGui::ColorEdit4(widget.m_Label.c_str(), value.m_VectorValue);
                value.m_IntValue = ImGui::ColorConvertFloat4ToU32(ImVec4(value.m_VectorValue[0], value.m_VectorValue[1], value.m_VectorValue[2], value.m_VectorValue[3]));
                break;
            case CustomInspectorValueType_Vector3:
                hasChanged = ImGui::ColorEdit3(widget.m_Label.c_str(), value.m_VectorValue);
                break;
            case CustomInspectorValueType_Vector4:
                hasChanged = ImGui::ColorEdit4(widget.m_Label.c_str(), value.m_VectorValue);
                break;
            case CustomInspectorValueType_String:
                hasChanged          = ImGui::ColorEdit4(widget.m_Label.c_str(), value.m_VectorValue);
                value.m_StringValue = ColorConvertToHexString(value.m_VectorValue[0], value.m_VectorValue[1], value.m_VectorValue[2], value.m_VectorValue[3]);
                break;
            case CustomInspectorValueType_Float:
            case CustomInspectorValueType_Bool:
            case CustomInspectorValueType_Vector2:
            case CustomInspectorValueType_Texture:
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
            case CustomInspectorValueType_Texture:
                break;
            case CustomInspectorValueType_Int:
            case CustomInspectorValueType_Float:
            case CustomInspectorValueType_Vector2:
            case CustomInspectorValueType_Vector3:
            case CustomInspectorValueType_Vector4:
            case CustomInspectorValueType_Bool:
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
            case CustomInspectorValueType_Int:
                break;
            case CustomInspectorValueType_Float:
            case CustomInspectorValueType_Vector2:
            case CustomInspectorValueType_Vector3:
            case CustomInspectorValueType_Vector4:
            case CustomInspectorValueType_String:
            case CustomInspectorValueType_Bool:
            case CustomInspectorValueType_Texture:
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
        if (value.GetType() != CustomInspectorValueType_Path)
            throw std::runtime_error("Invalid data type for Path");
        return utils::DrawPathEditor<CustomInspectorMaxPathPoints>(
            widget.m_Label.c_str(), value.m_PathPoints, value.m_PathPointCount);
    }

    bool CustomInspector::RenderCurve(const CustomInspectorWidget &widget)
    {
        auto &value = m_Values[widget.m_VariableName];
        if (value.GetType() != CustomInspectorValueType_Curve)
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
            case CustomInspectorValueType_Int:
                value.m_IntValue = (value.m_BoolValue ? 1 : 0);
                ;
                break;
            case CustomInspectorValueType_Float:
                value.m_FloatValue = (value.m_BoolValue ? 1.0f : 0.0f);
                break;
            case CustomInspectorValueType_Vector2:
            case CustomInspectorValueType_Vector3:
            case CustomInspectorValueType_Vector4:
                value.m_VectorValue[0] = (value.m_BoolValue ? 1.0f : 0.0f);
                value.m_VectorValue[1] = (value.m_BoolValue ? 1.0f : 0.0f);
                value.m_VectorValue[2] = (value.m_BoolValue ? 1.0f : 0.0f);
                value.m_VectorValue[3] = (value.m_BoolValue ? 1.0f : 0.0f);
                break;
            case CustomInspectorValueType_String:
                value.m_StringValue = (value.m_BoolValue ? "true" : "false");
                break;
            case CustomInspectorValueType_Bool:
                break;
            case CustomInspectorValueType_Texture:
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
            case CustomInspectorValueType_Int:
                hasChanged = ImGui::InputInt(widget.m_Label.c_str(), &value.m_IntValue, widget.m_ISpeed, widget.m_ISpeed * 10);
                break;
            case CustomInspectorValueType_Float:
                hasChanged = ImGui::InputFloat(widget.m_Label.c_str(), &value.m_FloatValue, widget.m_FSpeed, widget.m_FSpeed * 10.0f);
                break;
            case CustomInspectorValueType_Vector2:
                hasChanged = ImGui::InputFloat2(widget.m_Label.c_str(), value.m_VectorValue);
                break;
            case CustomInspectorValueType_Vector3:
                hasChanged = ImGui::InputFloat3(widget.m_Label.c_str(), value.m_VectorValue);
                break;
            case CustomInspectorValueType_Vector4:
                hasChanged = ImGui::InputFloat4(widget.m_Label.c_str(), value.m_VectorValue);
                break;
            case CustomInspectorValueType_String:
                static char s_Buffer[4096];
                std::strcpy(s_Buffer, value.m_StringValue.c_str());
                hasChanged          = ImGui::InputText(widget.m_Label.c_str(), s_Buffer, sizeof(s_Buffer));
                value.m_StringValue = s_Buffer;
                break;
            case CustomInspectorValueType_Bool:
            case CustomInspectorValueType_Texture:
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
            case CustomInspectorValueType_Int:
                hasChanged = ShowSeedSettings(widget.m_Label, &value.m_IntValue, widget.m_SeedHistory);
                break;
            case CustomInspectorValueType_Float:
                hasChanged         = ShowSeedSettings(widget.m_Label, &value.m_IntValue, widget.m_SeedHistory);
                value.m_FloatValue = static_cast<float>(value.m_IntValue);
                break;
            case CustomInspectorValueType_String:
                if (ImGui::Button(("Seed Value: " + value.m_StringValue + " [Click to change]").c_str())) {
                    value.m_StringValue = GenerateId(8);
                    hasChanged          = true;
                }
                break;
            case CustomInspectorValueType_Vector2:
            case CustomInspectorValueType_Vector3:
            case CustomInspectorValueType_Vector4:
            case CustomInspectorValueType_Bool:
            case CustomInspectorValueType_Texture: // todo: add seed texture here too
                throw std::runtime_error(std::string("Invalid data type for Seed"));
            default:
                throw std::runtime_error(std::string("Invalid data type for Seed"));
        }
        return hasChanged;
    }

} // namespace tf3d::misc
