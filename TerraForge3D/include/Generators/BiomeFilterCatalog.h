#pragma once

#include "Generators/BiomeFilterDefinition.h"

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::generators
{

    class BiomeFilterCatalog
    {
    public:
        BiomeFilterCatalog(ApplicationState *appState);

        bool Reload();
        inline const std::vector<std::shared_ptr<BiomeFilterDefinition>> &GetDefinitions() const
        {
            return m_Definitions;
        }
        std::shared_ptr<BiomeFilterDefinition> FindByID(const std::string &id) const;

    private:
        ApplicationState *m_AppState = nullptr;
        std::vector<std::shared_ptr<BiomeFilterDefinition>> m_Definitions;
    };

} // namespace tf3d::generators
using tf3d::generators::BiomeFilterCatalog;
