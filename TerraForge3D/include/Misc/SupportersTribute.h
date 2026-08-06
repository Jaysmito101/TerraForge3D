#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace tf3d::base
{
    class Texture2D;
}
using tf3d::base::Texture2D;

namespace tf3d::misc
{

    struct GitHubData {
        std::string name  = "";
        Texture2D *avatar = nullptr;
    };

    class SupportersTribute
    {
    public:
        SupportersTribute();
        ~SupportersTribute();
        void ShowSettings(bool *pOpen);

    private:
        void LoadstargazersData(nlohmann::json &data);
        void LoadcontributorsData(nlohmann::json &data);
        bool stargazersUnavailable = false;

    public:
        std::vector<GitHubData> stargazers;
        std::vector<GitHubData> contributors;
    };

} // namespace tf3d::misc
using tf3d::misc::SupportersTribute;
