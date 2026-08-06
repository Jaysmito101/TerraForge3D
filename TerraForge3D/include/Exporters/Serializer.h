#pragma once

#include "Base/Logging/Logger.h"
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tf3d::exporters
{

    enum class SerializerValueType {
        Boolean,
        Integer,
        Float,
        String,
        Object,
        Vector2,
        Vector3,
        Vector4,
        BooleanArray,
        IntegerArray,
        FloatArray,
        StringArray,
        Vector2Array,
        Vector3Array,
        Vector4Array,
        ObjectArray
    };

    inline constexpr std::string_view SerializerValueTypeToString(SerializerValueType type)
    {
        switch (type) {
            case SerializerValueType::Boolean:
                return "Boolean";
            case SerializerValueType::Integer:
                return "Integer";
            case SerializerValueType::Float:
                return "Float";
            case SerializerValueType::String:
                return "String";
            case SerializerValueType::Object:
                return "Object";
            case SerializerValueType::Vector2:
                return "Vector2";
            case SerializerValueType::Vector3:
                return "Vector3";
            case SerializerValueType::Vector4:
                return "Vector4";
            case SerializerValueType::BooleanArray:
                return "BooleanArray";
            case SerializerValueType::IntegerArray:
                return "IntegerArray";
            case SerializerValueType::FloatArray:
                return "FloatArray";
            case SerializerValueType::StringArray:
                return "StringArray";
            case SerializerValueType::Vector2Array:
                return "Vector2Array";
            case SerializerValueType::Vector3Array:
                return "Vector3Array";
            case SerializerValueType::Vector4Array:
                return "Vector4Array";
            case SerializerValueType::ObjectArray:
                return "ObjectArray";
        }
        return {};
    }

    inline constexpr std::optional<SerializerValueType> SerializerValueTypeFromString(
        std::string_view type)
    {
        if (type == "Boolean")
            return SerializerValueType::Boolean;
        if (type == "Integer")
            return SerializerValueType::Integer;
        if (type == "Float")
            return SerializerValueType::Float;
        if (type == "String")
            return SerializerValueType::String;
        if (type == "Object")
            return SerializerValueType::Object;
        if (type == "Vector2")
            return SerializerValueType::Vector2;
        if (type == "Vector3")
            return SerializerValueType::Vector3;
        if (type == "Vector4")
            return SerializerValueType::Vector4;
        if (type == "BooleanArray")
            return SerializerValueType::BooleanArray;
        if (type == "IntegerArray")
            return SerializerValueType::IntegerArray;
        if (type == "FloatArray")
            return SerializerValueType::FloatArray;
        if (type == "StringArray")
            return SerializerValueType::StringArray;
        if (type == "Vector2Array")
            return SerializerValueType::Vector2Array;
        if (type == "Vector3Array")
            return SerializerValueType::Vector3Array;
        if (type == "Vector4Array")
            return SerializerValueType::Vector4Array;
        if (type == "ObjectArray")
            return SerializerValueType::ObjectArray;
        return std::nullopt;
    }

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
        SerializerNodeInternal *SetTyped(const std::string &key,
                                         T &&value,
                                         SerializerValueType type)
        {
            nlohmann::json data;
            data["Type"]  = std::string(SerializerValueTypeToString(type));
            data["Value"] = std::forward<T>(value);

            m_Value[key] = std::move(data);
            m_Children.erase(key);
            m_Arrays.erase(key);
            return this;
        }

        template <typename T>
        T GetTyped(const std::string &key,
                   SerializerValueType type,
                   const T &defaultValue) const
        {
            const auto it = m_Value.find(key);
            if (it == m_Value.end()) {
                if (m_Children.contains(key) || m_Arrays.contains(key))
                    WarnInvalidField(key, "expected a scalar value");
                return defaultValue;
            }
            const auto storedType = GetStoredType(key);
            if (!it.value().is_object() || !storedType || *storedType != type ||
                !it.value().contains("Value")) {
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
                                SerializerValueType type,
                                const std::vector<T> &defaultValue) const
        {
            const auto it = m_Value.find(key);
            if (it == m_Value.end()) {
                if (m_Children.contains(key) || m_Arrays.contains(key))
                    WarnInvalidField(key, "expected a value array");
                return defaultValue;
            }
            const auto storedType = GetStoredType(key);
            if (!it.value().is_object() || !storedType || *storedType != type ||
                !it.value().contains("Value") ||
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
            return {{"X", value.x}, {"Y", value.y}};
        }

        inline static nlohmann::json EncodeVector(const glm::vec3 &value)
        {
            return {{"X", value.x}, {"Y", value.y}, {"Z", value.z}};
        }

        inline static nlohmann::json EncodeVector(const glm::vec4 &value)
        {
            return {{"X", value.x}, {"Y", value.y}, {"Z", value.z}, {"W", value.w}};
        }

        template <typename T>
        std::vector<T> GetVectorArray(const std::string &key,
                                      SerializerValueType type,
                                      const std::vector<T> &defaultValue) const
        {
            const auto it = m_Value.find(key);
            if (it == m_Value.end()) {
                if (m_Children.contains(key) || m_Arrays.contains(key))
                    WarnInvalidField(key, "expected a vector array");
                return defaultValue;
            }
            const auto storedType = GetStoredType(key);
            if (!it.value().is_object() || !storedType || *storedType != type ||
                !it.value().contains("Value") ||
                !it.value()["Value"].is_array()) {
                WarnInvalidField(key, "unexpected vector array type");
                return defaultValue;
            }
            try {
                std::vector<T> result;
                result.reserve(it.value()["Value"].size());
                for (const auto &item : it.value()["Value"]) {
                    if constexpr (std::same_as<T, glm::vec2>)
                        result.emplace_back(item.at("X").get<float>(), item.at("Y").get<float>());
                    else if constexpr (std::same_as<T, glm::vec3>)
                        result.emplace_back(item.at("X").get<float>(), item.at("Y").get<float>(),
                                            item.at("Z").get<float>());
                    else if constexpr (std::same_as<T, glm::vec4>)
                        result.emplace_back(item.at("X").get<float>(), item.at("Y").get<float>(),
                                            item.at("Z").get<float>(), item.at("W").get<float>());
                    else
                        static_assert(SerializerUnsupportedType<T>, "Unsupported serializer vector element type");
                }
                return result;
            } catch (...) {
                WarnInvalidField(key, "vector array element is malformed");
                return defaultValue;
            }
        }
        inline std::optional<SerializerValueType> GetStoredType(const std::string &key) const
        {
            const auto it = m_Value.find(key);
            if (it == m_Value.end() || !it.value().is_object() || !it.value().contains("Type"))
                return std::nullopt;
            if (!it.value()["Type"].is_string()) {
                WarnInvalidField(key, "stored type metadata is malformed");
                return std::nullopt;
            }
            try {
                const auto type = SerializerValueTypeFromString(it.value()["Type"].get<std::string>());
                if (!type)
                    WarnInvalidField(key, "unknown stored type metadata");
                return type;
            } catch (...) {
                WarnInvalidField(key, "stored type metadata is malformed");
                return std::nullopt;
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
                return SetTyped(key, value, SerializerValueType::Boolean);
            } else if constexpr (std::integral<Value> && !std::same_as<Value, bool>) {
                return SetTyped(key, value, SerializerValueType::Integer);
            } else if constexpr (std::is_enum_v<Value>) {
                using Underlying = std::underlying_type_t<Value>;
                return SetTyped(key, static_cast<Underlying>(value), SerializerValueType::Integer);
            } else if constexpr (std::floating_point<Value>) {
                return SetTyped(key, value, SerializerValueType::Float);
            } else if constexpr (std::same_as<Value, std::string>) {
                return SetTyped(key, value, SerializerValueType::String);
            } else if constexpr (std::convertible_to<Value, std::string_view>) {
                return SetTyped(key, std::string_view(value), SerializerValueType::String);
            } else if constexpr (std::same_as<Value, std::shared_ptr<SerializerNodeInternal>>) {
                m_Value.erase(key);
                m_Arrays.erase(key);
                m_Children[key] = value;
                return this;
            } else if constexpr (std::same_as<Value, glm::vec2>) {
                return SetTyped(key,
                                nlohmann::json{{"X", value.x}, {"Y", value.y}},
                                SerializerValueType::Vector2);
            } else if constexpr (std::same_as<Value, glm::vec3>) {
                return SetTyped(key,
                                nlohmann::json{{"X", value.x}, {"Y", value.y}, {"Z", value.z}},
                                SerializerValueType::Vector3);
            } else if constexpr (std::same_as<Value, glm::vec4>) {
                return SetTyped(key,
                                nlohmann::json{{"X", value.x}, {"Y", value.y}, {"Z", value.z}, {"W", value.w}},
                                SerializerValueType::Vector4);
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
                        return SetTyped(key, std::move(encoded), SerializerValueType::Vector2Array);
                    } else if constexpr (std::same_as<ElementValue, glm::vec3>) {
                        for (const auto &item : value)
                            encoded.push_back(EncodeVector(item));
                        return SetTyped(key, std::move(encoded), SerializerValueType::Vector3Array);
                    } else if constexpr (std::same_as<ElementValue, glm::vec4>) {
                        for (const auto &item : value)
                            encoded.push_back(EncodeVector(item));
                        return SetTyped(key, std::move(encoded), SerializerValueType::Vector4Array);
                    } else if constexpr (std::same_as<ElementValue, bool>) {
                        for (const auto &item : value)
                            encoded.push_back(item);
                        return SetTyped(key, std::move(encoded), SerializerValueType::BooleanArray);
                    } else if constexpr (std::integral<ElementValue> || std::is_enum_v<ElementValue>) {
                        for (const auto &item : value)
                            encoded.push_back(item);
                        return SetTyped(key, std::move(encoded), SerializerValueType::IntegerArray);
                    } else if constexpr (std::floating_point<ElementValue>) {
                        for (const auto &item : value)
                            encoded.push_back(item);
                        return SetTyped(key, std::move(encoded), SerializerValueType::FloatArray);
                    } else if constexpr (std::same_as<ElementValue, std::string> ||
                                         std::convertible_to<ElementValue, std::string_view>) {
                        for (const auto &item : value)
                            encoded.push_back(std::string_view(item));
                        return SetTyped(key, std::move(encoded), SerializerValueType::StringArray);
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
                return GetTyped<bool>(key, SerializerValueType::Boolean, defaultValue);
            } else if constexpr (std::integral<Value> && !std::same_as<Value, bool>) {
                return GetTyped<Value>(key, SerializerValueType::Integer, defaultValue);
            } else if constexpr (std::is_enum_v<Value>) {
                using Underlying = std::underlying_type_t<Value>;
                return static_cast<Value>(GetTyped<Underlying>(
                    key, SerializerValueType::Integer, static_cast<Underlying>(defaultValue)));
            } else if constexpr (std::floating_point<Value>) {
                return GetTyped<Value>(key, SerializerValueType::Float, defaultValue);
            } else if constexpr (std::same_as<Value, std::string>) {
                return GetTyped<std::string>(key, SerializerValueType::String, defaultValue);
            } else if constexpr (std::same_as<Value, std::shared_ptr<SerializerNodeInternal>>) {
                const auto it = m_Children.find(key);
                if (it != m_Children.end())
                    return it->second;
                if (HasKey(key))
                    WarnInvalidField(key, "expected a child node");
                return defaultValue;
            } else if constexpr (std::same_as<Value, glm::vec2>) {
                if (GetStoredType(key) != SerializerValueType::Vector2) {
                    if (HasKey(key))
                        WarnInvalidField(key, "expected a Vector2 value");
                    return defaultValue;
                }
                try {
                    const auto &value = m_Value.at(key).at("Value");
                    return glm::vec2(value.at("X").get<float>(), value.at("Y").get<float>());
                } catch (...) {
                    WarnInvalidField(key, "Vector2 value is malformed");
                    return defaultValue;
                }
            } else if constexpr (std::same_as<Value, glm::vec3>) {
                if (GetStoredType(key) != SerializerValueType::Vector3) {
                    if (HasKey(key))
                        WarnInvalidField(key, "expected a Vector3 value");
                    return defaultValue;
                }
                try {
                    const auto &value = m_Value.at(key).at("Value");
                    return glm::vec3(value.at("X").get<float>(), value.at("Y").get<float>(),
                                     value.at("Z").get<float>());
                } catch (...) {
                    WarnInvalidField(key, "Vector3 value is malformed");
                    return defaultValue;
                }
            } else if constexpr (std::same_as<Value, glm::vec4>) {
                if (GetStoredType(key) != SerializerValueType::Vector4) {
                    if (HasKey(key))
                        WarnInvalidField(key, "expected a Vector4 value");
                    return defaultValue;
                }
                try {
                    const auto &value = m_Value.at(key).at("Value");
                    return glm::vec4(value.at("X").get<float>(), value.at("Y").get<float>(),
                                     value.at("Z").get<float>(), value.at("W").get<float>());
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
                    return GetVectorArray<ElementValue>(key, SerializerValueType::Vector2Array, defaultValue);
                else if constexpr (std::same_as<ElementValue, glm::vec3>)
                    return GetVectorArray<ElementValue>(key, SerializerValueType::Vector3Array, defaultValue);
                else if constexpr (std::same_as<ElementValue, glm::vec4>)
                    return GetVectorArray<ElementValue>(key, SerializerValueType::Vector4Array, defaultValue);
                else if constexpr (std::same_as<ElementValue, bool>)
                    return GetArray<ElementValue>(key, SerializerValueType::BooleanArray, defaultValue);
                else if constexpr (std::integral<ElementValue> || std::is_enum_v<ElementValue>)
                    return GetArray<ElementValue>(key, SerializerValueType::IntegerArray, defaultValue);
                else if constexpr (std::floating_point<ElementValue>)
                    return GetArray<ElementValue>(key, SerializerValueType::FloatArray, defaultValue);
                else if constexpr (std::same_as<ElementValue, std::string>)
                    return GetArray<ElementValue>(key, SerializerValueType::StringArray, defaultValue);
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

        // Returns the clean application-facing representation without internal type metadata.
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

        // Returns the lossless persistence representation, retaining Type/Value metadata.
        inline nlohmann::json ToTypedJson() const
        {
            nlohmann::json data = nlohmann::json::object();
            for (const auto &it : m_Value.items())
                data[it.key()] = it.value();
            for (const auto &it : m_Children) {
                if (!it.second) {
                    WarnInvalidField(it.first, "cannot serialize a null child node");
                    continue;
                }
                data[it.first] = {
                    {"Type", std::string(SerializerValueTypeToString(SerializerValueType::Object))},
                    {"Value", it.second->ToTypedJson()}};
            }
            for (const auto &it : m_Arrays) {
                nlohmann::json values = nlohmann::json::array();
                for (const auto &node : it.second) {
                    if (!node) {
                        WarnInvalidField(it.first, "cannot serialize a null node array element");
                        continue;
                    }
                    values.push_back(node->ToTypedJson());
                }
                data[it.first] = {
                    {"Type", std::string(SerializerValueTypeToString(SerializerValueType::ObjectArray))},
                    {"Value", std::move(values)}};
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
                                       return item.is_object() && item.size() == 4 && item.contains("X") && item.contains("Y") &&
                                              item.contains("Z") && item.contains("W") && item.at("X").is_number() &&
                                              item.at("Y").is_number() && item.at("Z").is_number() &&
                                              item.at("W").is_number();
                                   })) {
                            std::vector<glm::vec4> vectors;
                            vectors.reserve(value.size());
                            for (const auto &item : value)
                                vectors.emplace_back(item.at("X").get<float>(), item.at("Y").get<float>(),
                                                     item.at("Z").get<float>(), item.at("W").get<float>());
                            Set(key, vectors);
                        } else if (std::all_of(value.begin(), value.end(), [](const nlohmann::json &item) {
                                       return item.is_object() && item.size() == 3 && item.contains("X") && item.contains("Y") &&
                                              item.contains("Z") && item.at("X").is_number() &&
                                              item.at("Y").is_number() && item.at("Z").is_number();
                                   })) {
                            std::vector<glm::vec3> vectors;
                            vectors.reserve(value.size());
                            for (const auto &item : value)
                                vectors.emplace_back(item.at("X").get<float>(), item.at("Y").get<float>(),
                                                     item.at("Z").get<float>());
                            Set(key, vectors);
                        } else if (std::all_of(value.begin(), value.end(), [](const nlohmann::json &item) {
                                       return item.is_object() && item.size() == 2 && item.contains("X") && item.contains("Y") &&
                                              item.at("X").is_number() && item.at("Y").is_number();
                                   })) {
                            std::vector<glm::vec2> vectors;
                            vectors.reserve(value.size());
                            for (const auto &item : value)
                                vectors.emplace_back(item.at("X").get<float>(), item.at("Y").get<float>());
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
                        const bool hasX = value.contains("X");
                        const bool hasY = value.contains("Y");
                        const bool hasZ = value.contains("Z");
                        const bool hasW = value.contains("W");
                        if (hasX && hasY && hasZ && hasW) {
                            Set(key, glm::vec4(value.at("X").get<float>(), value.at("Y").get<float>(),
                                               value.at("Z").get<float>(), value.at("W").get<float>()));
                        } else if (hasX && hasY && hasZ) {
                            Set(key, glm::vec3(value.at("X").get<float>(), value.at("Y").get<float>(),
                                               value.at("Z").get<float>()));
                        } else if (hasX && hasY) {
                            Set(key, glm::vec2(value.at("X").get<float>(), value.at("Y").get<float>()));
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

        inline bool LoadTypedJson(const nlohmann::json &data)
        {
            Clear();
            if (!data.is_object()) {
                TF3D_LOG_WARN("Invalid typed serializer root: expected a JSON object");
                return false;
            }

            bool valid = true;
            for (const auto &[key, value] : data.items()) {
                if (!value.is_object() || !value.contains("Type") || !value.at("Type").is_string() ||
                    !value.contains("Value")) {
                    WarnInvalidField(key, "typed value must contain Type and Value");
                    valid = false;
                    continue;
                }

                const auto type = SerializerValueTypeFromString(value.at("Type").get<std::string>());
                if (!type) {
                    WarnInvalidField(key, "unknown typed serializer value type");
                    valid = false;
                    continue;
                }

                if (*type == SerializerValueType::Object) {
                    if (!value.at("Value").is_object()) {
                        WarnInvalidField(key, "Object value must be a JSON object");
                        valid = false;
                        continue;
                    }
                    auto child = CreateSerializerNode();
                    if (!child->LoadTypedJson(value.at("Value")))
                        valid = false;
                    m_Children[key] = std::move(child);
                } else if (*type == SerializerValueType::ObjectArray) {
                    if (!value.at("Value").is_array()) {
                        WarnInvalidField(key, "ObjectArray value must be a JSON array");
                        valid = false;
                        continue;
                    }
                    auto &children = m_Arrays[key];
                    for (const auto &item : value.at("Value")) {
                        if (!item.is_object()) {
                            WarnInvalidField(key, "ObjectArray elements must be JSON objects");
                            valid = false;
                            continue;
                        }
                        auto child = CreateSerializerNode();
                        if (!child->LoadTypedJson(item))
                            valid = false;
                        children.push_back(std::move(child));
                    }
                } else {
                    m_Value[key] = {
                        {"Type", std::string(SerializerValueTypeToString(*type))},
                        {"Value", value.at("Value")}};
                }
            }
            return valid;
        }

        inline void Clear()
        {
            m_Value.clear();
            m_Children.clear();
            m_Arrays.clear();
        }

    private:
        nlohmann::json m_Value = nlohmann::json::object();
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

    inline static SerializerNode CreateSerializerNodeFromTypedJson(const nlohmann::json &data)
    {
        SerializerNode node = CreateSerializerNode();
        node->LoadTypedJson(data);
        return node;
    }

} // namespace tf3d::exporters

using tf3d::exporters::CreateSerializerNode;
using tf3d::exporters::CreateSerializerNodeFromJson;
using tf3d::exporters::CreateSerializerNodeFromTypedJson;
using tf3d::exporters::SerializerNode;
using tf3d::exporters::SerializerNodeInternal;
using tf3d::exporters::SerializerValueType;
