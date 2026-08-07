#include "Misc/CustomInspector.h"
#include "Base/Base.h"
#include "Base/Shader.h"
#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "UI/ImGuiComponents.h"
#include "Utils/PathEditor.h"
#include "Utils/Utils.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

namespace tf3d::misc
{

    namespace
    {
        bool ReadPresetFloat(const nlohmann::json &source, float &destination)
        {
            if (!source.is_number())
                return false;
            const double parsed = source.get<double>();
            if (!std::isfinite(parsed) ||
                parsed < -static_cast<double>(std::numeric_limits<float>::max()) ||
                parsed > static_cast<double>(std::numeric_limits<float>::max()))
                return false;
            destination = static_cast<float>(parsed);
            return std::isfinite(destination);
        }

        bool ReadPresetInteger(const nlohmann::json &source, int32_t &destination)
        {
            int64_t parsed = 0;
            if (source.is_number_integer()) {
                parsed = source.get<int64_t>();
            } else if (source.is_number_unsigned()) {
                const uint64_t unsignedValue = source.get<uint64_t>();
                if (unsignedValue > static_cast<uint64_t>(std::numeric_limits<int32_t>::max()))
                    return false;
                parsed = static_cast<int64_t>(unsignedValue);
            } else {
                return false;
            }
            if (parsed < static_cast<int64_t>(std::numeric_limits<int32_t>::min()) ||
                parsed > static_cast<int64_t>(std::numeric_limits<int32_t>::max()))
                return false;
            destination = static_cast<int32_t>(parsed);
            return true;
        }

