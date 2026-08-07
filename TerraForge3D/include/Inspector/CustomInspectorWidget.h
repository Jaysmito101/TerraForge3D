#pragma once

#include "Base/Base.h"
#include "Inspector/CustomInspectorTypes.h"

#include <string>
#include <vector>

namespace tf3d::inspector
{

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
        inline void SetShaderUniformName(const std::string &uniformName)
        {
            m_ShaderUniformName       = uniformName;
            m_ShaderUniformConfigured = true;
        }
        inline void DisableShaderUniform()
        {
            SetShaderUniformName("");
        }
        inline bool IsShaderUniformConfigured() const
        {
            return m_ShaderUniformConfigured;
        }
        inline const std::string &GetShaderUniformName() const
        {
            return m_ShaderUniformName;
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

        static std::string CustomInspectorWidgetTypeToString(CustomInspectorWidgetType type);
        static CustomInspectorWidgetType CustomInspectorWidgetTypeFromString(const std::string &type);

        friend class CustomInspector;

    private:
        std::string m_Label              = "";
        std::string m_VariableName       = "";
        std::string m_ShaderUniformName  = "";
        std::string m_FontName           = "";
        CustomInspectorWidgetType m_Type = CustomInspectorWidgetType::Unknown;
        float m_Constratins[4]           = {0.0f, 0.0f, 0.0f, 0.0f};
        float m_FSpeed                   = 1.0f;
        int32_t m_ISpeed                 = 1;
        std::string m_Tooltip            = "";
        std::vector<int32_t> m_SeedHistory;
        std::vector<std::string> m_DropdownOptions;
        std::vector<int32_t> m_DropdownValues;
        std::string m_ID                    = "";
        bool m_ShaderUniformConfigured      = false;
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

} // namespace tf3d::inspector
