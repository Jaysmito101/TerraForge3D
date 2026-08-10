#pragma once

#include "Base/Base.h"
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

        template <bool IsConst>
        class BasicValueView;

        using ValueView      = BasicValueView<false>;
        using ConstValueView = BasicValueView<true>;

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

        ValueView At(std::string_view key);
        ConstValueView At(std::string_view key) const;

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

    template <bool IsConst>
    class CustomInspectorDataStore::BasicValueView
    {
    public:
        using StoreType = std::conditional_t<IsConst, const CustomInspectorDataStore, CustomInspectorDataStore>;

        BasicValueView(StoreType *store = nullptr, std::string_view key = {})
            : m_Store(store), m_Key(key)
        {
        }

        template <typename T>
        inline auto Edit() const -> std::conditional_t<IsConst, const std::decay_t<T> *, std::decay_t<T> *>
        {
            using ValueType = std::decay_t<T>;
            if (m_Store == nullptr)
                return nullptr;
            return m_Store->template Edit<ValueType>(m_Key);
        }

        template <typename T>
        inline T Get(T fallback = {}) const
        {
            using ValueType = std::decay_t<T>;
            if (m_Store == nullptr)
                return fallback;

            if constexpr (std::is_same_v<ValueType, bool>) {
                return m_Store->template Get<bool>(m_Key, fallback);
            } else if constexpr (std::is_integral_v<ValueType>) {
                return static_cast<T>(m_Store->template Get<int32_t>(m_Key, static_cast<int32_t>(fallback)));
            } else if constexpr (std::is_floating_point_v<ValueType>) {
                return static_cast<T>(m_Store->template Get<float>(m_Key, static_cast<float>(fallback)));
            } else if constexpr (std::is_same_v<ValueType, std::string>) {
                return m_Store->template Get<std::string>(m_Key, fallback);
            } else if constexpr (std::is_same_v<ValueType, glm::vec2>) {
                return m_Store->template Get<glm::vec2>(m_Key, fallback);
            } else if constexpr (std::is_same_v<ValueType, glm::vec3>) {
                return m_Store->template Get<glm::vec3>(m_Key, fallback);
            } else if constexpr (std::is_same_v<ValueType, glm::vec4>) {
                return m_Store->template Get<glm::vec4>(m_Key, fallback);
            } else if constexpr (std::is_same_v<ValueType, std::vector<float>>) {
                return m_Store->template Get<std::vector<float>>(m_Key, fallback);
            } else if constexpr (std::is_same_v<ValueType, std::shared_ptr<Texture2D>>) {
                return m_Store->template Get<std::shared_ptr<Texture2D>>(m_Key, fallback);
            } else if constexpr (std::is_same_v<ValueType, std::vector<glm::vec2>>) {
                if (const auto *data = Edit<CustomInspectorPathData>(); data != nullptr) {
                    const int count = std::clamp(data->pointCount, 0, static_cast<int>(CustomInspectorMaxPathPoints));
                    return std::vector<glm::vec2>(data->points.begin(), data->points.begin() + count);
                }
                if (const auto *data = Edit<CustomInspectorCurveData>(); data != nullptr) {
                    const int count = std::clamp(data->pointCount, 0, static_cast<int>(CustomInspectorMaxCurvePoints));
                    return std::vector<glm::vec2>(data->points.begin(), data->points.begin() + count);
                }
                return fallback;
            } else {
                return fallback;
            }
        }

        template <typename T>
        inline T GetDefault(T fallback = {}) const
        {
            using ValueType = std::decay_t<T>;
            if (m_Store == nullptr)
                return fallback;

            if constexpr (std::is_same_v<ValueType, std::vector<float>>) {
                return m_Store->template GetDefault<std::vector<float>>(m_Key, fallback);
            } else if constexpr (std::is_same_v<ValueType, std::vector<glm::vec2>>) {
                if (Edit<CustomInspectorPathData>() != nullptr) {
                    const auto data = m_Store->template GetDefault<CustomInspectorPathData>(m_Key);
                    const int count = std::clamp(data.pointCount, 0, static_cast<int>(CustomInspectorMaxPathPoints));
                    return std::vector<glm::vec2>(data.points.begin(), data.points.begin() + count);
                }
                if (Edit<CustomInspectorCurveData>() != nullptr) {
                    const auto data = m_Store->template GetDefault<CustomInspectorCurveData>(m_Key);
                    const int count = std::clamp(data.pointCount, 0, static_cast<int>(CustomInspectorMaxCurvePoints));
                    return std::vector<glm::vec2>(data.points.begin(), data.points.begin() + count);
                }
                return fallback;
            } else if constexpr (std::is_same_v<ValueType, int32_t> ||
                                 std::is_same_v<ValueType, float> ||
                                 std::is_same_v<ValueType, bool> ||
                                 std::is_same_v<ValueType, std::string> ||
                                 std::is_same_v<ValueType, glm::vec2> ||
                                 std::is_same_v<ValueType, glm::vec3> ||
                                 std::is_same_v<ValueType, glm::vec4> ||
                                 std::is_same_v<ValueType, std::shared_ptr<Texture2D>>) {
                return m_Store->template GetDefault<ValueType>(m_Key, fallback);
            } else if constexpr (std::is_integral_v<ValueType>) {
                return static_cast<T>(m_Store->template GetDefault<int32_t>(m_Key, static_cast<int32_t>(fallback)));
            } else if constexpr (std::is_floating_point_v<ValueType>) {
                return static_cast<T>(m_Store->template GetDefault<float>(m_Key, static_cast<float>(fallback)));
            } else {
                return fallback;
            }
        }

        template <typename T>
        inline bool Set(T value)
            requires(!IsConst)
        {
            using ValueType = std::decay_t<T>;
            if (m_Store == nullptr)
                return false;

            if constexpr (std::is_same_v<ValueType, bool>) {
                return m_Store->SetRaw(m_Key, value);
            } else if constexpr (std::is_integral_v<ValueType>) {
                return m_Store->SetRaw(m_Key, static_cast<int32_t>(value));
            } else if constexpr (std::is_floating_point_v<ValueType>) {
                return m_Store->SetRaw(m_Key, static_cast<float>(value));
            } else if constexpr (std::is_same_v<ValueType, std::string>) {
                return m_Store->SetRaw(m_Key, std::move(value));
            } else if constexpr (std::is_same_v<ValueType, glm::vec2> ||
                                 std::is_same_v<ValueType, glm::vec3> ||
                                 std::is_same_v<ValueType, glm::vec4>) {
                return m_Store->SetRaw(m_Key, value);
            } else if constexpr (std::is_same_v<ValueType, std::vector<float>>) {
                return !value.empty() && m_Store->SetRaw(m_Key, std::move(value));
            } else if constexpr (std::is_same_v<ValueType, std::shared_ptr<Texture2D>>) {
                return m_Store->SetRaw(m_Key, std::move(value));
            } else if constexpr (std::is_same_v<ValueType, std::vector<glm::vec2>>) {
                if (value.empty())
                    return false;
                if (auto *data = Edit<CustomInspectorPathData>(); data != nullptr) {
                    data->pointCount = std::clamp(static_cast<int32_t>(value.size()), 1, static_cast<int32_t>(CustomInspectorMaxPathPoints));
                    for (int index = 0; index < data->pointCount; ++index)
                        data->points[static_cast<size_t>(index)] = value[static_cast<size_t>(index)];
                    return true;
                }
                if (auto *data = Edit<CustomInspectorCurveData>(); data != nullptr) {
                    data->pointCount = std::clamp(static_cast<int32_t>(value.size()), 2, static_cast<int32_t>(CustomInspectorMaxCurvePoints));
                    data->points.fill(glm::vec2(-1.0f));
                    for (int index = 0; index < data->pointCount; ++index)
                        data->points[static_cast<size_t>(index)] = value[static_cast<size_t>(index)];
                    return true;
                }
                return false;
            } else {
                return false;
            }
        }

        template <typename T>
        inline bool SetDefault(T value)
            requires(!IsConst)
        {
            return Set(std::move(value)) && m_Store->CopyCurrentToDefault(m_Key);
        }

        inline bool Reset()
            requires(!IsConst)
        {
            return m_Store != nullptr && m_Store->Reset(m_Key);
        }

    private:
        StoreType *m_Store = nullptr;
        std::string m_Key;
    };

    inline CustomInspectorDataStore::ValueView CustomInspectorDataStore::At(std::string_view key)
    {
        return ValueView(this, key);
    }

    inline CustomInspectorDataStore::ConstValueView CustomInspectorDataStore::At(std::string_view key) const
    {
        return ConstValueView(this, key);
    }

} // namespace tf3d::inspector
