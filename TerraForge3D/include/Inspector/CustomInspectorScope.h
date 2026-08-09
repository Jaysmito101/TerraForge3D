#pragma once

#include "Inspector/CustomInspectorValue.h"

#include <concepts>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace tf3d::inspector
{

    class CustomInspector;

    template <typename Owner>
    concept InspectorOwner =
        std::same_as<std::remove_const_t<Owner>, CustomInspector>;

    template <typename Owner>
    concept MutableInspectorOwner =
        InspectorOwner<Owner> && !std::is_const_v<Owner>;

    template <typename Owner>
    class InspectorScopeImpl
    {
        static_assert(InspectorOwner<Owner>, "InspectorScopeImpl owner must be CustomInspector or const CustomInspector");

    public:
        InspectorScopeImpl() = default;

        InspectorScopeImpl Scope(std::string_view path) const
        {
            return InspectorScopeImpl(m_Inspector, JoinPath(m_Path, path));
        }

        template <typename T>
        T Get(std::string_view name, T fallback = {}) const
        {
            const auto *value = Find(name);
            return value == nullptr ? fallback : value->Get(fallback);
        }

        template <typename T>
            requires MutableInspectorOwner<Owner>
        bool Set(std::string_view name, T value) const
        {
            return m_Inspector != nullptr &&
                   m_Inspector->SetExactValue(JoinPath(m_Path, name), std::move(value));
        }

        const CustomInspectorValue *Find(std::string_view name) const
        {
            return m_Inspector == nullptr ? nullptr : m_Inspector->FindExactValue(JoinPath(m_Path, name));
        }

        bool Contains(std::string_view name) const
        {
            return Find(name) != nullptr;
        }

        bool Remove(std::string_view name) const
            requires MutableInspectorOwner<Owner>
        {
            return m_Inspector != nullptr &&
                   m_Inspector->m_Values.erase(JoinPath(m_Path, name)) != 0;
        }

        bool SetDropdownOptions(std::string_view name,
                                const std::vector<std::string> &options,
                                const std::vector<int32_t> &values = {}) const
            requires MutableInspectorOwner<Owner>
        {
            return m_Inspector != nullptr &&
                   m_Inspector->SetDropdownOptionsAt(JoinPath(m_Path, name), options, values);
        }

        const std::string &GetPath() const
        {
            return m_Path;
        }

    private:
        friend class CustomInspector;

        InspectorScopeImpl(Owner *inspector, std::string path)
            : m_Inspector(inspector), m_Path(std::move(path))
        {
        }

        static std::string JoinPath(std::string_view parent, std::string_view child)
        {
            if (parent.empty())
                return std::string(child);
            if (child.empty())
                return std::string(parent);
            return std::string(parent) + "." + std::string(child);
        }

        Owner *m_Inspector = nullptr;
        std::string m_Path;
    };

    using InspectorScope      = InspectorScopeImpl<CustomInspector>;
    using ConstInspectorScope = InspectorScopeImpl<const CustomInspector>;

} // namespace tf3d::inspector
