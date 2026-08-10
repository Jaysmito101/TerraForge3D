#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Inspector/CustomInspectorDataStore.h"
#include "Inspector/CustomInspectorValue.h"
#include "Inspector/CustomInspectorWidget.h"

#include <nlohmann/json.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
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
            return m_ValueState.dataStore;
        }
        inline const CustomInspectorDataStore &GetDataStore() const
        {
            return m_ValueState.dataStore;
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

        void Reset();

        inline bool IsResetEnabled() const
        {
            return m_ConfigState.showResetButton;
        }

        bool Render();
        inline const std::string &GetDescription() const
        {
            return m_ConfigState.description;
        }
        inline const std::string &GetLastChangedVariable() const
        {
            return m_InteractionState.lastChangedVariable;
        }
        inline const std::string &GetLastAction() const
        {
            return m_InteractionState.lastAction;
        }
        inline void SetShowResetButton(bool show)
        {
            m_ConfigState.showResetButton = show;
        }

        void Clear();
        inline const std::unordered_map<std::string, CustomInspectorWidget> &GetWidgets() const
        {
            return m_WidgetState.byName;
        }
        inline const std::vector<std::string> &GetWidgetsOrder() const
        {
            return m_WidgetState.order;
        }
        inline const std::unordered_map<std::string, CustomInspectorSection> &GetSections() const
        {
            return m_SectionState.byName;
        }
        inline const std::vector<std::string> &GetSectionsOrder() const
        {
            return m_SectionState.order;
        }
        inline const std::unordered_map<std::string, std::string> &GetWidgetSections() const
        {
            return m_SectionState.widgetSections;
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
            const auto existing = m_ValueState.metadata.find(std::string(path));
            if (existing == m_ValueState.metadata.end())
                return false;

            CustomInspectorDataStore candidates = m_ValueState.dataStore;
            CustomInspectorValue candidate      = existing->second;
            candidate.BindDataStore(&candidates, existing->first);
            if (!candidate.Store().Set(std::move(value)) || !ValidateValue(existing->first, candidate))
                return false;
            m_ValueState.dataStore = std::move(candidates);
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

        struct ValueState {
            CustomInspectorDataStore dataStore;
            std::unordered_map<std::string, CustomInspectorValue> metadata;
        };

        struct WidgetState {
            std::unordered_map<std::string, CustomInspectorWidget> byName;
            std::vector<std::string> order;
        };

        struct SectionState {
            std::unordered_map<std::string, CustomInspectorSection> byName;
            std::vector<std::string> order;
            std::unordered_map<std::string, std::string> widgetSections;
            std::unordered_map<std::string, nlohmann::json> customData;
            std::unordered_map<std::string, int32_t> selectionValues;
            std::string currentName;
        };

        struct ConfigState {
            nlohmann::json schemaMetadata = nlohmann::json::object();
            std::string description;
            bool showResetButton = true;
        };

        struct InteractionState {
            std::string id;
            std::string lastChangedVariable;
            std::string lastAction;
        };

        struct PresetState {
            std::vector<Preset> entries;
            int32_t selectedIndex = 0;
        };

        ValueState m_ValueState;
        WidgetState m_WidgetState;
        SectionState m_SectionState;
        ConfigState m_ConfigState;
        InteractionState m_InteractionState;
        PresetState m_PresetState;
    };

} // namespace tf3d::inspector
