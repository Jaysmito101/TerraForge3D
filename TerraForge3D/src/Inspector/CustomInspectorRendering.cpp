#include "Base/Base.h"
#include "Inspector/CustomInspector.h"
#include "UI/ImGuiComponents.h"
#include "Utils/PathEditor.h"
#include "Utils/Utils.h"

#include <algorithm>
#include <cstring>

namespace tf3d::inspector
{

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

    bool CustomInspector::RenderWidget(const std::string &widgetLabel)
    {
        const auto widgetIterator = m_Widgets.find(widgetLabel);
        if (widgetIterator == m_Widgets.end())
            return false;

        const auto &widget = widgetIterator->second;
        if (!IsWidgetVisible(widgetLabel))
            return false;

        ImGui::PushID(widget.m_ID.c_str());
        if (!widget.m_FontName.empty())
            ImGui::PushFont(GetUIFont(widget.m_FontName));
        bool widgetChanged = false;
        if (widget.m_Type == CustomInspectorWidgetType::Slider)
            widgetChanged = RenderSlider(widgetLabel, widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Drag)
            widgetChanged = RenderDrag(widgetLabel, widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Color)
            widgetChanged = RenderColor(widgetLabel, widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Texture)
            widgetChanged = RenderTexture(widgetLabel, widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Path)
            widgetChanged = RenderPath(widgetLabel, widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Curve)
            widgetChanged = RenderCurve(widgetLabel, widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Octaves)
            widgetChanged = RenderOctaves(widgetLabel, widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Button)
            widgetChanged = RenderButton(widgetLabel, widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Checkbox)
            widgetChanged = RenderCheckbox(widgetLabel, widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Input)
            widgetChanged = RenderInput(widgetLabel, widget);
        else if (widget.m_Type == CustomInspectorWidgetType::Seed)
            widgetChanged = RenderSeed(widgetLabel, m_Widgets[widgetLabel]);
        else if (widget.m_Type == CustomInspectorWidgetType::Dropdown)
            widgetChanged = RenderDropdown(widgetLabel, widget);
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
                    ValueForWidget(widgetLabel).Store().Reset();
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
                if (!IsSectionVisible(sectionName))
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

    bool CustomInspector::RenderSlider(const std::string &widgetLabel, const CustomInspectorWidget &widget)
    {
        bool hasChanged = false;
        auto &value     = ValueForWidget(widgetLabel);
        switch (value.GetType()) {
            case CustomInspectorValueType::Int:
                hasChanged = ImGui::SliderInt(widget.m_Label.c_str(), value.Store().Edit<int32_t>(), static_cast<int32_t>(widget.m_Constratins[0]), static_cast<int32_t>(widget.m_Constratins[1]));
                break;
            case CustomInspectorValueType::Float:
                hasChanged = ImGui::SliderFloat(widget.m_Label.c_str(), value.Store().Edit<float>(), widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType::Vector2:
                hasChanged = ImGui::SliderFloat2(widget.m_Label.c_str(), glm::value_ptr(*value.Store().Edit<glm::vec2>()), widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType::Vector3:
                hasChanged = ImGui::SliderFloat3(widget.m_Label.c_str(), glm::value_ptr(*value.Store().Edit<glm::vec3>()), widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType::Vector4:
                hasChanged = ImGui::SliderFloat4(widget.m_Label.c_str(), glm::value_ptr(*value.Store().Edit<glm::vec4>()), widget.m_Constratins[0], widget.m_Constratins[1]);
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

    bool CustomInspector::RenderDrag(const std::string &widgetLabel, const CustomInspectorWidget &widget)
    {
        bool hasChanged = false;
        auto &value     = ValueForWidget(widgetLabel);
        switch (value.GetType()) {
            case CustomInspectorValueType::Int:
                hasChanged = ImGui::DragInt(widget.m_Label.c_str(), value.Store().Edit<int32_t>(), widget.m_FSpeed, static_cast<int32_t>(widget.m_Constratins[0]), static_cast<int32_t>(widget.m_Constratins[1]));
                if (abs(widget.m_Constratins[0] - widget.m_Constratins[1]) < 0.001f)
                    break; // no constraints
                *value.Store().Edit<int32_t>() = std::clamp(*value.Store().Edit<int32_t>(), static_cast<int32_t>(widget.m_Constratins[0]), static_cast<int32_t>(widget.m_Constratins[1]));
                break;
            case CustomInspectorValueType::Float:
                hasChanged = ImGui::DragFloat(widget.m_Label.c_str(), value.Store().Edit<float>(), widget.m_FSpeed, widget.m_Constratins[0], widget.m_Constratins[1]);
                if (abs(widget.m_Constratins[0] - widget.m_Constratins[1]) < 0.001f)
                    break; // no constraints
                *value.Store().Edit<float>() = std::clamp(*value.Store().Edit<float>(), widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType::Vector2:
                hasChanged = ImGui::DragFloat2(widget.m_Label.c_str(), glm::value_ptr(*value.Store().Edit<glm::vec2>()), widget.m_FSpeed, widget.m_Constratins[0], widget.m_Constratins[1]);
                if (abs(widget.m_Constratins[0] - widget.m_Constratins[1]) < 0.001f)
                    break; // no constraints
                value.Store().Edit<glm::vec2>()->x = std::clamp(value.Store().Edit<glm::vec2>()->x, widget.m_Constratins[0], widget.m_Constratins[1]);
                value.Store().Edit<glm::vec2>()->y = std::clamp(value.Store().Edit<glm::vec2>()->y, widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType::Vector3:
                hasChanged = ImGui::DragFloat3(widget.m_Label.c_str(), glm::value_ptr(*value.Store().Edit<glm::vec3>()), widget.m_FSpeed, widget.m_Constratins[0], widget.m_Constratins[1]);
                if (abs(widget.m_Constratins[0] - widget.m_Constratins[1]) < 0.001f)
                    break; // no constraints
                value.Store().Edit<glm::vec3>()->x = std::clamp(value.Store().Edit<glm::vec3>()->x, widget.m_Constratins[0], widget.m_Constratins[1]);
                value.Store().Edit<glm::vec3>()->y = std::clamp(value.Store().Edit<glm::vec3>()->y, widget.m_Constratins[0], widget.m_Constratins[1]);
                value.Store().Edit<glm::vec3>()->z = std::clamp(value.Store().Edit<glm::vec3>()->z, widget.m_Constratins[0], widget.m_Constratins[1]);
                break;
            case CustomInspectorValueType::Vector4:
                hasChanged = ImGui::DragFloat4(widget.m_Label.c_str(), glm::value_ptr(*value.Store().Edit<glm::vec4>()), widget.m_FSpeed, widget.m_Constratins[0], widget.m_Constratins[1]);
                if (abs(widget.m_Constratins[0] - widget.m_Constratins[1]) < 0.001f)
                    break; // no constraints
                value.Store().Edit<glm::vec4>()->x = std::clamp(value.Store().Edit<glm::vec4>()->x, widget.m_Constratins[0], widget.m_Constratins[1]);
                value.Store().Edit<glm::vec4>()->y = std::clamp(value.Store().Edit<glm::vec4>()->y, widget.m_Constratins[0], widget.m_Constratins[1]);
                value.Store().Edit<glm::vec4>()->z = std::clamp(value.Store().Edit<glm::vec4>()->z, widget.m_Constratins[0], widget.m_Constratins[1]);
                value.Store().Edit<glm::vec4>()->w = std::clamp(value.Store().Edit<glm::vec4>()->w, widget.m_Constratins[0], widget.m_Constratins[1]);
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

    bool CustomInspector::RenderColor(const std::string &widgetLabel, const CustomInspectorWidget &widget)
    {
        bool hasChanged = false;
        auto &value     = ValueForWidget(widgetLabel);
        switch (value.GetType()) {
            case CustomInspectorValueType::Int: {
                ImVec4 color = ImGui::ColorConvertU32ToFloat4(static_cast<ImU32>(value.Store().Get<int32_t>()));
                hasChanged   = ImGui::ColorEdit4(widget.m_Label.c_str(), &color.x);
                if (hasChanged)
                    value.Store().Set(static_cast<int32_t>(ImGui::ColorConvertFloat4ToU32(color)));
                break;
            }
            case CustomInspectorValueType::Vector3:
                hasChanged = ImGui::ColorEdit3(widget.m_Label.c_str(), glm::value_ptr(*value.Store().Edit<glm::vec3>()));
                break;
            case CustomInspectorValueType::Vector4:
                hasChanged = ImGui::ColorEdit4(widget.m_Label.c_str(), glm::value_ptr(*value.Store().Edit<glm::vec4>()));
                break;
            case CustomInspectorValueType::String: {
                ImU32 packed                  = 0;
                const std::string colorString = value.Store().Get<std::string>();
                if (colorString.size() == 8) {
                    try {
                        packed = static_cast<ImU32>(std::stoul(colorString, nullptr, 16));
                    } catch (const std::exception &) {
                        packed = 0;
                    }
                }
                ImVec4 color = ImGui::ColorConvertU32ToFloat4(packed);
                hasChanged   = ImGui::ColorEdit4(widget.m_Label.c_str(), &color.x);
                if (hasChanged)
                    value.Store().Set(ColorConvertToHexString(color.x, color.y, color.z, color.w));
                break;
            }
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

    bool CustomInspector::RenderTexture(const std::string &widgetLabel, const CustomInspectorWidget &widget)
    {
        bool hasChanged = false;

        auto &value = ValueForWidget(widgetLabel);
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
        auto *texture         = value.Store().Edit<std::shared_ptr<Texture2D>>();
        ImTextureID textureID = texture != nullptr && *texture
                                    ? (ImTextureID)(int64_t)(*texture)->GetRendererID()
                                    : static_cast<ImTextureID>(0);
        if (ImGui::ImageButton(textureID, ImVec2(widget.m_Constratins[0], widget.m_Constratins[1]))) {
            std::string path = ShowOpenFileDialog("*.*");
            if (path.size() > 3) {
                *texture   = std::make_shared<Texture2D>(path, false, false, value.m_TextureLoadAs16Bit);
                hasChanged = true;
            }
        }
        return hasChanged;
    }

    bool CustomInspector::RenderDropdown(const std::string &widgetLabel, const CustomInspectorWidget &widget)
    {
        bool hasChanged = false;
        auto &value     = ValueForWidget(widgetLabel);
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
        auto *selectedValue        = value.Store().Edit<int32_t>();
        int32_t selectedIndex      = 0;
        if (hasMappedValues) {
            const auto selected = std::find(widget.m_DropdownValues.begin(), widget.m_DropdownValues.end(), *selectedValue);
            if (selected != widget.m_DropdownValues.end()) {
                selectedIndex = static_cast<int32_t>(std::distance(widget.m_DropdownValues.begin(), selected));
            } else {
                *selectedValue = widget.m_DropdownValues.front();
            }
        } else {
            selectedIndex  = glm::clamp(*selectedValue, 0, static_cast<int32_t>(widget.m_DropdownOptions.size()) - 1);
            *selectedValue = selectedIndex;
        }

        if (ImGui::BeginCombo(widget.m_Label.c_str(), widget.m_DropdownOptions[selectedIndex].c_str())) {
            for (int i = 0; i < static_cast<int32_t>(widget.m_DropdownOptions.size()); i++) {
                const bool isSelected = (selectedIndex == i);
                if (ImGui::Selectable(widget.m_DropdownOptions[i].c_str(), isSelected)) {
                    *selectedValue = hasMappedValues ? widget.m_DropdownValues[i] : i;
                    hasChanged     = true;
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        return hasChanged;
    }

    bool CustomInspector::RenderButton(const std::string &widgetLabel, const CustomInspectorWidget &widget)
    {
        (void)widgetLabel;
        return ImGui::Button(widget.m_Label.c_str());
    }

    bool CustomInspector::RenderPath(const std::string &widgetLabel, const CustomInspectorWidget &widget)
    {
        auto &value = ValueForWidget(widgetLabel);
        if (value.GetType() != CustomInspectorValueType::Path)
            throw std::runtime_error("Invalid data type for Path");
        auto *path = value.Store().Edit<CustomInspectorPathData>();
        if (path == nullptr)
            throw std::runtime_error("Path value is not present in the data store");
        return utils::DrawPathEditor<CustomInspectorMaxPathPoints>(
            widget.m_Label.c_str(), path->points, path->pointCount);
    }

    bool CustomInspector::RenderCurve(const std::string &widgetLabel, const CustomInspectorWidget &widget)
    {
        auto &value = ValueForWidget(widgetLabel);
        if (value.GetType() != CustomInspectorValueType::Curve)
            throw std::runtime_error("Invalid data type for Curve");

        auto *curve = value.Store().Edit<CustomInspectorCurveData>();
        if (curve == nullptr)
            throw std::runtime_error("Curve value is not present in the data store");

        std::array<ImVec2, CustomInspectorMaxCurvePoints> points{};
        for (size_t index = 0; index < CustomInspectorMaxCurvePoints; ++index)
            points[index] = ImVec2(curve->points[index].x, curve->points[index].y);

        const float width            = std::max(ImGui::GetContentRegionAvail().x, 220.0f);
        const std::string curveLabel = widget.m_Label + "##" + widget.m_ID;
        const bool changed           = ImGui::Curve(curveLabel.c_str(), ImVec2(width, 180.0f),
                                                    static_cast<int>(CustomInspectorMaxCurvePoints), points.data()) != 0;
        int pointCount               = 0;
        while (pointCount < static_cast<int>(CustomInspectorMaxCurvePoints) && points[pointCount].x >= 0.0f)
            ++pointCount;
        pointCount = std::clamp(pointCount, 2, static_cast<int>(CustomInspectorMaxCurvePoints));
        for (size_t index = 0; index < CustomInspectorMaxCurvePoints; ++index)
            curve->points[index] = glm::vec2(points[index].x, points[index].y);
        curve->pointCount = pointCount;
        return changed;
    }

    bool CustomInspector::RenderOctaves(const std::string &widgetLabel, const CustomInspectorWidget &widget)
    {
        auto &value = ValueForWidget(widgetLabel);
        if (value.GetType() != CustomInspectorValueType::FloatArray)
            throw std::runtime_error("Invalid data type for Octaves");

        auto *octaves = value.Store().Edit<std::vector<float>>();
        if (octaves == nullptr || octaves->empty())
            return false;

        ImGui::TextUnformatted(widget.m_Label.c_str());
        bool changed = false;
        ImGui::PushID((widget.m_ID + "Values").c_str());
        for (size_t index = 0; index < octaves->size(); ++index) {
            ImGui::PushID(static_cast<int>(index));
            const std::string label = "Octave " + std::to_string(index + 1);
            if (ImGui::VSliderFloat("##Value",
                                    ImVec2(20.0f, 200.0f),
                                    &(*octaves)[index],
                                    widget.m_Constratins[0],
                                    widget.m_Constratins[1]))
                changed = true;
            RenderInspectorTooltip(label, std::to_string((*octaves)[index]));
            ImGui::SameLine();
            ImGui::PopID();
        }
        ImGui::NewLine();
        ImGui::PopID();
        return changed;
    }

    bool CustomInspector::RenderCheckbox(const std::string &widgetLabel, const CustomInspectorWidget &widget)
    {
        bool hasChanged = false;
        auto &value     = ValueForWidget(widgetLabel);
        bool checked    = false;
        switch (value.GetType()) {
            case CustomInspectorValueType::Int:
                checked = value.Store().Get<int32_t>() != 0;
                break;
            case CustomInspectorValueType::Float:
                checked = value.Store().Get<float>() != 0.0f;
                break;
            case CustomInspectorValueType::Vector2:
                checked = value.Store().Get<glm::vec2>().x != 0.0f;
                break;
            case CustomInspectorValueType::Vector3:
                checked = value.Store().Get<glm::vec3>().x != 0.0f;
                break;
            case CustomInspectorValueType::Vector4:
                checked = value.Store().Get<glm::vec4>().x != 0.0f;
                break;
            case CustomInspectorValueType::String:
                checked = value.Store().Get<std::string>() == "true";
                break;
            case CustomInspectorValueType::Bool:
                checked = value.Store().Get<bool>();
                break;
            case CustomInspectorValueType::Texture:
                throw std::runtime_error(std::string("Invalid data type for Checkbox"));
            default:
                throw std::runtime_error(std::string("Invalid data type for Checkbox"));
        }
        hasChanged = ImGui::Checkbox(widget.m_Label.c_str(), &checked);
        if (!hasChanged)
            return false;

        switch (value.GetType()) {
            case CustomInspectorValueType::Int:
                value.Store().Set<int32_t>(checked ? 1 : 0);
                break;
            case CustomInspectorValueType::Float:
                value.Store().Set<float>(checked ? 1.0f : 0.0f);
                break;
            case CustomInspectorValueType::Vector2:
                value.Store().Set(glm::vec2(checked ? 1.0f : 0.0f));
                break;
            case CustomInspectorValueType::Vector3:
                value.Store().Set(glm::vec3(checked ? 1.0f : 0.0f));
                break;
            case CustomInspectorValueType::Vector4:
                value.Store().Set(glm::vec4(checked ? 1.0f : 0.0f));
                break;
            case CustomInspectorValueType::String:
                value.Store().Set(std::string(checked ? "true" : "false"));
                break;
            case CustomInspectorValueType::Bool:
                value.Store().Set(checked);
                break;
            default:
                break;
        }
        return hasChanged;
    }

    bool CustomInspector::RenderInput(const std::string &widgetLabel, const CustomInspectorWidget &widget)
    {
        bool hasChanged = false;
        auto &value     = ValueForWidget(widgetLabel);
        switch (value.GetType()) {
            case CustomInspectorValueType::Int:
                hasChanged = ImGui::InputInt(widget.m_Label.c_str(), value.Store().Edit<int32_t>(), widget.m_ISpeed, widget.m_ISpeed * 10);
                break;
            case CustomInspectorValueType::Float:
                hasChanged = ImGui::InputFloat(widget.m_Label.c_str(), value.Store().Edit<float>(), widget.m_FSpeed, widget.m_FSpeed * 10.0f);
                break;
            case CustomInspectorValueType::Vector2:
                hasChanged = ImGui::InputFloat2(widget.m_Label.c_str(), glm::value_ptr(*value.Store().Edit<glm::vec2>()));
                break;
            case CustomInspectorValueType::Vector3:
                hasChanged = ImGui::InputFloat3(widget.m_Label.c_str(), glm::value_ptr(*value.Store().Edit<glm::vec3>()));
                break;
            case CustomInspectorValueType::Vector4:
                hasChanged = ImGui::InputFloat4(widget.m_Label.c_str(), glm::value_ptr(*value.Store().Edit<glm::vec4>()));
                break;
            case CustomInspectorValueType::String:
                static char s_Buffer[4096];
                std::strcpy(s_Buffer, value.Store().Get<std::string>().c_str());
                hasChanged = ImGui::InputText(widget.m_Label.c_str(), s_Buffer, sizeof(s_Buffer));
                value.Store().Set(std::string(s_Buffer));
                break;
            case CustomInspectorValueType::Bool:
            case CustomInspectorValueType::Texture:
                throw std::runtime_error(std::string("Invalid data type for Input"));
            default:
                throw std::runtime_error(std::string("Invalid data type for Input"));
        }
        return hasChanged;
    }

    bool CustomInspector::RenderSeed(const std::string &widgetLabel, CustomInspectorWidget &widget)
    {
        bool hasChanged = false;
        auto &value     = ValueForWidget(widgetLabel);
        switch (value.GetType()) {
            case CustomInspectorValueType::Int: {
                int seed   = value.Store().Get<int32_t>();
                hasChanged = ShowSeedSettings(widget.m_Label, &seed, widget.m_SeedHistory);
                if (hasChanged)
                    value.Store().Set<int32_t>(seed);
                break;
            }
            case CustomInspectorValueType::Float: {
                int seed   = static_cast<int>(value.Store().Get<float>());
                hasChanged = ShowSeedSettings(widget.m_Label, &seed, widget.m_SeedHistory);
                if (hasChanged)
                    value.Store().Set<float>(static_cast<float>(seed));
                break;
            }
            case CustomInspectorValueType::String:
                if (ImGui::Button(("Seed Value: " + value.Store().Get<std::string>() + " [Click to change]").c_str())) {
                    value.Store().Set(GenerateId(8));
                    hasChanged = true;
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

} // namespace tf3d::inspector
