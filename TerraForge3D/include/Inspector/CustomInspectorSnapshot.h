#pragma once

#include "Inspector/CustomInspectorDataStore.h"
#include "Inspector/CustomInspectorScope.h"

namespace tf3d::inspector
{

    class CustomInspector;
    class CustomInspectorSnapshot;

    using SnapshotScope = Scope<const CustomInspectorSnapshot>;

    class CustomInspectorSnapshot
    {
    public:
        explicit CustomInspectorSnapshot(const CustomInspector &inspector);

        const CustomInspectorDataStore &GetDataStore() const
        {
            return m_DataStore;
        }

        SnapshotScope Root() const;

    private:
        template <typename>
        friend class ScopeHandle;

        CustomInspectorDataStore m_DataStore;
    };

} // namespace tf3d::inspector
