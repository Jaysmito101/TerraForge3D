#pragma once

#include "Base/Base.h"
#include "Inspector/CustomInspectorTypes.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace tf3d::inspector
{

    struct CustomInspectorPathData {
        std::array<glm::vec2, CustomInspectorMaxPathPoints> points{};
        int32_t pointCount = 2;
    };

    struct CustomInspectorCurveData {
        std::array<glm::vec2, CustomInspectorMaxCurvePoints> points{};
        int32_t pointCount = 2;
    };

    class CustomInspectorDataStore
    {
    public:
        using RawValue = std::variant<std::monostate,
                                      int32_t,
                                      float,
                                      bool,
                                      std::string,
                                      glm::vec2,
                                      glm::vec3,
                                      glm::vec4,
                                      std::vector<float>,
                                      std::shared_ptr<Texture2D>,
                                      CustomInspectorPathData,
                                      CustomInspectorCurveData>;

        void Ensure(std::string_view key, CustomInspectorValueType type)
        {
            const RawValue defaultValue = DefaultForType(type);
            m_Entries[std::string(key)] = Entry{defaultValue, defaultValue};
        }

        bool Contains(std::string_view key) const
        {
            return m_Entries.find(std::string(key)) != m_Entries.end();
        }

        bool Remove(std::string_view key)
        {
            return m_Entries.erase(std::string(key)) != 0;
        }

        void Clear()
        {
            m_Entries.clear();
        }

        template <typename T>
        T *Edit(std::string_view key)
        {
            const auto entry = m_Entries.find(std::string(key));
            if (entry == m_Entries.end())
                return nullptr;
            return std::get_if<T>(&entry->second.value);
        }

        template <typename T>
        const T *Edit(std::string_view key) const
        {
            const auto entry = m_Entries.find(std::string(key));
            if (entry == m_Entries.end())
                return nullptr;
            return std::get_if<T>(&entry->second.value);
        }

        template <typename T>
        T Get(std::string_view key, T fallback = {}) const
        {
            const auto *value = Edit<T>(key);
            return value == nullptr ? fallback : *value;
        }

        template <typename T>
        T GetDefault(std::string_view key, T fallback = {}) const
        {
            const auto entry = m_Entries.find(std::string(key));
            if (entry == m_Entries.end())
                return fallback;
            const auto *value = std::get_if<T>(&entry->second.defaultValue);
            return value == nullptr ? fallback : *value;
        }

        bool SetRaw(std::string_view key, RawValue value)
        {
            const auto entry = m_Entries.find(std::string(key));
            if (entry == m_Entries.end() || entry->second.value.index() != value.index())
                return false;
            entry->second.value = std::move(value);
            return true;
        }

        bool CopyCurrentToDefault(std::string_view key)
        {
            const auto entry = m_Entries.find(std::string(key));
            if (entry == m_Entries.end())
                return false;
            entry->second.defaultValue = entry->second.value;
            return true;
        }

        bool Reset(std::string_view key)
        {
            const auto entry = m_Entries.find(std::string(key));
            if (entry == m_Entries.end())
                return false;
            entry->second.value = entry->second.defaultValue;
            return true;
        }

    private:
        struct Entry {
            RawValue value;
            RawValue defaultValue;
        };

        static RawValue DefaultForType(CustomInspectorValueType type)
        {
            switch (type) {
                case CustomInspectorValueType::Int:
                    return int32_t{0};
                case CustomInspectorValueType::Float:
                    return 0.0f;
                case CustomInspectorValueType::Bool:
                    return false;
                case CustomInspectorValueType::String:
                    return std::string{};
                case CustomInspectorValueType::Vector2:
                    return glm::vec2(0.0f);
                case CustomInspectorValueType::Vector3:
                    return glm::vec3(0.0f);
                case CustomInspectorValueType::Vector4:
                    return glm::vec4(0.0f);
                case CustomInspectorValueType::FloatArray:
                    return std::vector<float>{};
                case CustomInspectorValueType::Texture:
                    return std::shared_ptr<Texture2D>{};
                case CustomInspectorValueType::Path:
                    return CustomInspectorPathData{};
                case CustomInspectorValueType::Curve:
                    return CustomInspectorCurveData{};
                case CustomInspectorValueType::Unknown:
                case CustomInspectorValueType::Count:
                default:
                    return std::monostate{};
            }
        }

        std::unordered_map<std::string, Entry> m_Entries;
    };

} // namespace tf3d::inspector
