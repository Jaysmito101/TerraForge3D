#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Inspector/CustomInspectorTypes.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tf3d::inspector
{

    class CustomInspectorValue
    {
    public:
        CustomInspectorValue(CustomInspectorValueType type = CustomInspectorValueType::Unknown)
            : m_Type(type)
        {
        }
        ~CustomInspectorValue() = default;

        inline CustomInspectorValueType GetType() const
        {
            return m_Type;
        }
        inline std::string GetTypeString() const
        {
            return CustomInspectorValueTypeToString(m_Type);
        }
        inline std::string GetName() const
        {
            return m_Name;
        }
        inline std::string GetSerializedName() const
        {
            return m_SerializedName.empty() ? m_Name : m_SerializedName;
        }
        inline void SetShaderUniformName(const std::string &uniformName)
        {
            m_ShaderUniformName       = uniformName;
            m_ShaderUniformConfigured = true;
        }
        inline bool IsShaderUniformConfigured() const
        {
            return m_ShaderUniformConfigured;
        }
        inline const std::string &GetShaderUniformName() const
        {
            return m_ShaderUniformName;
        }

    private:
        inline int32_t GetInt() const
        {
            switch (m_Type) {
                case CustomInspectorValueType::Int:
                    return m_IntValue;
                case CustomInspectorValueType::Float:
                    return static_cast<int32_t>(m_FloatValue);
                case CustomInspectorValueType::Bool:
                    return m_BoolValue ? 1 : 0;
                case CustomInspectorValueType::String:
                    return std::atoi(m_StringValue.c_str());
                case CustomInspectorValueType::Vector2:
                case CustomInspectorValueType::Vector3:
                case CustomInspectorValueType::Vector4:
                    return static_cast<int32_t>(m_VectorValue[0]);
                case CustomInspectorValueType::Unknown:
                case CustomInspectorValueType::Texture:
                default:
                    return 0;
            }
            return 0;
        }

        inline float GetFloat() const
        {
            switch (m_Type) {
                case CustomInspectorValueType::Int:
                    return static_cast<float>(m_IntValue);
                case CustomInspectorValueType::Float:
                    return (m_FloatValue);
                case CustomInspectorValueType::Bool:
                    return m_BoolValue ? 1.0f : 0.0f;
                case CustomInspectorValueType::String:
                    return static_cast<float>(std::atof(m_StringValue.c_str()));
                case CustomInspectorValueType::Vector2:
                case CustomInspectorValueType::Vector3:
                case CustomInspectorValueType::Vector4:
                    return (m_VectorValue[0]);
                case CustomInspectorValueType::Unknown:
                case CustomInspectorValueType::Texture:
                default:
                    return 0.0f;
            }
            return 0.0f;
        }

        inline bool GetBool() const
        {
            switch (m_Type) {
                case CustomInspectorValueType::Int:
                    return static_cast<bool>(m_IntValue);
                case CustomInspectorValueType::Float:
                    return static_cast<bool>(m_FloatValue);
                case CustomInspectorValueType::Bool:
                    return m_BoolValue;
                case CustomInspectorValueType::String:
                    return m_StringValue == "true";
                case CustomInspectorValueType::Vector2:
                case CustomInspectorValueType::Vector3:
                case CustomInspectorValueType::Vector4:
                    return static_cast<bool>(m_VectorValue[0]);
                case CustomInspectorValueType::Unknown:
                case CustomInspectorValueType::Texture:
                default:
                    return false;
            }
            return false;
        }

        inline std::string GetString() const
        {
            switch (m_Type) {
                case CustomInspectorValueType::Int:
                    return std::to_string(m_IntValue);
                case CustomInspectorValueType::Float:
                    return std::to_string(m_FloatValue);
                case CustomInspectorValueType::Bool:
                    return m_BoolValue ? "true" : "false";
                case CustomInspectorValueType::String:
                    return m_StringValue;
                case CustomInspectorValueType::Vector2:
                    return std::to_string(m_VectorValue[0]) + " " + std::to_string(m_VectorValue[1]);
                case CustomInspectorValueType::Vector3:
                    return std::to_string(m_VectorValue[0]) + " " + std::to_string(m_VectorValue[1]) + " " + std::to_string(m_VectorValue[2]);
                case CustomInspectorValueType::Vector4:
                    return std::to_string(m_VectorValue[0]) + " " + std::to_string(m_VectorValue[1]) + " " + std::to_string(m_VectorValue[2]) + " " + std::to_string(m_VectorValue[3]);
                case CustomInspectorValueType::Unknown:
                case CustomInspectorValueType::Texture:
                default:
                    return "";
            }
            return "";
        }

        inline glm::vec2 GetVector2() const
        {
            switch (m_Type) {
                case CustomInspectorValueType::Int:
                    return glm::vec2(static_cast<float>(m_IntValue));
                case CustomInspectorValueType::Float:
                    return glm::vec2(static_cast<float>(m_FloatValue));
                case CustomInspectorValueType::Bool:
                    return glm::vec2(m_BoolValue ? 1.0f : 0.0f);
                case CustomInspectorValueType::Vector2:
                case CustomInspectorValueType::Vector3:
                case CustomInspectorValueType::Vector4:
                    return glm::vec2(m_VectorValue[0], m_VectorValue[1]);
                case CustomInspectorValueType::Unknown:
                case CustomInspectorValueType::String:
                case CustomInspectorValueType::Texture:
                default:
                    return glm::vec2(0.0f);
            }
            return glm::vec2(0.0f);
        }

        inline glm::vec3 GetVector3() const
        {
            switch (m_Type) {
                case CustomInspectorValueType::Int:
                    return glm::vec3(static_cast<float>(m_IntValue));
                case CustomInspectorValueType::Float:
                    return glm::vec3(static_cast<float>(m_FloatValue));
                case CustomInspectorValueType::Bool:
                    return glm::vec3(m_BoolValue ? 1.0f : 0.0f);
                case CustomInspectorValueType::Vector2:
                    return glm::vec3(m_VectorValue[0], m_VectorValue[1], 0.0f);
                case CustomInspectorValueType::Vector3:
                case CustomInspectorValueType::Vector4:
                    return glm::vec3(m_VectorValue[0], m_VectorValue[1], m_VectorValue[2]);
                case CustomInspectorValueType::Unknown:
                case CustomInspectorValueType::Texture:
                case CustomInspectorValueType::String:
                default:
                    return glm::vec3(0.0f);
            }
            return glm::vec3(0.0f);
        }

        inline glm::vec4 GetVector4() const
        {
            switch (m_Type) {
                case CustomInspectorValueType::Int:
                    return glm::vec4(static_cast<float>(m_IntValue));
                case CustomInspectorValueType::Float:
                    return glm::vec4(static_cast<float>(m_FloatValue));
                case CustomInspectorValueType::Bool:
                    return glm::vec4(m_BoolValue ? 1.0f : 0.0f);
                case CustomInspectorValueType::Vector2:
                    return glm::vec4(m_VectorValue[0], m_VectorValue[1], 0.0f, 0.0f);
                case CustomInspectorValueType::Vector3:
                    return glm::vec4(m_VectorValue[0], m_VectorValue[1], m_VectorValue[2], 0.0f);
                case CustomInspectorValueType::Vector4:
                    return glm::vec4(m_VectorValue[0], m_VectorValue[1], m_VectorValue[2], m_VectorValue[3]);
                case CustomInspectorValueType::Unknown:
                case CustomInspectorValueType::Texture:
                case CustomInspectorValueType::String:
                default:
                    return glm::vec4(0.0f);
            }
            return glm::vec4(0.0f);
        }

        inline std::shared_ptr<Texture2D> GetTexture() const
        {
            switch (m_Type) {
                case CustomInspectorValueType::Texture:
                    return m_TextureValue;
                case CustomInspectorValueType::Int:
                case CustomInspectorValueType::Float:
                case CustomInspectorValueType::Bool:
                case CustomInspectorValueType::String:
                case CustomInspectorValueType::Vector2:
                case CustomInspectorValueType::Vector3:
                case CustomInspectorValueType::Vector4:
                case CustomInspectorValueType::Unknown:
                default:
                    return nullptr;
            }
            return nullptr;
        }

        inline const std::array<glm::vec2, CustomInspectorMaxPathPoints> &GetPathPoints() const
        {
            return m_PathPoints;
        }
        inline int GetPathPointCount() const
        {
            return m_PathPointCount;
        }
        inline const std::array<glm::vec2, CustomInspectorMaxCurvePoints> &GetCurvePoints() const
        {
            return m_CurvePoints;
        }
        inline int GetCurvePointCount() const
        {
            return m_CurvePointCount;
        }

    public:
        template <typename T>
        static constexpr CustomInspectorValueType TypeFor()
        {
            using ValueType = std::decay_t<T>;
            if constexpr (std::is_same_v<ValueType, bool>)
                return CustomInspectorValueType::Bool;
            else if constexpr (std::is_integral_v<ValueType>)
                return CustomInspectorValueType::Int;
            else if constexpr (std::is_floating_point_v<ValueType>)
                return CustomInspectorValueType::Float;
            else if constexpr (std::is_same_v<ValueType, std::string>)
                return CustomInspectorValueType::String;
            else if constexpr (std::is_same_v<ValueType, glm::vec2>)
                return CustomInspectorValueType::Vector2;
            else if constexpr (std::is_same_v<ValueType, glm::vec3>)
                return CustomInspectorValueType::Vector3;
            else if constexpr (std::is_same_v<ValueType, glm::vec4>)
                return CustomInspectorValueType::Vector4;
            else if constexpr (std::is_same_v<ValueType, std::vector<float>>)
                return CustomInspectorValueType::FloatArray;
            else if constexpr (std::is_same_v<ValueType, std::shared_ptr<Texture2D>>)
                return CustomInspectorValueType::Texture;
            else
                return CustomInspectorValueType::Unknown;
        }

        template <typename T>
        T Get(T fallback = {}) const
        {
            using ValueType = std::decay_t<T>;
            if constexpr (std::is_same_v<ValueType, bool>) {
                return m_Type == CustomInspectorValueType::Bool ? GetBool() : fallback;
            } else if constexpr (std::is_integral_v<ValueType>) {
                return m_Type == CustomInspectorValueType::Int ? static_cast<T>(GetInt()) : fallback;
            } else if constexpr (std::is_floating_point_v<ValueType>) {
                return m_Type == CustomInspectorValueType::Float ? static_cast<T>(GetFloat()) : fallback;
            } else if constexpr (std::is_same_v<ValueType, std::string>) {
                return m_Type == CustomInspectorValueType::String ? GetString() : fallback;
            } else if constexpr (std::is_same_v<ValueType, glm::vec2>) {
                return m_Type == CustomInspectorValueType::Vector2 ? GetVector2() : fallback;
            } else if constexpr (std::is_same_v<ValueType, glm::vec3>) {
                return m_Type == CustomInspectorValueType::Vector3 ? GetVector3() : fallback;
            } else if constexpr (std::is_same_v<ValueType, glm::vec4>) {
                return m_Type == CustomInspectorValueType::Vector4 ? GetVector4() : fallback;
            } else if constexpr (std::is_same_v<ValueType, std::vector<float>>) {
                return m_Type == CustomInspectorValueType::FloatArray ? m_FloatArrayValue : fallback;
            } else if constexpr (std::is_same_v<ValueType, std::shared_ptr<Texture2D>>) {
                return m_Type == CustomInspectorValueType::Texture ? GetTexture() : fallback;
            } else if constexpr (std::is_same_v<ValueType, std::vector<glm::vec2>>) {
                std::vector<glm::vec2> points;
                if (m_Type == CustomInspectorValueType::Path) {
                    points.reserve(static_cast<size_t>(m_PathPointCount));
                    for (int index = 0; index < m_PathPointCount; ++index)
                        points.push_back(m_PathPoints[index]);
                } else if (m_Type == CustomInspectorValueType::Curve) {
                    points.reserve(static_cast<size_t>(m_CurvePointCount));
                    for (int index = 0; index < m_CurvePointCount; ++index)
                        points.push_back(m_CurvePoints[index]);
                } else {
                    return fallback;
                }
                return points;
            } else {
                return fallback;
            }
        }

        template <typename T>
        bool Set(T value)
        {
            using ValueType = std::decay_t<T>;
            if constexpr (std::is_same_v<ValueType, bool>) {
                if (m_Type != CustomInspectorValueType::Bool)
                    return false;
                m_BoolValue = value;
            } else if constexpr (std::is_integral_v<ValueType>) {
                if (m_Type != CustomInspectorValueType::Int)
                    return false;
                m_IntValue = static_cast<int32_t>(value);
            } else if constexpr (std::is_floating_point_v<ValueType>) {
                if (m_Type != CustomInspectorValueType::Float)
                    return false;
                m_FloatValue = static_cast<float>(value);
            } else if constexpr (std::is_same_v<ValueType, std::string>) {
                if (m_Type != CustomInspectorValueType::String)
                    return false;
                m_StringValue = std::move(value);
            } else if constexpr (std::is_same_v<ValueType, glm::vec2>) {
                if (m_Type != CustomInspectorValueType::Vector2)
                    return false;
                m_VectorValue[0] = value.x;
                m_VectorValue[1] = value.y;
            } else if constexpr (std::is_same_v<ValueType, glm::vec3>) {
                if (m_Type != CustomInspectorValueType::Vector3)
                    return false;
                m_VectorValue[0] = value.x;
                m_VectorValue[1] = value.y;
                m_VectorValue[2] = value.z;
            } else if constexpr (std::is_same_v<ValueType, glm::vec4>) {
                if (m_Type != CustomInspectorValueType::Vector4)
                    return false;
                m_VectorValue[0] = value.x;
                m_VectorValue[1] = value.y;
                m_VectorValue[2] = value.z;
                m_VectorValue[3] = value.w;
            } else if constexpr (std::is_same_v<ValueType, std::vector<float>>) {
                if (m_Type != CustomInspectorValueType::FloatArray || value.empty())
                    return false;
                m_FloatArrayValue = std::move(value);
            } else if constexpr (std::is_same_v<ValueType, std::shared_ptr<Texture2D>>) {
                if (m_Type != CustomInspectorValueType::Texture)
                    return false;
                m_TextureValue = std::move(value);
            } else if constexpr (std::is_same_v<ValueType, std::vector<glm::vec2>>) {
                if (value.empty())
                    return false;
                if (m_Type == CustomInspectorValueType::Path) {
                    m_PathPointCount = std::clamp(static_cast<int32_t>(value.size()), 1, static_cast<int32_t>(CustomInspectorMaxPathPoints));
                    for (int index = 0; index < m_PathPointCount; ++index)
                        m_PathPoints[index] = value[static_cast<size_t>(index)];
                } else if (m_Type == CustomInspectorValueType::Curve) {
                    m_CurvePointCount = std::clamp(static_cast<int32_t>(value.size()), 2, static_cast<int32_t>(CustomInspectorMaxCurvePoints));
                    for (int index = 0; index < m_CurvePointCount; ++index)
                        m_CurvePoints[index] = value[static_cast<size_t>(index)];
                } else {
                    return false;
                }
            } else {
                return false;
            }
            return true;
        }

        template <typename T>
        bool SetDefault(T value)
        {
            if (!Set(std::move(value)))
                return false;
            m_DefaultIntValue        = m_IntValue;
            m_DefaultFloatValue      = m_FloatValue;
            m_DefaultBoolValue       = m_BoolValue;
            m_DefaultStringValue     = m_StringValue;
            m_DefaultTextureValue    = m_TextureValue;
            m_DefaultVectorValue[0]  = m_VectorValue[0];
            m_DefaultVectorValue[1]  = m_VectorValue[1];
            m_DefaultVectorValue[2]  = m_VectorValue[2];
            m_DefaultVectorValue[3]  = m_VectorValue[3];
            m_DefaultFloatArrayValue = m_FloatArrayValue;
            m_DefaultPathPoints      = m_PathPoints;
            m_DefaultPathPointCount  = m_PathPointCount;
            m_DefaultCurvePoints     = m_CurvePoints;
            m_DefaultCurvePointCount = m_CurvePointCount;
            return true;
        }

        inline void ResetValue()
        {
            m_IntValue        = m_DefaultIntValue;
            m_FloatValue      = m_DefaultFloatValue;
            m_BoolValue       = m_DefaultBoolValue;
            m_StringValue     = m_DefaultStringValue;
            m_TextureValue    = m_DefaultTextureValue;
            m_VectorValue[0]  = m_DefaultVectorValue[0];
            m_VectorValue[1]  = m_DefaultVectorValue[1];
            m_VectorValue[2]  = m_DefaultVectorValue[2];
            m_VectorValue[3]  = m_DefaultVectorValue[3];
            m_FloatArrayValue = m_DefaultFloatArrayValue;
            m_PathPoints      = m_DefaultPathPoints;
            m_PathPointCount  = m_DefaultPathPointCount;
            m_CurvePoints     = m_DefaultCurvePoints;
            m_CurvePointCount = m_DefaultCurvePointCount;
        }

        static std::string CustomInspectorValueTypeToString(CustomInspectorValueType type);
        static CustomInspectorValueType CustomInspectorValueTypeFromString(const std::string &type);

    private:
        bool WriteStateValue(SerializerNode target, const std::string &name) const;
        bool ReadStateValue(SerializerNode source, const std::string &name);

        friend class CustomInspector;

    private:
        std::string m_Name              = "";
        std::string m_SerializedName    = "";
        std::string m_ShaderUniformName = "";
        CustomInspectorValueType m_Type = CustomInspectorValueType::Unknown;
        bool m_ShaderUniformConfigured  = false;
        int32_t m_IntValue = 0, m_DefaultIntValue = 0;
        float m_FloatValue = 0.0f, m_DefaultFloatValue = 0.0f;
        bool m_BoolValue = false, m_DefaultBoolValue = false;
        std::string m_StringValue = "", m_DefaultStringValue = "";
        std::shared_ptr<Texture2D> m_TextureValue = nullptr, m_DefaultTextureValue = nullptr;
        bool m_TextureLoadAs16Bit = false;
        float m_VectorValue[4] = {0.0f, 0.0f, 0.0f, 0.0f}, m_DefaultVectorValue[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        std::vector<float> m_FloatArrayValue, m_DefaultFloatArrayValue;
        std::array<glm::vec2, CustomInspectorMaxPathPoints> m_PathPoints{};
        std::array<glm::vec2, CustomInspectorMaxPathPoints> m_DefaultPathPoints{};
        int32_t m_PathPointCount = 2, m_DefaultPathPointCount = 2;
        std::array<glm::vec2, CustomInspectorMaxCurvePoints> m_CurvePoints{};
        std::array<glm::vec2, CustomInspectorMaxCurvePoints> m_DefaultCurvePoints{};
        int32_t m_CurvePointCount = 2, m_DefaultCurvePointCount = 2;
    };

} // namespace tf3d::inspector