        bool ReadPresetVector(const nlohmann::json &source, int dimensions, float *destination)
        {
            static constexpr const char *componentNames[] = {"X", "Y", "Z", "W"};
            if (source.is_array()) {
                if (source.size() != static_cast<size_t>(dimensions))
                    return false;
                for (int index = 0; index < dimensions; ++index) {
                    if (!ReadPresetFloat(source[index], destination[index]))
                        return false;
                }
                return true;
            }
            if (!source.is_object() || source.size() != static_cast<size_t>(dimensions))
                return false;
            for (int index = 0; index < dimensions; ++index) {
                if (!source.contains(componentNames[index]) ||
                    !ReadPresetFloat(source[componentNames[index]], destination[index]))
                    return false;
            }
            return true;
        }
    } // namespace

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
        if (!m_SerializedName.empty() && m_SerializedName != m_Name)
            node->Set("SerializedName", m_SerializedName);
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
        m_Type           = CustomInspectorValueTypeFromString(node->Get<std::string>("TypeName", CustomInspectorValueTypeToString(m_Type)));
        m_Name           = node->Get<std::string>("Name");
        m_SerializedName = node->Get<std::string>("SerializedName", m_Name);
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
            case CustomInspectorWidgetType::Slider:
                return "Slider";
            case CustomInspectorWidgetType::Drag:
                return "Drag";
            case CustomInspectorWidgetType::Color:
                return "Color";
            case CustomInspectorWidgetType::Texture:
                return "Texture";
            case CustomInspectorWidgetType::Path:
                return "Path";
            case CustomInspectorWidgetType::Curve:
                return "Curve";
            case CustomInspectorWidgetType::Button:
                return "Button";
            case CustomInspectorWidgetType::Checkbox:
                return "Checkbox";
            case CustomInspectorWidgetType::Input:
                return "Input";
            case CustomInspectorWidgetType::Seed:
                return "Seed";
            case CustomInspectorWidgetType::Dropdown:
                return "Dropdown";
            case CustomInspectorWidgetType::Separator:
                return "Separator";
            case CustomInspectorWidgetType::NewLine:
                return "NewLine";
            case CustomInspectorWidgetType::Text:
                return "Text";
            case CustomInspectorWidgetType::Unknown:
            default:
                return "Unknown";
        }
    }

    CustomInspectorWidgetType CustomInspectorWidget::CustomInspectorWidgetTypeFromString(const std::string &type)
    {
        if (type == "Slider")
            return CustomInspectorWidgetType::Slider;
        if (type == "Drag")
            return CustomInspectorWidgetType::Drag;
        if (type == "Color")
            return CustomInspectorWidgetType::Color;
        if (type == "Texture")
            return CustomInspectorWidgetType::Texture;
        if (type == "Path")
            return CustomInspectorWidgetType::Path;
        if (type == "Curve")
            return CustomInspectorWidgetType::Curve;
        if (type == "Button")
            return CustomInspectorWidgetType::Button;
        if (type == "Checkbox")
            return CustomInspectorWidgetType::Checkbox;
        if (type == "Input")
            return CustomInspectorWidgetType::Input;
        if (type == "Seed")
            return CustomInspectorWidgetType::Seed;
        if (type == "Dropdown")
            return CustomInspectorWidgetType::Dropdown;
        if (type == "Separator" || type == "Seperator")
            return CustomInspectorWidgetType::Separator;
        if (type == "NewLine")
            return CustomInspectorWidgetType::NewLine;
        if (type == "Text")
            return CustomInspectorWidgetType::Text;
        return CustomInspectorWidgetType::Unknown;
    }

    SerializerNode CustomInspectorWidget::Save() const
    {
        SerializerNode node = CreateSerializerNode();
        node->Set("Type", static_cast<int32_t>(m_Type));
        node->Set("TypeName", CustomInspectorWidgetTypeToString(m_Type));
        node->Set("TargetVariable", m_VariableName);
        node->Set("Label", m_Label);
        node->Set("ShaderUniformConfigured", m_ShaderUniformConfigured);
        if (m_ShaderUniformConfigured)
            node->Set("ShaderUniformName", m_ShaderUniformName);
        if (m_Type == CustomInspectorWidgetType::Seed)
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
        if (!m_DropdownOptions.empty())
            node->Set("DropdownOptions", m_DropdownOptions);
        if (!m_DropdownValues.empty())
            node->Set("DropdownValues", m_DropdownValues);
        return node;
    }

    void CustomInspectorWidget::Load(SerializerNode node)
    {
        m_Type                    = CustomInspectorWidgetTypeFromString(node->Get<std::string>("TypeName", CustomInspectorWidgetTypeToString(m_Type)));
        m_VariableName            = node->Get<std::string>("TargetVariable", m_VariableName);
        m_Label                   = node->Get<std::string>("Label", m_Label);
        m_ShaderUniformConfigured = node->Get<bool>("ShaderUniformConfigured", false);
        m_ShaderUniformName       = node->Get<std::string>("ShaderUniformName", m_ShaderUniformName);
        m_ID                      = node->Get<std::string>("ID", m_ID);
        if (m_Type == CustomInspectorWidgetType::Seed)
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
        m_Tooltip         = node->Get<std::string>("Tooltip", m_Tooltip);
        m_FontName        = node->Get<std::string>("FontName", m_FontName);
        m_DropdownOptions = node->Get<std::vector<std::string>>("DropdownOptions", m_DropdownOptions);
        m_DropdownValues  = node->Get<std::vector<int32_t>>("DropdownValues", m_DropdownValues);
        if (!m_DropdownValues.empty() && m_DropdownValues.size() != m_DropdownOptions.size())
            m_DropdownValues.clear();
    }

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
        // Ideally this should not be called
        m_Values.erase(name);
        // Suggestion:
        // Also delete all widgets referencing this variable
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

    bool CustomInspector::SetPresetValue(std::unordered_map<std::string, CustomInspectorValue> &values,
                                         const std::string &name,
                                         const nlohmann::json &value,
                                         std::string_view presetName) const
    {
        const auto invalid = [&](std::string_view reason) {
            TF3D_LOG_WARN("Invalid CustomInspector preset '{}' field '{}': {}",
                          std::string(presetName), name, std::string(reason));
            return false;
        };

        auto target = values.find(name);
        if (target == values.end()) {
            for (auto iterator = values.begin(); iterator != values.end(); ++iterator) {
                if (iterator->second.GetSerializedName() == name) {
                    target = iterator;
                    break;
                }
            }
        }
        if (target == values.end())
            return invalid("unknown inspector value");

        CustomInspectorValue candidate = target->second;
        bool converted                 = false;
        switch (candidate.GetType()) {
            case CustomInspectorValueType::Int: {
                int32_t parsed = 0;
                converted      = ReadPresetInteger(value, parsed) && candidate.Set(parsed);
                break;
            }
            case CustomInspectorValueType::Float: {
                float parsed = 0.0f;
                converted    = ReadPresetFloat(value, parsed) && candidate.Set(parsed);
                break;
            }
            case CustomInspectorValueType::Bool:
                converted = value.is_boolean() && candidate.Set(value.get<bool>());
                break;
            case CustomInspectorValueType::String:
                converted = value.is_string() && candidate.Set(value.get<std::string>());
                break;
            case CustomInspectorValueType::Vector2: {
                float components[4] = {};
                converted           = ReadPresetVector(value, 2, components) &&
                            candidate.Set(glm::vec2(components[0], components[1]));
                break;
            }
            case CustomInspectorValueType::Vector3: {
                float components[4] = {};
                converted           = ReadPresetVector(value, 3, components) &&
                            candidate.Set(glm::vec3(components[0], components[1], components[2]));
                break;
            }
            case CustomInspectorValueType::Vector4: {
                float components[4] = {};
                converted           = ReadPresetVector(value, 4, components) &&
                            candidate.Set(glm::vec4(components[0], components[1], components[2], components[3]));
                break;
            }
            case CustomInspectorValueType::Texture: {
                if (!value.is_string())
                    break;
                const std::string path = value.get<std::string>();
                if (path.empty() || path == "null") {
                    converted = candidate.Set(std::shared_ptr<Texture2D>{});
                    break;
                }
                auto texture = std::make_shared<Texture2D>(path, true, false, candidate.m_TextureLoadAs16Bit);
                if (!texture->IsLoaded())
                    return invalid("texture could not be loaded");
                converted = candidate.Set(std::move(texture));
                break;
            }
            case CustomInspectorValueType::Path:
            case CustomInspectorValueType::Curve: {
                if (!value.is_array())
                    break;
                const size_t minimumPoints = candidate.GetType() == CustomInspectorValueType::Curve ? 2u : 1u;
                if (value.size() < minimumPoints || value.size() > CustomInspectorMaxPathPoints)
                    break;
                std::vector<glm::vec2> points;
                points.reserve(value.size());
                for (const auto &point : value) {
                    float components[4] = {};
                    if (!ReadPresetVector(point, 2, components)) {
                        points.clear();
                        break;
                    }
                    points.emplace_back(components[0], components[1]);
                }
                converted = !points.empty() && candidate.Set(std::move(points));
                break;
            }
            case CustomInspectorValueType::Unknown:
            case CustomInspectorValueType::Count:
            default:
                break;
        }

        if (!converted)
            return invalid("value does not match its inspector type");
        if (!ValidateValue(target->first, candidate))
            return false;
        target->second = std::move(candidate);
        return true;
    }

    bool CustomInspector::ApplyPresetValues(const nlohmann::json &values,
                                            std::string_view presetName,
                                            bool commit)
    {
        if (!values.is_object()) {
            TF3D_LOG_WARN("Invalid CustomInspector preset '{}': Values must be an object",
                          std::string(presetName));
            return false;
        }

        auto candidates = m_Values;
        bool valid      = true;
        for (const auto &[name, value] : values.items()) {
            if (!SetPresetValue(candidates, name, value, presetName))
                valid = false;
        }
        if (valid && commit)
            m_Values = std::move(candidates);
        return valid;
    }

    bool CustomInspector::RenderPresetSelector()
    {
        if (m_Presets.empty())
            return false;

        std::string preview     = "Custom";
        std::string description = "Choose a preset or reset all inspector values to their defaults.";
        if (m_SelectedPreset == 0) {
            preview = "Default";
        } else if (m_SelectedPreset > 0 && static_cast<size_t>(m_SelectedPreset - 1) < m_Presets.size()) {
            const auto &preset = m_Presets[static_cast<size_t>(m_SelectedPreset - 1)];
            preview            = preset.label;
            description        = preset.description;
        }

        bool changed = false;
        if (ImGui::BeginCombo("Preset", preview.c_str())) {
            if (ImGui::Selectable("Default", m_SelectedPreset == 0)) {
                Reset();
                changed = true;
            }
            if (m_SelectedPreset == 0)
                ImGui::SetItemDefaultFocus();

            for (size_t index = 0; index < m_Presets.size(); ++index) {
                const auto &preset   = m_Presets[index];
                const int32_t choice = static_cast<int32_t>(index + 1);
                const bool selected  = m_SelectedPreset == choice;
                if (ImGui::Selectable(preset.label.c_str(), selected)) {
                    if (ApplyPresetValues(preset.values, preset.name, true)) {
                        m_SelectedPreset      = choice;
                        m_LastChangedVariable = "Preset";
                        m_LastAction.clear();
                        changed = true;
                    }
                }
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        RenderInspectorTooltip("Preset", description);
        return changed;
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
        m_SelectedPreset = -1;
    }

    SerializerNode CustomInspector::SaveState() const
    {
        SerializerNode state = CreateSerializerNode();
        std::unordered_set<std::string> savedVariables;

        auto saveValue = [&](SerializerNode target,
                             const std::string &name,
                             const CustomInspectorValue &value) {
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

    bool CustomInspector::ValidateValue(const std::string &name, const CustomInspectorValue &value) const
    {
        if (value.GetType() != CustomInspectorValueType::Int &&
            value.GetType() != CustomInspectorValueType::Float)
            return true;

        const float numericValue = value.GetType() == CustomInspectorValueType::Int
                                       ? static_cast<float>(value.Get<int32_t>())
                                       : value.Get<float>();
        if (!std::isfinite(numericValue)) {
            TF3D_LOG_WARN("Invalid CustomInspector value '{}' must be finite", name);
            return false;
        }

        for (const auto &[widgetName, widget] : m_Widgets) {
            if (widget.m_VariableName != name ||
                (widget.m_Type != CustomInspectorWidgetType::Slider &&
                 widget.m_Type != CustomInspectorWidgetType::Drag))
                continue;

            const float minimum = widget.m_Constratins[0];
            const float maximum = widget.m_Constratins[1];
            if (minimum == 0.0f && maximum == 0.0f)
                continue;
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
                const auto value = m_Values.find(widget->second.m_VariableName);
                if (value != m_Values.end() && value->second.GetSerializedName() == serializedName)
                    return value->first;
            }

            if (sectionName.empty()) {
                for (const auto &[name, value] : m_Values) {
                    if (value.GetSerializedName() == serializedName)
                        return name;
                }
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
                if ((widget->m_Type == CustomInspectorWidgetType::Slider ||
                     widget->m_Type == CustomInspectorWidgetType::Drag) &&
                    (widget->m_Constratins[0] != 0.0f || widget->m_Constratins[1] != 0.0f)) {
                    schema["minimum"] = widget->m_Constratins[0];
                    schema["maximum"] = widget->m_Constratins[1];
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
        m_SelectedPreset = -1;
    }

    bool CustomInspector::LoadConfig(ApplicationState *appState, std::string_view inspectorName)
    {
        if (appState == nullptr || appState->resourceManager == nullptr) {
            TF3D_LOG_ERROR("Cannot load inspector metadata '{}' without a resource manager", inspectorName);
            return false;
        }
        if (inspectorName.empty()) {
            TF3D_LOG_ERROR("Cannot load inspector metadata with an empty name");
            return false;
        }

        const std::string configPath = appState->constants.dataDir + PATH_SEPARATOR + "inspectors" +
                                       PATH_SEPARATOR + std::string(inspectorName) + ".json";
        bool loaded              = false;
        const std::string source = appState->resourceManager->LoadText(configPath, false, &loaded);
        if (!loaded) {
            TF3D_LOG_ERROR("Could not load inspector metadata '{}'", configPath);
            return false;
        }

        const nlohmann::json config = nlohmann::json::parse(source, nullptr, false);
        if (config.is_discarded()) {
            TF3D_LOG_ERROR("Could not parse inspector metadata '{}'", configPath);
            return false;
        }
        if (!LoadConfig(config)) {
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

    void CustomInspector::ApplyToShader(tf3d::base::Shader &shader, std::string_view uniformPrefix) const
    {
        for (const auto &[name, value] : m_Values) {
            const CustomInspectorWidget *widget = nullptr;
            for (const auto &widgetLabel : m_WidgetsOrder) {
                const auto widgetIterator = m_Widgets.find(widgetLabel);
                if (widgetIterator == m_Widgets.end() || widgetIterator->second.m_VariableName != name)
                    continue;
                if (widget == nullptr || widgetIterator->second.m_ShaderUniformConfigured)
                    widget = &widgetIterator->second;
                if (widget->m_ShaderUniformConfigured)
                    break;
            }

            const std::string uniformName = widget != nullptr && widget->m_ShaderUniformConfigured
                                                ? widget->m_ShaderUniformName
                                                : std::string(uniformPrefix) + name;
            if (uniformName.empty())
                continue;

            switch (value.GetType()) {
                case CustomInspectorValueType::Int:
                    shader.SetUniform1i(uniformName, value.Get<int32_t>());
                    break;
                case CustomInspectorValueType::Float:
                    shader.SetUniform1f(uniformName, value.Get<float>());
                    break;
                case CustomInspectorValueType::Bool:
                    shader.SetUniform1i(uniformName, value.Get<bool>() ? 1 : 0);
                    break;
                case CustomInspectorValueType::Vector2:
                    shader.SetUniform2f(uniformName, value.Get<glm::vec2>());
                    break;
                case CustomInspectorValueType::Vector3:
                    shader.SetUniform3f(uniformName, value.Get<glm::vec3>());
                    break;
                case CustomInspectorValueType::Vector4: {
                    const glm::vec4 vector = value.Get<glm::vec4>();
                    shader.SetUniform4f(uniformName, vector.x, vector.y, vector.z, vector.w);
                    break;
                }
                case CustomInspectorValueType::String:
                case CustomInspectorValueType::Texture:
                case CustomInspectorValueType::Path:
                case CustomInspectorValueType::Curve:
                case CustomInspectorValueType::Unknown:
                case CustomInspectorValueType::Count:
                default:
                    break;
            }
        }
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
        if (widget.m_Type == CustomInspectorWidgetType::Slider)
            widgetChanged = RenderSlider(widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Drag)
            widgetChanged = RenderDrag(widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Color)
            widgetChanged = RenderColor(widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Texture)
            widgetChanged = RenderTexture(widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Path)
            widgetChanged = RenderPath(widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Curve)
            widgetChanged = RenderCurve(widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Button)
            widgetChanged = RenderButton(widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Checkbox)
            widgetChanged = RenderCheckbox(widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Input)
            widgetChanged = RenderInput(widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Seed)
            widgetChanged = RenderSeed(m_Widgets[widgetLabel]);
        else if (widget.m_Type == CustomInspectorWidgetType::Dropdown)
            widgetChanged = RenderDropdown(widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Separator)
            ImGui::Separator();
        else if (widget.m_Type == CustomInspectorWidgetType::NewLine)
            ImGui::NewLine();
        else if (widget.m_Type == CustomInspectorWidgetType::Text)
            ImGui::TextWrapped("%s", widget.m_Label.c_str());

        if (widgetChanged) {
            if (widget.m_Type == CustomInspectorWidgetType::Button)
                m_LastAction = widget.m_VariableName;
            else if (!widget.m_VariableName.empty())
                m_LastChangedVariable = widget.m_VariableName;
        }
        if (!widget.m_FontName.empty())
            ImGui::PopFont();
        RenderInspectorTooltip(widget.m_Label, widget.m_Tooltip);
        if (widget.m_Type != CustomInspectorWidgetType::Seed &&
            widget.m_Type != CustomInspectorWidgetType::Button &&
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

        if (!m_Presets.empty()) {
            hasChanged = RenderPresetSelector() || hasChanged;
            ImGui::Separator();
        }

        const auto renderWidget = [&](const std::string &widgetLabel) {
            const bool widgetChanged = RenderWidget(widgetLabel);
            if (widgetChanged && m_LastChangedVariable != "Preset")
                m_SelectedPreset = -1;
            hasChanged = widgetChanged || hasChanged;
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
            Reset();
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
        const bool hasMappedValues = widget.m_DropdownValues.size() == widget.m_DropdownOptions.size();
        int32_t selectedIndex      = 0;
        if (hasMappedValues) {
            const auto selected = std::find(widget.m_DropdownValues.begin(), widget.m_DropdownValues.end(), value.m_IntValue);
            if (selected != widget.m_DropdownValues.end()) {
                selectedIndex = static_cast<int32_t>(std::distance(widget.m_DropdownValues.begin(), selected));
            } else {
                value.m_IntValue = widget.m_DropdownValues.front();
            }
        } else {
            selectedIndex    = glm::clamp(value.m_IntValue, 0, static_cast<int32_t>(widget.m_DropdownOptions.size()) - 1);
            value.m_IntValue = selectedIndex;
        }

        if (ImGui::BeginCombo(widget.m_Label.c_str(), widget.m_DropdownOptions[selectedIndex].c_str())) {
            for (int i = 0; i < static_cast<int32_t>(widget.m_DropdownOptions.size()); i++) {
                const bool isSelected = (selectedIndex == i);
                if (ImGui::Selectable(widget.m_DropdownOptions[i].c_str(), isSelected)) {
                    value.m_IntValue = hasMappedValues ? widget.m_DropdownValues[i] : i;
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
