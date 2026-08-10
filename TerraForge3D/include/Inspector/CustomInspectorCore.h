#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Inspector/CustomInspectorDataStore.h"
#include "Inspector/CustomInspectorValue.h"
#include "Inspector/CustomInspectorWidget.h"

#include <nlohmann/json.hpp>
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::inspector
{
    void RenderInspectorTooltip(const std::string &label, const std::string &description);

    template <typename Owner>
    class InspectorScopeImpl;

    class CustomInspector
    {
    public:
        CustomInspector();
        ~CustomInspector();

        InspectorScopeImpl<CustomInspector> Root();
        InspectorScopeImpl<const CustomInspector> Root() const;

        template <typename T>
        CustomInspectorValue &Add(const std::string &name, T defaultValue = {})
        {
            using ValueType             = std::decay_t<T>;
            CustomInspectorValue &value = AddVariable(name, CustomInspectorValue(CustomInspectorValue::TypeFor<ValueType>()));
            value.Store().SetDefault(std::move(defaultValue));
            return value;
        }

        inline CustomInspectorDataStore &GetDataStore()
        {
            return m_DataStore;
        }
        inline const CustomInspectorDataStore &GetDataStore() const
        {
            return m_DataStore;
        }

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
        SerializerNode SaveState(std::initializer_list<std::string_view> excludedValues) const;
        bool LoadState(SerializerNode node);
        static std::filesystem::path GetConfigPath(const std::filesystem::path &dataDirectory,
                                                   std::string_view inspectorName);
        bool LoadConfig(ApplicationState *appState, std::string_view inspectorName);
        nlohmann::json BuildSchema() const;
        bool LoadConfig(const nlohmann::json &config);
        std::optional<std::string> GetShaderUniformDeclarations(std::string *error = nullptr) const;
        std::optional<std::string> GetSectionCustomDataString(std::string_view sectionName,
                                                              std::string_view key) const;
        std::optional<int32_t> GetSectionSelectionValue(std::string_view sectionName) const;
        void ResetVisible();
        void ApplyToShader(tf3d::base::ShaderCore &shader, std::string_view uniformPrefix = "u_") const;

        inline void Reset()
        {
            for (auto &[name, value] : m_Values)
                value.Store().Reset();
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
            m_DataStore.Clear();
            m_Widgets.clear();
            m_WidgetsOrder.clear();
            m_Sections.clear();
            m_SectionsOrder.clear();
            m_WidgetSections.clear();
            m_SectionCustomData.clear();
            m_SectionSelectionValues.clear();
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
        template <typename>
        friend class InspectorScopeImpl;

        CustomInspectorValue &AddVariable(const std::string &name, const CustomInspectorValue &value);
        CustomInspectorValue *FindExactValue(std::string_view path);
        const CustomInspectorValue *FindExactValue(std::string_view path) const;
        bool RemoveExactValue(std::string_view path);
        template <typename T>
        bool SetExactValue(std::string_view path, T value)
        {
            const auto existing = m_Values.find(std::string(path));
            if (existing == m_Values.end())
                return false;

            CustomInspectorDataStore candidates = m_DataStore;
            CustomInspectorValue candidate      = existing->second;
            candidate.BindDataStore(&candidates, existing->first);
            if (!candidate.Store().Set(std::move(value)) || !ValidateValue(existing->first, candidate))
                return false;
            m_DataStore = std::move(candidates);
            return true;
        }
        std::string PathForWidget(std::string_view widgetLabel) const;
        CustomInspectorValue &ValueForWidget(const std::string &widgetLabel);
        CustomInspectorValue &AddPathVariable(const std::string &name,
                                              const std::array<glm::vec2, CustomInspectorMaxPathPoints> &defaultPoints = {}, int defaultPointCount = 2);
        CustomInspectorValue &AddCurveVariable(const std::string &name,
                                               const std::array<glm::vec2, CustomInspectorMaxCurvePoints> &defaultPoints = {}, int defaultPointCount = 2);
        CustomInspectorValue &AddVairableFromConfig(const nlohmann::json &config);
        bool IsConditionSatisfied(const std::vector<CustomInspectorRenderCondition> &conditions,
                                  std::string_view sectionName = {}) const;
        bool IsWidgetVisible(std::string_view widgetLabel) const;
        bool IsSectionVisible(std::string_view sectionName) const;
        const CustomInspectorValue *FindConditionValue(std::string_view name,
                                                       std::string_view sectionName) const;
        void ConfigureSectionSelector(const nlohmann::json &config);
        bool SetDropdownOptionsAt(std::string_view path,
                                  const std::vector<std::string> &options,
                                  const std::vector<int32_t> &values = {});
        bool ApplyPresetValues(const nlohmann::json &values, std::string_view presetName, bool commit);
        bool SetPresetValue(std::unordered_map<std::string, CustomInspectorValue> &values,
                            const std::string &name,
                            const nlohmann::json &value,
                            std::string_view presetName) const;
        bool RenderPresetSelector();
        bool ValidateValue(const std::string &name, const CustomInspectorValue &value) const;
        bool RenderWidget(const std::string &widgetLabel);
        bool RenderSlider(const std::string &widgetLabel, const CustomInspectorWidget &widget);
        bool RenderDrag(const std::string &widgetLabel, const CustomInspectorWidget &widget);
        bool RenderColor(const std::string &widgetLabel, const CustomInspectorWidget &widget);
        bool RenderTexture(const std::string &widgetLabel, const CustomInspectorWidget &widget);
        bool RenderButton(const std::string &widgetLabel, const CustomInspectorWidget &widget);
        bool RenderCheckbox(const std::string &widgetLabel, const CustomInspectorWidget &widget);
        bool RenderInput(const std::string &widgetLabel, const CustomInspectorWidget &widget);
        bool RenderSeed(const std::string &widgetLabel, CustomInspectorWidget &widget);
        bool RenderDropdown(const std::string &widgetLabel, const CustomInspectorWidget &widget);
        bool RenderPath(const std::string &widgetLabel, const CustomInspectorWidget &widget);
        bool RenderCurve(const std::string &widgetLabel, const CustomInspectorWidget &widget);
        bool RenderOctaves(const std::string &widgetLabel, const CustomInspectorWidget &widget);

    private:
        struct Preset {
            std::string name;
            std::string label;
            std::string description;
            nlohmann::json values = nlohmann::json::object();
        };

        CustomInspectorDataStore m_DataStore;
        std::unordered_map<std::string, CustomInspectorValue> m_Values;
        std::unordered_map<std::string, CustomInspectorWidget> m_Widgets;
        std::vector<std::string> m_WidgetsOrder;
        std::unordered_map<std::string, CustomInspectorSection> m_Sections;
        std::vector<std::string> m_SectionsOrder;
        std::unordered_map<std::string, std::string> m_WidgetSections;
        std::unordered_map<std::string, nlohmann::json> m_SectionCustomData;
        std::unordered_map<std::string, int32_t> m_SectionSelectionValues;
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
