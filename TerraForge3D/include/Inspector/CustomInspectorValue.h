#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Inspector/CustomInspectorDataStore.h"
#include "Inspector/CustomInspectorTypes.h"

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace tf3d::inspector
{

    class CustomInspectorValue
    {
    public:
        using StoreView      = CustomInspectorDataStore::ValueView;
        using ConstStoreView = CustomInspectorDataStore::ConstValueView;

        explicit CustomInspectorValue(CustomInspectorValueType type = CustomInspectorValueType::Unknown)
            : m_Type(type)
        {
        }
        ~CustomInspectorValue() = default;

        CustomInspectorValueType GetType() const
        {
            return m_Type;
        }

        inline std::string GetTypeString() const
        {
            return CustomInspectorValueTypeToString(m_Type);
        }

        inline std::string GetName() const
        {
            return m_Name;
        }

        inline std::string GetSerializedName() const
        {
            return m_SerializedName.empty() ? m_Name : m_SerializedName;
        }

        inline void SetShaderUniformName(const std::string &uniformName)
        {
            m_ShaderUniformName       = uniformName;
            m_ShaderUniformConfigured = true;
        }

        inline bool IsShaderUniformConfigured() const
        {
            return m_ShaderUniformConfigured;
        }

        inline const std::string &GetShaderUniformName() const
        {
            return m_ShaderUniformName;
        }

        inline StoreView Store()
        {
            return m_DataStore == nullptr ? StoreView{} : m_DataStore->At(m_DataKey);
        }

        inline ConstStoreView Store() const
        {
            const auto *store = m_DataStore;
            return store == nullptr ? ConstStoreView{} : store->At(m_DataKey);
        }

        template <typename T>
        static constexpr CustomInspectorValueType TypeFor()
        {
            using ValueType = std::decay_t<T>;
            if constexpr (std::is_same_v<ValueType, bool>)
                return CustomInspectorValueType::Bool;
            else if constexpr (std::is_integral_v<ValueType>)
                return CustomInspectorValueType::Int;
            else if constexpr (std::is_floating_point_v<ValueType>)
                return CustomInspectorValueType::Float;
            else if constexpr (std::is_same_v<ValueType, std::string>)
                return CustomInspectorValueType::String;
            else if constexpr (std::is_same_v<ValueType, glm::vec2>)
                return CustomInspectorValueType::Vector2;
            else if constexpr (std::is_same_v<ValueType, glm::vec3>)
                return CustomInspectorValueType::Vector3;
            else if constexpr (std::is_same_v<ValueType, glm::vec4>)
                return CustomInspectorValueType::Vector4;
            else if constexpr (std::is_same_v<ValueType, std::vector<float>>)
                return CustomInspectorValueType::FloatArray;
            else if constexpr (std::is_same_v<ValueType, std::shared_ptr<Texture2D>>)
                return CustomInspectorValueType::Texture;
            else
                return CustomInspectorValueType::Unknown;
        }

        static std::string CustomInspectorValueTypeToString(CustomInspectorValueType type);
        static CustomInspectorValueType CustomInspectorValueTypeFromString(const std::string &type);

    private:
        bool WriteStateValue(SerializerNode target, const std::string &name) const;
        bool ReadStateValue(SerializerNode source, const std::string &name);
        void BindDataStore(CustomInspectorDataStore *store, std::string_view key)
        {
            m_DataStore = store;
            m_DataKey   = key;
        }

        friend class CustomInspector;

    private:
        std::string m_Name;
        std::string m_SerializedName;
        std::string m_ShaderUniformName;
        CustomInspectorValueType m_Type = CustomInspectorValueType::Unknown;
        bool m_ShaderUniformConfigured  = false;
        bool m_TextureLoadAs16Bit       = false;

        CustomInspectorDataStore *m_DataStore = nullptr;
        std::string m_DataKey;
    };

} // namespace tf3d::inspector
