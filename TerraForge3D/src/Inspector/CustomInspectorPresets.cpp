#include "Base/Base.h"
#include "Inspector/CustomInspector.h"
#include "Utils/Utils.h"

#include <cmath>
#include <limits>

namespace tf3d::inspector
{

    namespace
    {
        bool ReadPresetFloat(const nlohmann::json &source, float &destination)
        {
            if (!source.is_number())
                return false;
            const double parsed = source.get<double>();
            if (!std::isfinite(parsed) ||
                parsed < -static_cast<double>(std::numeric_limits<float>::max()) ||
                parsed > static_cast<double>(std::numeric_limits<float>::max()))
                return false;
            destination = static_cast<float>(parsed);
            return std::isfinite(destination);
        }

        bool ReadPresetInteger(const nlohmann::json &source, int32_t &destination)
        {
            int64_t parsed = 0;
            if (source.is_number_integer()) {
                parsed = source.get<int64_t>();
            } else if (source.is_number_unsigned()) {
                const uint64_t unsignedValue = source.get<uint64_t>();
                if (unsignedValue > static_cast<uint64_t>(std::numeric_limits<int32_t>::max()))
                    return false;
                parsed = static_cast<int64_t>(unsignedValue);
            } else {
                return false;
            }
            if (parsed < static_cast<int64_t>(std::numeric_limits<int32_t>::min()) ||
                parsed > static_cast<int64_t>(std::numeric_limits<int32_t>::max()))
                return false;
            destination = static_cast<int32_t>(parsed);
            return true;
        }

        bool ReadPresetVector(const nlohmann::json &source, int dimensions, float *destination)
        {
            static constexpr const char *componentNames[] = {"X", "Y", "Z", "W"};
            if (source.is_array()) {
                if (source.size() != static_cast<size_t>(dimensions))
                    return false;
                for (int index = 0; index < dimensions; ++index) {
                    if (!ReadPresetFloat(source[index], destination[index]))
                        return false;
                }
                return true;
            }
            if (!source.is_object() || source.size() != static_cast<size_t>(dimensions))
                return false;
            for (int index = 0; index < dimensions; ++index) {
                if (!source.contains(componentNames[index]) ||
                    !ReadPresetFloat(source[componentNames[index]], destination[index]))
                    return false;
            }
            return true;
        }
    } // namespace

    bool CustomInspector::SetPresetValue(std::unordered_map<std::string, CustomInspectorValue> &values,
                                         const std::string &name,
                                         const nlohmann::json &value,
                                         std::string_view presetName) const
    {
        const auto invalid = [&](std::string_view reason) {
            TF3D_LOG_WARN("Invalid CustomInspector preset '{}' field '{}': {}",
                          std::string(presetName), name, std::string(reason));
            return false;
        };

        auto target = values.find(name);
        if (target == values.end())
            return invalid("unknown inspector value");

        CustomInspectorValue candidate = target->second;
        bool converted                 = false;
        switch (candidate.GetType()) {
            case CustomInspectorValueType::Int: {
                int32_t parsed = 0;
                converted      = ReadPresetInteger(value, parsed) && candidate.Store().Set(parsed);
                break;
            }
            case CustomInspectorValueType::Float: {
                float parsed = 0.0f;
                converted    = ReadPresetFloat(value, parsed) && candidate.Store().Set(parsed);
                break;
            }
            case CustomInspectorValueType::Bool:
                converted = value.is_boolean() && candidate.Store().Set(value.get<bool>());
                break;
            case CustomInspectorValueType::String:
                converted = value.is_string() && candidate.Store().Set(value.get<std::string>());
                break;
            case CustomInspectorValueType::Vector2: {
                float components[4] = {};
                converted           = ReadPresetVector(value, 2, components) &&
                            candidate.Store().Set(glm::vec2(components[0], components[1]));
                break;
            }
            case CustomInspectorValueType::Vector3: {
                float components[4] = {};
                converted           = ReadPresetVector(value, 3, components) &&
                            candidate.Store().Set(glm::vec3(components[0], components[1], components[2]));
                break;
            }
            case CustomInspectorValueType::Vector4: {
                float components[4] = {};
                converted           = ReadPresetVector(value, 4, components) &&
                            candidate.Store().Set(glm::vec4(components[0], components[1], components[2], components[3]));
                break;
            }
            case CustomInspectorValueType::FloatArray: {
                if (!value.is_array() || value.empty())
                    break;
                std::vector<float> values;
                values.reserve(value.size());
                for (const auto &item : value) {
                    float parsed = 0.0f;
                    if (!ReadPresetFloat(item, parsed)) {
                        values.clear();
                        break;
                    }
                    values.push_back(parsed);
                }
                converted = !values.empty() && candidate.Store().Set(std::move(values));
                break;
            }
            case CustomInspectorValueType::Texture: {
                if (!value.is_string())
                    break;
                const std::string path = value.get<std::string>();
                if (path.empty() || path == "null") {
                    converted = candidate.Store().Set(std::shared_ptr<Texture2D>{});
                    break;
                }
                auto texture = std::make_shared<Texture2D>(path, true, false, candidate.m_TextureLoadAs16Bit);
                if (!texture->IsLoaded())
                    return invalid("texture could not be loaded");
                converted = candidate.Store().Set(std::move(texture));
                break;
            }
            case CustomInspectorValueType::Path:
            case CustomInspectorValueType::Curve: {
                if (!value.is_array())
                    break;
                const size_t minimumPoints = candidate.GetType() == CustomInspectorValueType::Curve ? 2u : 1u;
                if (value.size() < minimumPoints || value.size() > CustomInspectorMaxPathPoints)
                    break;
                std::vector<glm::vec2> points;
                points.reserve(value.size());
                for (const auto &point : value) {
                    float components[4] = {};
                    if (!ReadPresetVector(point, 2, components)) {
                        points.clear();
                        break;
                    }
                    points.emplace_back(components[0], components[1]);
                }
                converted = !points.empty() && candidate.Store().Set(std::move(points));
                break;
            }
            case CustomInspectorValueType::Unknown:
            case CustomInspectorValueType::Count:
            default:
                break;
        }

        if (!converted)
            return invalid("value does not match its inspector type");
        if (!ValidateValue(target->first, candidate))
            return false;
        target->second = std::move(candidate);
        return true;
    }

