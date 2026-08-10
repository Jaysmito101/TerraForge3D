#pragma once

#include "Inspector/CustomInspectorTypes.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace tf3d::inspector
{

    struct CustomInspectorRenderCondition {
        std::string name;
        std::vector<int32_t> values;
    };

    class CustomInspectorWidget
    {
    public:
        CustomInspectorWidget(CustomInspectorWidgetType type = CustomInspectorWidgetType::Unknown);
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
            m_Constraints = {a, b, c, d};
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
            m_Speed = speed;
        }
        inline void SetRenderOnCondition(const std::string &conditionName, int32_t conditionValue)
        {
            SetRenderOnConditions(conditionName, {conditionValue});
        }
        inline void SetRenderOnConditions(const std::string &conditionName, const std::vector<int32_t> &conditionValues)
        {
            ClearCondition();
            AddRenderOnCondition(conditionName, conditionValues);
        }
        inline void AddRenderOnCondition(const std::string &conditionName, const std::vector<int32_t> &conditionValues)
        {
            if (conditionName.empty())
                return;
            m_RenderOnConditions.push_back({conditionName, conditionValues});
        }
        inline void ClearCondition()
        {
            m_RenderOnConditions.clear();
        }

        static std::string CustomInspectorWidgetTypeToString(CustomInspectorWidgetType type);
        static CustomInspectorWidgetType CustomInspectorWidgetTypeFromString(const std::string &type);

        friend class CustomInspector;

    private:
        CustomInspectorWidgetType m_Type = CustomInspectorWidgetType::Unknown;
        std::string m_ID;

        std::string m_Label;
        std::string m_VariableName;
        std::string m_FontName;
        std::string m_Tooltip;

        std::array<float, 4> m_Constraints{};
        float m_Speed = 1.0f;

        std::vector<int32_t> m_SeedHistory;
        std::vector<std::string> m_DropdownOptions;
        std::vector<int32_t> m_DropdownValues;
        std::vector<CustomInspectorRenderCondition> m_RenderOnConditions;
    };

    struct CustomInspectorSection {
        std::string name;
        std::string label;
        std::string description;
        bool collapsible = false;
        bool defaultOpen = true;
        std::vector<CustomInspectorRenderCondition> renderConditions;
    };

} // namespace tf3d::inspector
