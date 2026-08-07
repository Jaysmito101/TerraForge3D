#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Inspector/CustomInspectorValue.h"
#include "Inspector/CustomInspectorWidget.h"

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::inspector
{
    void RenderInspectorTooltip(const std::string &label, const std::string &description);

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
            if (existing == m_Values.end())
                return false;

            CustomInspectorValue candidate = existing->second;
            if (!candidate.Set(std::move(value)) || !ValidateValue(name, candidate))
                return false;
            existing->second = std::move(candidate);
            return true;
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

        SerializerNode SaveState() const;
        bool LoadState(SerializerNode node);
        bool LoadConfig(ApplicationState *appState, std::string_view inspectorName);
        nlohmann::json BuildSchema() const;
        bool LoadConfig(const nlohmann::json &config);
        void ApplyToShader(tf3d::base::ShaderCore &shader, std::string_view uniformPrefix = "u_") const;

        inline void Reset()
        {
            for (auto &[name, value] : m_Values)
                value.ResetValue();
            m_SelectedPreset      = 0;
            m_LastChangedVariable = "Preset";
            m_LastAction.clear();
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
            m_Presets.clear();
            m_CurrentSection.clear();
            m_Description.clear();
            m_SchemaMetadata = nlohmann::json::object();
            m_LastChangedVariable.clear();
            m_LastAction.clear();
            m_SelectedPreset = 0;
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
        bool ApplyPresetValues(const nlohmann::json &values, std::string_view presetName, bool commit);
        bool SetPresetValue(std::unordered_map<std::string, CustomInspectorValue> &values,
                            const std::string &name,
                            const nlohmann::json &value,
                            std::string_view presetName) const;
        bool RenderPresetSelector();
        bool ValidateValue(const std::string &name, const CustomInspectorValue &value) const;
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
        bool RenderOctaves(const CustomInspectorWidget &widget);

    private:
        struct Preset {
            std::string name;
            std::string label;
            std::string description;
            nlohmann::json values = nlohmann::json::object();
        };

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
        std::vector<Preset> m_Presets;
        int32_t m_SelectedPreset = 0;
    };

} // namespace tf3d::inspector
