#pragma once

#include <string>
#include <vector>

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::misc
{

    class OSLiscences
    {
    public:
        OSLiscences(ApplicationState *appState);
        ~OSLiscences();

        void ShowSettings(bool *pOpen);

    private:
        void ShowLisc(std::string &name, std::string &content, int id);

    public:
        ApplicationState *appState;
        std::vector<std::pair<std::string, std::string>> osls;
    };

} // namespace tf3d::misc