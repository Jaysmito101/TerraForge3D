#pragma once

#include "Base/Logging/Logger.h"
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tf3d::exporters
{

    template <typename>
    inline constexpr bool SerializerUnsupportedType = false;

    template <typename T>
    struct SerializerVectorTraits {
        static constexpr bool IsVector = false;
    };

    template <typename T, typename Allocator>
    struct SerializerVectorTraits<std::vector<T, Allocator>> {
        static constexpr bool IsVector = true;
        using ElementType              = T;
    };

    class SerializerNodeInternal;
    static std::shared_ptr<SerializerNodeInternal> CreateSerializerNode();

    class SerializerNodeInternal
    {
    private:
        inline static void WarnInvalidField(const std::string &key, std::string_view reason)
        {
            TF3D_LOG_WARN("Invalid serializer field '{}': {}", key, reason);
        }

        template <typename T>
        SerializerNodeInternal *SetTyped(const std::string &key, T &&value, std::string_view typeName)
        {
            nlohmann::json data;
            data["Type"]  = std::string(typeName);
            data["Value"] = std::forward<T>(value);

            m_Value[key] = std::move(data);
            m_Children.erase(key);
            m_Arrays.erase(key);
            return this;
        }

        template <typename T>
        T GetTyped(const std::string &key, std::string_view typeName, const T &defaultValue) const
        {
            const auto it = m_Value.find(key);
            if (it == m_Value.end()) {
                if (m_Children.contains(key) || m_Arrays.contains(key))
                    WarnInvalidField(key, "expected a scalar value");
                return defaultValue;
            }
            if (!it.value().is_object() || !it.value().contains("Type") ||
                it.value()["Type"] != std::string(typeName) || !it.value().contains("Value")) {
                WarnInvalidField(key, "unexpected value type");
                return defaultValue;
            }
            try {
                return it.value()["Value"].get<T>();
            } catch (...) {
                WarnInvalidField(key, "value could not be converted");
                return defaultValue;
            }
        }
        template <typename T>
        std::vector<T> GetArray(const std::string &key,
                                std::string_view typeName,
                                const std::vector<T> &defaultValue) const
        {
            const auto it = m_Value.find(key);
            if (it == m_Value.end()) {
                if (m_Children.contains(key) || m_Arrays.contains(key))
                    WarnInvalidField(key, "expected a value array");
                return defaultValue;
            }
            if (!it.value().is_object() || !it.value().contains("Type") ||
                it.value()["Type"] != std::string(typeName) || !it.value().contains("Value") ||
                !it.value()["Value"].is_array()) {
                WarnInvalidField(key, "unexpected array type");
                return defaultValue;
            }
            try {
                std::vector<T> result;
                result.reserve(it.value()["Value"].size());
                for (const auto &item : it.value()["Value"])
                    result.push_back(item.get<T>());
                return result;
            } catch (...) {
                WarnInvalidField(key, "array element could not be converted");
                return defaultValue;
            }
        }
        inline static nlohmann::json EncodeVector(const glm::vec2 &value)
        {
            return {{"x", value.x}, {"y", value.y}};
        }

        inline static nlohmann::json EncodeVector(const glm::vec3 &value)
        {
            return {{"x", value.x}, {"y", value.y}, {"z", value.z}};
        }

        inline static nlohmann::json EncodeVector(const glm::vec4 &value)
        {
            return {{"x", value.x}, {"y", value.y}, {"z", value.z}, {"w", value.w}};
        }

        template <typename T>
        std::vector<T> GetVectorArray(const std::string &key,
                                      std::string_view typeName,
                                      const std::vector<T> &defaultValue) const
        {
            const auto it = m_Value.find(key);
            if (it == m_Value.end()) {
                if (m_Children.contains(key) || m_Arrays.contains(key))
                    WarnInvalidField(key, "expected a vector array");
                return defaultValue;
            }
            if (!it.value().is_object() || !it.value().contains("Type") ||
                it.value()["Type"] != std::string(typeName) || !it.value().contains("Value") ||
                !it.value()["Value"].is_array()) {
                WarnInvalidField(key, "unexpected vector array type");
                return defaultValue;
            }
            try {
                std::vector<T> result;
                result.reserve(it.value()["Value"].size());
                for (const auto &item : it.value()["Value"]) {
                    if constexpr (std::same_as<T, glm::vec2>)
                        result.emplace_back(item.at("x").get<float>(), item.at("y").get<float>());
                    else if constexpr (std::same_as<T, glm::vec3>)
                        result.emplace_back(item.at("x").get<float>(), item.at("y").get<float>(),
                                            item.at("z").get<float>());
                    else if constexpr (std::same_as<T, glm::vec4>)
                        result.emplace_back(item.at("x").get<float>(), item.at("y").get<float>(),
                                            item.at("z").get<float>(), item.at("w").get<float>());
                    else
                        static_assert(SerializerUnsupportedType<T>, "Unsupported serializer vector element type");
                }
                return result;
            } catch (...) {
                WarnInvalidField(key, "vector array element is malformed");
                return defaultValue;
            }
        }
        inline std::string GetStoredType(const std::string &key) const
        {
            const auto it = m_Value.find(key);
            if (it == m_Value.end() || !it.value().is_object() || !it.value().contains("Type"))
                return {};
            try {
                return it.value()["Type"].get<std::string>();
            } catch (...) {
                WarnInvalidField(key, "stored type metadata is malformed");
                return {};
            }
        }

    public:
        SerializerNodeInternal()  = default;
        ~SerializerNodeInternal() = default;

        template <typename T>
        SerializerNodeInternal *Set(const std::string &key, const T &value)
        {
            using Value = std::remove_cvref_t<T>;

            if constexpr (std::same_as<Value, bool>) {
                return SetTyped(key, value, "Boolean");
            } else if constexpr (std::integral<Value> && !std::same_as<Value, bool>) {
                return SetTyped(key, value, "Integer");
            } else if constexpr (std::is_enum_v<Value>) {
                using Underlying = std::underlying_type_t<Value>;
                return SetTyped(key, static_cast<Underlying>(value), "Integer");
            } else if constexpr (std::floating_point<Value>) {
                return SetTyped(key, value, "Float");
            } else if constexpr (std::same_as<Value, std::string>) {
                return SetTyped(key, value, "String");
            } else if constexpr (std::convertible_to<Value, std::string_view>) {
                return SetTyped(key, std::string_view(value), "String");
            } else if constexpr (std::same_as<Value, std::shared_ptr<SerializerNodeInternal>>) {
                m_Value.erase(key);
                m_Arrays.erase(key);
                m_Children[key] = value;
                return this;
            } else if constexpr (std::same_as<Value, glm::vec2>) {
                return SetTyped(key, nlohmann::json{{"x", value.x}, {"y", value.y}}, "Vector2");
            } else if constexpr (std::same_as<Value, glm::vec3>) {
                return SetTyped(key,
                                nlohmann::json{{"x", value.x}, {"y", value.y}, {"z", value.z}},
                                "Vector3");
            } else if constexpr (std::same_as<Value, glm::vec4>) {
                return SetTyped(key,
                                nlohmann::json{{"x", value.x}, {"y", value.y}, {"z", value.z}, {"w", value.w}},
                                "Vector4");
            } else if constexpr (SerializerVectorTraits<Value>::IsVector) {
                using Element      = typename SerializerVectorTraits<Value>::ElementType;
                using ElementValue = std::remove_cv_t<Element>;

                if constexpr (std::same_as<ElementValue, std::shared_ptr<SerializerNodeInternal>>) {
                    m_Value.erase(key);
                    m_Children.erase(key);
                    m_Arrays[key] = value;
                    return this;
                } else {

                    nlohmann::json encoded = nlohmann::json::array();

                    if constexpr (std::same_as<ElementValue, glm::vec2>) {
                        for (const auto &item : value)
                            encoded.push_back(EncodeVector(item));
                        return SetTyped(key, std::move(encoded), "Vector2Array");
                    } else if constexpr (std::same_as<ElementValue, glm::vec3>) {
                        for (const auto &item : value)
                            encoded.push_back(EncodeVector(item));
                        return SetTyped(key, std::move(encoded), "Vector3Array");
                    } else if constexpr (std::same_as<ElementValue, glm::vec4>) {
                        for (const auto &item : value)
                            encoded.push_back(EncodeVector(item));
                        return SetTyped(key, std::move(encoded), "Vector4Array");
                    } else if constexpr (std::same_as<ElementValue, bool>) {
                        for (const auto &item : value)
                            encoded.push_back(item);
                        return SetTyped(key, std::move(encoded), "BooleanArray");
                    } else if constexpr (std::integral<ElementValue> || std::is_enum_v<ElementValue>) {
                        for (const auto &item : value)
                            encoded.push_back(item);
                        return SetTyped(key, std::move(encoded), "IntegerArray");
                    } else if constexpr (std::floating_point<ElementValue>) {
                        for (const auto &item : value)
                            encoded.push_back(item);
                        return SetTyped(key, std::move(encoded), "FloatArray");
                    } else if constexpr (std::same_as<ElementValue, std::string> ||
                                         std::convertible_to<ElementValue, std::string_view>) {
                        for (const auto &item : value)
                            encoded.push_back(std::string_view(item));
                        return SetTyped(key, std::move(encoded), "StringArray");
                    } else {
                        static_assert(SerializerUnsupportedType<Value>, "Unsupported serializer vector element type");
                    }
                }
            } else {
                static_assert(SerializerUnsupportedType<Value>, "Unsupported serializer value type");
            }
        }

        template <typename T>
        T Get(const std::string &key, const T &defaultValue = T()) const
        {
            using Value = std::remove_cvref_t<T>;

            if constexpr (std::same_as<Value, bool>) {
                return GetTyped<bool>(key, "Boolean", defaultValue);
            } else if constexpr (std::integral<Value> && !std::same_as<Value, bool>) {
                return GetTyped<Value>(key, "Integer", defaultValue);
            } else if constexpr (std::is_enum_v<Value>) {
                using Underlying = std::underlying_type_t<Value>;
                return static_cast<Value>(GetTyped<Underlying>(key, "Integer", static_cast<Underlying>(defaultValue)));
            } else if constexpr (std::floating_point<Value>) {
                return GetTyped<Value>(key, "Float", defaultValue);
            } else if constexpr (std::same_as<Value, std::string>) {
                return GetTyped<std::string>(key, "String", defaultValue);
            } else if constexpr (std::same_as<Value, std::shared_ptr<SerializerNodeInternal>>) {
                const auto it = m_Children.find(key);
                if (it != m_Children.end())
                    return it->second;
                if (HasKey(key))
                    WarnInvalidField(key, "expected a child node");
                return defaultValue;
            } else if constexpr (std::same_as<Value, glm::vec2>) {
                if (GetStoredType(key) != "Vector2") {
                    if (HasKey(key))
                        WarnInvalidField(key, "expected a Vector2 value");
                    return defaultValue;
                }
                try {
                    const auto &value = m_Value.at(key).at("Value");
                    return glm::vec2(value.at("x").get<float>(), value.at("y").get<float>());
                } catch (...) {
                    WarnInvalidField(key, "Vector2 value is malformed");
                    return defaultValue;
                }
            } else if constexpr (std::same_as<Value, glm::vec3>) {
                if (GetStoredType(key) != "Vector3") {
                    if (HasKey(key))
                        WarnInvalidField(key, "expected a Vector3 value");
                    return defaultValue;
                }
                try {
                    const auto &value = m_Value.at(key).at("Value");
                    return glm::vec3(value.at("x").get<float>(), value.at("y").get<float>(),
                                     value.at("z").get<float>());
                } catch (...) {
                    WarnInvalidField(key, "Vector3 value is malformed");
                    return defaultValue;
                }
            } else if constexpr (std::same_as<Value, glm::vec4>) {
                if (GetStoredType(key) != "Vector4") {
                    if (HasKey(key))
                        WarnInvalidField(key, "expected a Vector4 value");
                    return defaultValue;
                }
                try {
                    const auto &value = m_Value.at(key).at("Value");
                    return glm::vec4(value.at("x").get<float>(), value.at("y").get<float>(),
                                     value.at("z").get<float>(), value.at("w").get<float>());
                } catch (...) {
                    WarnInvalidField(key, "Vector4 value is malformed");
                    return defaultValue;
                }
            } else if constexpr (SerializerVectorTraits<Value>::IsVector) {
                using Element      = typename SerializerVectorTraits<Value>::ElementType;
                using ElementValue = std::remove_cv_t<Element>;

                if constexpr (std::same_as<ElementValue, std::shared_ptr<SerializerNodeInternal>>) {
                    const auto it = m_Arrays.find(key);
                    if (it != m_Arrays.end())
                        return it->second;
                    if (HasKey(key))
                        WarnInvalidField(key, "expected a node array");
                    return defaultValue;
                } else if constexpr (std::same_as<ElementValue, glm::vec2>)
                    return GetVectorArray<ElementValue>(key, "Vector2Array", defaultValue);
                else if constexpr (std::same_as<ElementValue, glm::vec3>)
                    return GetVectorArray<ElementValue>(key, "Vector3Array", defaultValue);
                else if constexpr (std::same_as<ElementValue, glm::vec4>)
                    return GetVectorArray<ElementValue>(key, "Vector4Array", defaultValue);
                else if constexpr (std::same_as<ElementValue, bool>)
                    return GetArray<ElementValue>(key, "BooleanArray", defaultValue);
                else if constexpr (std::integral<ElementValue> || std::is_enum_v<ElementValue>)
                    return GetArray<ElementValue>(key, "IntegerArray", defaultValue);
                else if constexpr (std::floating_point<ElementValue>)
                    return GetArray<ElementValue>(key, "FloatArray", defaultValue);
                else if constexpr (std::same_as<ElementValue, std::string>)
                    return GetArray<ElementValue>(key, "StringArray", defaultValue);
                else
                    static_assert(SerializerUnsupportedType<Value>, "Unsupported serializer vector element type");
            } else {
                static_assert(SerializerUnsupportedType<Value>, "Unsupported serializer value type");
            }
        }

        inline bool HasKey(const std::string &key) const
        {
            return m_Value.contains(key) || m_Children.contains(key) || m_Arrays.contains(key);
        }

        inline std::vector<std::string> GetKeys() const
        {
            std::vector<std::string> keys;
            keys.reserve(m_Value.size() + m_Children.size() + m_Arrays.size());
            for (const auto &it : m_Value.items())
                keys.push_back(it.key());
            for (const auto &it : m_Children)
                keys.push_back(it.first);
            for (const auto &it : m_Arrays)
                keys.push_back(it.first);
            return keys;
        }

        inline std::shared_ptr<SerializerNodeInternal> Clone() const
        {
            auto node     = std::make_shared<SerializerNodeInternal>();
            node->m_Value = m_Value;
            for (const auto &it : m_Children)
                node->m_Children[it.first] = it.second->Clone();
            for (const auto &it : m_Arrays) {
                for (const auto &child : it.second)
                    node->m_Arrays[it.first].push_back(child->Clone());
            }
            return node;
        }

        inline SerializerNodeInternal *Merge(const SerializerNodeInternal &other)
        {
            for (const auto &it : other.m_Value.items()) {
                m_Value[it.key()] = it.value();
                m_Children.erase(it.key());
                m_Arrays.erase(it.key());
            }
            for (const auto &it : other.m_Children) {
                m_Value.erase(it.first);
                m_Arrays.erase(it.first);
                const auto current = m_Children.find(it.first);
                if (current == m_Children.end())
                    m_Children[it.first] = it.second->Clone();
                else
                    current->second->Merge(*it.second);
            }
            for (const auto &it : other.m_Arrays) {
                m_Value.erase(it.first);
                m_Children.erase(it.first);
                m_Arrays[it.first].clear();
                for (const auto &node : it.second)
                    m_Arrays[it.first].push_back(node->Clone());
            }
            return this;
        }

        inline nlohmann::json ToJson() const
        {
            nlohmann::json data = nlohmann::json::object();
            for (const auto &it : m_Value.items())
                data[it.key()] = it.value()["Value"];
            for (const auto &it : m_Children)
                data[it.first] = it.second->ToJson();
            for (const auto &it : m_Arrays) {
                data[it.first] = nlohmann::json::array();
                for (const auto &node : it.second)
                    data[it.first].push_back(node->ToJson());
            }
            return data;
        }

        inline void LoadJson(const nlohmann::json &data)
        {
            Clear();
            if (!data.is_object()) {
                TF3D_LOG_WARN("Invalid serializer root: expected a JSON object");
                return;
            }

            for (const auto &[key, value] : data.items()) {
                try {
                    if (value.is_boolean()) {
                        Set(key, value.get<bool>());
                    } else if (value.is_number_integer()) {
                        Set(key, value.get<std::int64_t>());
                    } else if (value.is_number()) {
                        Set(key, value.get<double>());
                    } else if (value.is_string()) {
                        Set(key, value.get<std::string>());
                    } else if (value.is_array()) {
                        if (value.empty()) {
                            m_Arrays[key] = {};
                        } else if (std::all_of(value.begin(), value.end(), [](const nlohmann::json &item) {
                                       return item.is_boolean();
                                   })) {
                            Set(key, value.get<std::vector<bool>>());
                        } else if (std::all_of(value.begin(), value.end(), [](const nlohmann::json &item) {
                                       return item.is_number_integer();
                                   })) {
                            Set(key, value.get<std::vector<std::int64_t>>());
                        } else if (std::all_of(value.begin(), value.end(), [](const nlohmann::json &item) {
                                       return item.is_number();
                                   })) {
                            Set(key, value.get<std::vector<double>>());
                        } else if (std::all_of(value.begin(), value.end(), [](const nlohmann::json &item) {
                                       return item.is_string();
                                   })) {
                            Set(key, value.get<std::vector<std::string>>());
                        } else if (std::all_of(value.begin(), value.end(), [](const nlohmann::json &item) {
                                       return item.is_object() && item.size() == 4 && item.contains("x") && item.contains("y") &&
                                              item.contains("z") && item.contains("w") && item.at("x").is_number() &&
                                              item.at("y").is_number() && item.at("z").is_number() &&
                                              item.at("w").is_number();
                                   })) {
                            std::vector<glm::vec4> vectors;
                            vectors.reserve(value.size());
                            for (const auto &item : value)
                                vectors.emplace_back(item.at("x").get<float>(), item.at("y").get<float>(),
                                                     item.at("z").get<float>(), item.at("w").get<float>());
                            Set(key, vectors);
                        } else if (std::all_of(value.begin(), value.end(), [](const nlohmann::json &item) {
                                       return item.is_object() && item.size() == 3 && item.contains("x") && item.contains("y") &&
                                              item.contains("z") && item.at("x").is_number() &&
                                              item.at("y").is_number() && item.at("z").is_number();
                                   })) {
                            std::vector<glm::vec3> vectors;
                            vectors.reserve(value.size());
                            for (const auto &item : value)
                                vectors.emplace_back(item.at("x").get<float>(), item.at("y").get<float>(),
                                                     item.at("z").get<float>());
                            Set(key, vectors);
                        } else if (std::all_of(value.begin(), value.end(), [](const nlohmann::json &item) {
                                       return item.is_object() && item.size() == 2 && item.contains("x") && item.contains("y") &&
                                              item.at("x").is_number() && item.at("y").is_number();
                                   })) {
                            std::vector<glm::vec2> vectors;
                            vectors.reserve(value.size());
                            for (const auto &item : value)
                                vectors.emplace_back(item.at("x").get<float>(), item.at("y").get<float>());
                            Set(key, vectors);
                        } else if (std::all_of(value.begin(), value.end(), [](const nlohmann::json &item) {
                                       return item.is_object();
                                   })) {
                            auto &nodes = m_Arrays[key];
                            for (const auto &item : value) {
                                auto node = CreateSerializerNode();
                                node->LoadJson(item);
                                nodes.push_back(std::move(node));
                            }
                        } else {
                            WarnInvalidField(key, "unsupported array element types");
                        }
                    } else if (value.is_object()) {
                        const bool hasX = value.contains("x");
                        const bool hasY = value.contains("y");
                        const bool hasZ = value.contains("z");
                        const bool hasW = value.contains("w");
                        if (hasX && hasY && hasZ && hasW) {
                            Set(key, glm::vec4(value.at("x").get<float>(), value.at("y").get<float>(),
                                               value.at("z").get<float>(), value.at("w").get<float>()));
                        } else if (hasX && hasY && hasZ) {
                            Set(key, glm::vec3(value.at("x").get<float>(), value.at("y").get<float>(),
                                               value.at("z").get<float>()));
                        } else if (hasX && hasY) {
                            Set(key, glm::vec2(value.at("x").get<float>(), value.at("y").get<float>()));
                        } else {
                            auto node = CreateSerializerNode();
                            node->LoadJson(value);
                            m_Children[key] = std::move(node);
                        }
                    } else {
                        WarnInvalidField(key, "unsupported JSON value type");
                    }
                } catch (...) {
                    WarnInvalidField(key, "JSON value could not be decoded");
                }
            }
        }

        inline void Clear()
        {
            m_Value.clear();
            m_Children.clear();
            m_Arrays.clear();
        }

    private:
        nlohmann::json m_Value;
        std::unordered_map<std::string, std::vector<std::shared_ptr<SerializerNodeInternal>>> m_Arrays;
        std::unordered_map<std::string, std::shared_ptr<SerializerNodeInternal>> m_Children;
    };

    using SerializerNode = std::shared_ptr<SerializerNodeInternal>;

    static SerializerNode CreateSerializerNode()
    {
        return std::make_shared<SerializerNodeInternal>();
    }

    inline static SerializerNode CreateSerializerNodeFromJson(const nlohmann::json &data)
    {
        SerializerNode node = CreateSerializerNode();
        node->LoadJson(data);
        return node;
    }

} // namespace tf3d::exporters

using tf3d::exporters::CreateSerializerNode;
using tf3d::exporters::CreateSerializerNodeFromJson;
using tf3d::exporters::SerializerNode;
using tf3d::exporters::SerializerNodeInternal;