    bool CustomInspector::ApplyPresetValues(const nlohmann::json &values,
                                            std::string_view presetName,
                                            bool commit)
    {
        if (!values.is_object()) {
            TF3D_LOG_WARN("Invalid CustomInspector preset '{}': Values must be an object",
                          std::string(presetName));
            return false;
        }

        CustomInspectorDataStore candidates = m_ValueState.dataStore;
        auto candidateValues                = m_ValueState.metadata;
        for (auto &[name, value] : candidateValues)
            value.BindDataStore(&candidates, name);

        bool valid = true;
        for (const auto &[name, value] : values.items()) {
            if (!SetPresetValue(candidateValues, name, value, presetName))
                valid = false;
        }
        if (valid && commit)
            m_ValueState.dataStore = std::move(candidates);
        return valid;
    }

    bool CustomInspector::RenderPresetSelector()
    {
        if (m_PresetState.entries.empty())
            return false;

        std::string preview     = "Custom";
        std::string description = "Choose a preset or reset all inspector values to their defaults.";
        if (m_PresetState.selectedIndex == 0) {
            preview = "Default";
        } else if (m_PresetState.selectedIndex > 0 &&
                   static_cast<size_t>(m_PresetState.selectedIndex - 1) < m_PresetState.entries.size()) {
            const auto &preset = m_PresetState.entries[static_cast<size_t>(m_PresetState.selectedIndex - 1)];
            preview            = preset.label;
            description        = preset.description;
        }

        bool changed = false;
        if (ImGui::BeginCombo("Preset", preview.c_str())) {
            if (ImGui::Selectable("Default", m_PresetState.selectedIndex == 0)) {
                Reset();
                changed = true;
            }
            if (m_PresetState.selectedIndex == 0)
                ImGui::SetItemDefaultFocus();

            for (size_t index = 0; index < m_PresetState.entries.size(); ++index) {
                const auto &preset   = m_PresetState.entries[index];
                const int32_t choice = static_cast<int32_t>(index + 1);
                const bool selected  = m_PresetState.selectedIndex == choice;
                if (ImGui::Selectable(preset.label.c_str(), selected)) {
                    if (ApplyPresetValues(preset.values, preset.name, true)) {
                        m_PresetState.selectedIndex             = choice;
                        m_InteractionState.lastChangedVariable = "Preset";
                        m_InteractionState.lastAction.clear();
                        changed = true;
                    }
                }
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        RenderInspectorTooltip("Preset", description);
        return changed;
    }

} // namespace tf3d::inspector
