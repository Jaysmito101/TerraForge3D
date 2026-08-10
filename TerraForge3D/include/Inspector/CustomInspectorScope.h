#pragma once

#include "Inspector/CustomInspectorCore.h"

#include <concepts>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tf3d::inspector
{

    template <typename Owner>
    class ScopeHandle
    {
        static_assert(requires(const Owner &owner) { owner.GetDataStore(); }, "Scope owner must expose a data store");

    public:
        ScopeHandle Scope(std::string_view path) const
        {
            return ScopeHandle(m_Owner, JoinPath(m_Path, path));
        }

        template <typename T>
        T Get(std::string_view name, T fallback = {}) const
        {
            return m_Owner.GetDataStore().At(JoinPath(m_Path, name)).Get(fallback);
        }

        bool Contains(std::string_view name) const
        {
            return m_Owner.GetDataStore().Contains(JoinPath(m_Path, name));
        }

        template <typename T>
            requires requires(Owner &owner, std::string_view path, T value) {
                { owner.SetExactValue(path, std::move(value)) } -> std::same_as<bool>;
            }
        bool Set(std::string_view name, T value) const
        {
            return m_Owner.SetExactValue(JoinPath(m_Path, name), std::move(value));
        }

        const CustomInspectorValue *Find(std::string_view name) const
            requires requires(const Owner &owner, std::string_view path) {
                { owner.FindExactValue(path) } -> std::same_as<const CustomInspectorValue *>;
            }
        {
            return m_Owner.FindExactValue(JoinPath(m_Path, name));
        }

        bool Remove(std::string_view name) const
            requires requires(Owner &owner, std::string_view path) {
                { owner.RemoveExactValue(path) } -> std::same_as<bool>;
            }
        {
            return m_Owner.RemoveExactValue(JoinPath(m_Path, name));
        }

        bool SetDropdownOptions(std::string_view name,
                                const std::vector<std::string> &options,
                                const std::vector<int32_t> &values = {}) const
            requires requires(Owner &owner,
                              std::string_view path,
                              const std::vector<std::string> &optionNames,
                              const std::vector<int32_t> &optionValues) {
                { owner.SetDropdownOptionsAt(path, optionNames, optionValues) } -> std::same_as<bool>;
            }
        {
            return m_Owner.SetDropdownOptionsAt(JoinPath(m_Path, name), options, values);
        }

        const std::string &GetPath() const
        {
            return m_Path;
        }

    private:
        friend class CustomInspector;
        friend class CustomInspectorSnapshot;

        ScopeHandle(Owner &owner, std::string path)
            : m_Owner(owner), m_Path(std::move(path))
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

        Owner &m_Owner;
        std::string m_Path;
    };

    using InspectorScope      = Scope<CustomInspector>;
    using ConstInspectorScope = Scope<const CustomInspector>;

} // namespace tf3d::inspector
