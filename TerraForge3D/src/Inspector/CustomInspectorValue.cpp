#include "Inspector/CustomInspectorValue.h"
#include "Base/Base.h"

namespace tf3d::inspector
{

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
            case CustomInspectorValueType::FloatArray:
                return "FloatArray";
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
        if (type == "FloatArray")
            return CustomInspectorValueType::FloatArray;
        if (type == "Texture")
            return CustomInspectorValueType::Texture;
        if (type == "Path")
            return CustomInspectorValueType::Path;
        if (type == "Curve")
            return CustomInspectorValueType::Curve;
        return CustomInspectorValueType::Unknown;
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
            case CustomInspectorValueType::FloatArray:
                target->Set(name, m_FloatArrayValue);
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
            case CustomInspectorValueType::FloatArray:
                return Set(source->Get(name, m_FloatArrayValue));
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

} // namespace tf3d::inspector
