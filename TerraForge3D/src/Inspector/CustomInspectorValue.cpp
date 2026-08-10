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
                target->Set(name, Store().Get<int32_t>());
                return true;
            case CustomInspectorValueType::Float:
                target->Set(name, Store().Get<float>());
                return true;
            case CustomInspectorValueType::Bool:
                target->Set(name, Store().Get<bool>());
                return true;
            case CustomInspectorValueType::String:
                target->Set(name, Store().Get<std::string>());
                return true;
            case CustomInspectorValueType::Vector2:
                target->Set(name, Store().Get<glm::vec2>());
                return true;
            case CustomInspectorValueType::Vector3:
                target->Set(name, Store().Get<glm::vec3>());
                return true;
            case CustomInspectorValueType::Vector4:
                target->Set(name, Store().Get<glm::vec4>());
                return true;
            case CustomInspectorValueType::FloatArray:
                target->Set(name, Store().Get<std::vector<float>>());
                return true;
            case CustomInspectorValueType::Texture: {
                const auto texture = Store().Get<std::shared_ptr<Texture2D>>();
                target->Set(name, texture ? texture->GetPath() : "");
                return true;
            }
            case CustomInspectorValueType::Path:
            case CustomInspectorValueType::Curve:
                target->Set(name, Store().Get<std::vector<glm::vec2>>());
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
                return Store().Set(source->Get(name, Store().Get<int32_t>()));
            case CustomInspectorValueType::Float:
                return Store().Set(source->Get(name, Store().Get<float>()));
            case CustomInspectorValueType::Bool:
                return Store().Set(source->Get(name, Store().Get<bool>()));
            case CustomInspectorValueType::String:
                return Store().Set(source->Get(name, Store().Get<std::string>()));
            case CustomInspectorValueType::Vector2:
                return Store().Set(source->Get(name, Store().Get<glm::vec2>()));
            case CustomInspectorValueType::Vector3:
                return Store().Set(source->Get(name, Store().Get<glm::vec3>()));
            case CustomInspectorValueType::Vector4:
                return Store().Set(source->Get(name, Store().Get<glm::vec4>()));
            case CustomInspectorValueType::FloatArray:
                return Store().Set(source->Get(name, Store().Get<std::vector<float>>()));
            case CustomInspectorValueType::Texture: {
                const auto texture     = Store().Get<std::shared_ptr<Texture2D>>();
                const std::string path = source->Get(name, texture ? texture->GetPath() : "");
                return Store().Set(path.empty() ? std::shared_ptr<Texture2D>{}
                                                : std::make_shared<Texture2D>(path, false, false, m_TextureLoadAs16Bit));
            }
            case CustomInspectorValueType::Path:
            case CustomInspectorValueType::Curve:
                return Store().Set(source->Get(name, Store().Get<std::vector<glm::vec2>>()));
            case CustomInspectorValueType::Unknown:
            default:
                return false;
        }
    }

} // namespace tf3d::inspector
