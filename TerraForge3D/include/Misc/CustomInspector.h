#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include <algorithm>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tf3d::misc
{

    enum class CustomInspectorValueType {
        Unknown = 0,
        Int,
        Float,
        Bool,
        String,
        Vector2,
        Vector3,
        Vector4,
        Texture,
        Path,
        Curve,
        Count
    };

    inline constexpr size_t CustomInspectorMaxPathPoints  = 16;
    inline constexpr size_t CustomInspectorMaxCurvePoints = 16;

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
            m_PathPoints      = m_DefaultPathPoints;
            m_PathPointCount  = m_DefaultPathPointCount;
            m_CurvePoints     = m_DefaultCurvePoints;
            m_CurvePointCount = m_DefaultCurvePointCount;
        }

        SerializerNode Save() const;
        void Load(const SerializerNode &node);

        static std::string CustomInspectorValueTypeToString(CustomInspectorValueType type);
        static CustomInspectorValueType CustomInspectorValueTypeFromString(const std::string &type);

    private:
        bool WriteStateValue(SerializerNode target, const std::string &name) const;
        bool ReadStateValue(SerializerNode source, const std::string &name);

        friend class CustomInspector;

    private:
        std::string m_Name              = "";
        std::string m_SerializedName    = "";
        CustomInspectorValueType m_Type = CustomInspectorValueType::Unknown;
        int32_t m_IntValue = 0, m_DefaultIntValue = 0;
        float m_FloatValue = 0.0f, m_DefaultFloatValue = 0.0f;
        bool m_BoolValue = false, m_DefaultBoolValue = false;
        std::string m_StringValue = "", m_DefaultStringValue = "";
        std::shared_ptr<Texture2D> m_TextureValue = nullptr, m_DefaultTextureValue = nullptr;
        bool m_TextureLoadAs16Bit = false;
        float m_VectorValue[4] = {0.0f, 0.0f, 0.0f, 0.0f}, m_DefaultVectorValue[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        std::array<glm::vec2, CustomInspectorMaxPathPoints> m_PathPoints{};
        std::array<glm::vec2, CustomInspectorMaxPathPoints> m_DefaultPathPoints{};
        int32_t m_PathPointCount = 2, m_DefaultPathPointCount = 2;
        std::array<glm::vec2, CustomInspectorMaxCurvePoints> m_CurvePoints{};
        std::array<glm::vec2, CustomInspectorMaxCurvePoints> m_DefaultCurvePoints{};
        int32_t m_CurvePointCount = 2, m_DefaultCurvePointCount = 2;
    };

    enum CustomInspectorWidgetType {
        CustomInspectorWidgetType_Unknown = 0,
        CustomInspectorWidgetType_Slider,
        CustomInspectorWidgetType_Drag,
        CustomInspectorWidgetType_Color,
        CustomInspectorWidgetType_Texture,
        CustomInspectorWidgetType_Path,
        CustomInspectorWidgetType_Curve,
        CustomInspectorWidgetType_Button,
        CustomInspectorWidgetType_Checkbox,
        CustomInspectorWidgetType_Input,
        CustomInspectorWidgetType_Seed,
        CustomInspectorWidgetType_Dropdown,
        CustomInspectorWidgetType_Seperator,
        CustomInspectorWidgetType_NewLine,
        CustomInspectorWidgetType_Text
    };

    class CustomInspectorWidget
    {
    public:
        CustomInspectorWidget(CustomInspectorWidgetType type = CustomInspectorWidgetType_Unknown);
        ~CustomInspectorWidget();

        inline CustomInspectorWidgetType GetType() const
        {
            return m_Type;
        }
        inline std::string GetTypeName() const
        {
            return CustomInspectorWidgetTypeToString(m_Type);
        }
        inline std::string GetLabel() const
        {
            return m_Label;
        }
        inline std::string GetVariableName() const
        {
            return m_VariableName;
        }
        inline void SetLabel(const std::string &label)
        {
            m_Label = label;
        }
        inline void SetVariableName(const std::string &variableName)
        {
            m_VariableName = variableName;
        }
        inline void SetActionName(const std::string &actionName)
        {
            m_VariableName = actionName;
        }
        inline void SetTooltip(const std::string &tooltip)
        {
            m_Tooltip = tooltip;
        }
        inline void SetConstraints(float a = 0.0f, float b = 0.0f, float c = 0.0f, float d = 0.0f)
        {
            m_Constratins[0] = a;
            m_Constratins[1] = b;
            m_Constratins[2] = c;
            m_Constratins[3] = d;
        }
        inline void SetFontName(const std::string &fontName)
        {
            m_FontName = fontName;
        }
        inline void SetDropdownOptions(const std::vector<std::string> &options,
                                       const std::vector<int32_t> &values = {})
        {
            m_DropdownOptions = options;
            m_DropdownValues  = values;
            if (!m_DropdownValues.empty() && m_DropdownValues.size() != m_DropdownOptions.size())
                m_DropdownValues.clear();
        }
        inline void SetSpeed(float speed)
        {
            m_FSpeed = speed;
            m_ISpeed = static_cast<int32_t>(speed);
        }
        inline void SetRenderOnCondition(const std::string &conditionName, int32_t conditionValue)
        {
            SetRenderOnConditions(conditionName, {conditionValue});
        }
        inline void SetRenderOnConditions(const std::string &conditionName, const std::vector<int32_t> &conditionValues)
        {
            m_UseRenderOnCondition    = true;
            m_RenderOnConditionName   = conditionName;
            m_RenderOnConditionValues = conditionValues;
            m_RenderOnConditionValue  = conditionValues.empty() ? 0 : conditionValues.front();
        }
        inline void ClearCondition()
        {
            m_UseRenderOnCondition   = false;
            m_RenderOnConditionName  = "";
            m_RenderOnConditionValue = 0;
            m_RenderOnConditionValues.clear();
        }

        SerializerNode Save() const;
        void Load(SerializerNode node);

        static std::string CustomInspectorWidgetTypeToString(CustomInspectorWidgetType type);
        static CustomInspectorWidgetType CustomInspectorWidgetTypeFromString(const std::string &type);

        friend class CustomInspector;

    private:
        std::string m_Label              = "";
        std::string m_VariableName       = "";
        std::string m_FontName           = "";
        CustomInspectorWidgetType m_Type = CustomInspectorWidgetType_Unknown;
        float m_Constratins[4]           = {0.0f, 0.0f, 0.0f, 0.0f};
        float m_FSpeed                   = 1.0f;
        int32_t m_ISpeed                 = 1;
        std::string m_Tooltip            = "";
        std::vector<int32_t> m_SeedHistory;
        std::vector<std::string> m_DropdownOptions;
        std::vector<int32_t> m_DropdownValues;
        std::string m_ID                    = "";
        bool m_UseRenderOnCondition         = false;
        std::string m_RenderOnConditionName = "";
        int32_t m_RenderOnConditionValue    = 0;
        std::vector<int32_t> m_RenderOnConditionValues;
    };

    struct CustomInspectorSection {
        std::string name;
        std::string label;
        std::string description;
        bool collapsible = false;
        bool defaultOpen = true;
    };

    class CustomInspector
    {
    public:
        CustomInspector();
        ~CustomInspector();

        bool Contains(const std::string &name) const;
        void Remove(const std::string &name);
        template <typename T>
        CustomInspectorValue &Add(const std::string &name, T defaultValue = {})
        {
            using ValueType = std::decay_t<T>;
            CustomInspectorValue value(CustomInspectorValue::TypeFor<ValueType>());
            value.m_Name = name;
            value.SetDefault(std::move(defaultValue));
            return AddVariable(name, value);
        }

        template <typename T>
        T Get(const std::string &name, T fallback = {}) const
        {
            const auto value = m_Values.find(name);
            return value == m_Values.end() ? fallback : value->second.Get(fallback);
        }

        template <typename T>
        bool Set(const std::string &name, T value)
        {
            const auto existing = m_Values.find(name);
            return existing != m_Values.end() && existing->second.Set(std::move(value));
        }

        template <typename Function>
        void ForEachValue(Function &&function) const
        {
            for (const auto &[name, value] : m_Values)
                function(name, value);
        }

        const CustomInspectorValue *FindValue(const std::string &name) const;

        bool HasWidget(const std::string &name);
        CustomInspectorWidget &GetWidget(const std::string &name);
        void RemoveWidget(const std::string &name);
        CustomInspectorWidget &AddWidget(const std::string &name, const CustomInspectorWidget &widget);
        CustomInspectorWidget &AddWidget(const std::string &label,
                                         CustomInspectorWidgetType type,
                                         const std::string &variableName = "");
        CustomInspectorWidget &AddWidgetFromString(const std::string &label, const std::string &type, const std::string &variableName);

        CustomInspectorSection &AddSection(const std::string &name,
                                           const std::string &label = "",
                                           bool collapsible         = false,
                                           bool defaultOpen         = true);
        void BeginSection(const std::string &name);
        void EndSection();
        bool HasSection(const std::string &name) const;
        CustomInspectorSection &GetSection(const std::string &name);

        SerializerNode SaveData() const;
        void LoadData(SerializerNode node);
        SerializerNode Save() const;
        void Load(SerializerNode node);
        SerializerNode SaveState() const;
        bool LoadState(SerializerNode node);
        nlohmann::json BuildSchema() const;
        bool LoadConfig(const nlohmann::json &config);

        inline void Reset()
        {
            for (auto &[name, value] : m_Values)
                value.ResetValue();
        }

        inline bool IsResetEnabled() const
        {
            return m_ShowResetButton;
        }

        bool Render();
        inline const std::string &GetDescription() const
        {
            return m_Description;
        }
        inline const std::string &GetLastChangedVariable() const
        {
            return m_LastChangedVariable;
        }
        inline const std::string &GetLastAction() const
        {
            return m_LastAction;
        }
        inline void SetShowResetButton(bool show)
        {
            m_ShowResetButton = show;
        }

        inline void Clear()
        {
            m_Values.clear();
            m_Widgets.clear();
            m_WidgetsOrder.clear();
            m_Sections.clear();
            m_SectionsOrder.clear();
            m_WidgetSections.clear();
            m_CurrentSection.clear();
            m_Description.clear();
            m_SchemaMetadata = nlohmann::json::object();
            m_LastChangedVariable.clear();
            m_LastAction.clear();
        }
        inline const std::unordered_map<std::string, CustomInspectorWidget> &GetWidgets() const
        {
            return m_Widgets;
        }
        inline const std::vector<std::string> &GetWidgetsOrder() const
        {
            return m_WidgetsOrder;
        }
        inline const std::unordered_map<std::string, CustomInspectorSection> &GetSections() const
        {
            return m_Sections;
        }
        inline const std::vector<std::string> &GetSectionsOrder() const
        {
            return m_SectionsOrder;
        }
        inline const std::unordered_map<std::string, std::string> &GetWidgetSections() const
        {
            return m_WidgetSections;
        }

    private:
        CustomInspectorValue &AddVariable(const std::string &name, const CustomInspectorValue &value);
        CustomInspectorValue &AddPathVariable(const std::string &name,
                                              const std::array<glm::vec2, CustomInspectorMaxPathPoints> &defaultPoints = {}, int defaultPointCount = 2);
        CustomInspectorValue &AddCurveVariable(const std::string &name,
                                               const std::array<glm::vec2, CustomInspectorMaxCurvePoints> &defaultPoints = {}, int defaultPointCount = 2);
        CustomInspectorValue &AddVairableFromConfig(const nlohmann::json &config);
        bool RenderWidget(const std::string &widgetLabel);
        bool RenderSlider(const CustomInspectorWidget &widget);
        bool RenderDrag(const CustomInspectorWidget &widget);
        bool RenderColor(const CustomInspectorWidget &widget);
        bool RenderTexture(const CustomInspectorWidget &widget);
        bool RenderButton(const CustomInspectorWidget &widget);
        bool RenderCheckbox(const CustomInspectorWidget &widget);
        bool RenderInput(const CustomInspectorWidget &widget);
        bool RenderSeed(CustomInspectorWidget &widget);
        bool RenderDropdown(const CustomInspectorWidget &widget);
        bool RenderPath(const CustomInspectorWidget &widget);
        bool RenderCurve(const CustomInspectorWidget &widget);

    private:
        std::unordered_map<std::string, CustomInspectorValue> m_Values;
        std::unordered_map<std::string, CustomInspectorWidget> m_Widgets;
        std::vector<std::string> m_WidgetsOrder;
        std::unordered_map<std::string, CustomInspectorSection> m_Sections;
        std::vector<std::string> m_SectionsOrder;
        std::unordered_map<std::string, std::string> m_WidgetSections;
        nlohmann::json m_SchemaMetadata = nlohmann::json::object();
        std::string m_CurrentSection;
        std::string m_ID = "";
        std::string m_Description;
        std::string m_LastChangedVariable;
        std::string m_LastAction;
        bool m_ShowResetButton = true;
    };

} // namespace tf3d::misc
using namespace tf3d::misc;
